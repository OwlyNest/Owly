#include "owlylexer.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define TOKEN_TYPE_WIDTH 24
#define MAX_DEBUG_LINE_WIDTH 70

static const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF:             return "EOF";
        case TOKEN_IDENTIFIER:      return "IDENTIFIER";
        case TOKEN_NUMBER:          return "NUMBER";
        case TOKEN_STRING:          return "STRING";
        case TOKEN_SYMBOL:          return "SYMBOL";
        case TOKEN_KEYWORD_FUNC:    return "KEYWORD_FUNC";
        case TOKEN_KEYWORD_PRINT:   return "KEYWORD_PRINT";
        case TOKEN_KEYWORD_INT:     return "KEYWORD_INT";
        case TOKEN_KEYWORD_LIST:    return "KEYWORD_LIST";
        case TOKEN_KEYWORD_ARRAY:   return "KEYWORD_ARRAY";
        case TOKEN_FOR:             return "FOR";
        case TOKEN_WHILE:           return "WHILE";
        case TOKEN_OPERATOR_LT:     return "TOKEN_OPERATOR_LT";
        case TOKEN_OPERATOR_GT:     return "TOKEN_OPERATOR_GT";
        case TOKEN_OPERATOR_LTE:    return "TOKEN_OPERATOR_LTE";
        case TOKEN_OPERATOR_GTE:    return "TOKEN_OPERATOR_GTE";
        case TOKEN_OPERATOR_EQ:     return "TOKEN_OPERATOR_EQ";
        case TOKEN_OPERATOR_NEQ:    return "TOKEN_OPERATOR_NEQ";
        case TOKEN_INC:             return "TOKEN_INCREMENT";
        case TOKEN_DEC:             return "TOKEN_DECREMENT";

        default: return "UNKNOWN";
    }
}

static void print_token(Token tok, int debug) {
    if (debug) {
        const char* type_str = token_type_to_string(tok.type);
        int lexeme_len = (int)tok.length;
        
        // First, print the left part into a buffer
        char buffer[256];
        int n = snprintf(buffer, sizeof(buffer),
                        "[DEBUG]: token = { type: %-*s, lexeme: '%.*s'",
                        TOKEN_TYPE_WIDTH, type_str,
                        lexeme_len, tok.lexeme);


        int padding = MAX_DEBUG_LINE_WIDTH - n;
        if (padding < 0) padding = 0;

        printf("%s%*s}\n", buffer, padding, "");
    }
}

static const char *src = NULL;
static size_t pos = 0;

static char current_char() {
    return src[pos];
}

static void advance() {
    if (src[pos] != '\0') pos++;
}

void lexer_init(const char *source_code) {
    src = source_code;
    pos = 0;
}

void lexer_cleanup(void) {
    // nothing to do
}

static void skip_whitespace() {
    while (isspace(current_char())) advance();
}

Token lexer_next_token(int debug) {
    skip_whitespace();

    if (current_char() == '\0') {
        Token tok;
        tok.type = TOKEN_EOF;
        tok.lexeme = "";
        tok.length = 0;
        print_token(tok, debug);
        return tok;
    }

    Token tok;
    tok.lexeme = NULL;
    tok.length = 0;

    const char *start = &src[pos];

    if (isalpha(current_char()) || current_char() == '_') {
        while (isalnum(current_char()) || current_char() == '_') advance();
        tok.type = TOKEN_IDENTIFIER;
        tok.lexeme = start;
        tok.length = &src[pos] - start;

        if (tok.length == 4 && strncmp(start, "func", 4) == 0)
            tok.type = TOKEN_KEYWORD_FUNC;
        else if (tok.length == 5 && strncmp(start, "print", 5) == 0)
            tok.type = TOKEN_KEYWORD_PRINT;
        else if (tok.length == 3 && strncmp(start, "int", 3) == 0)
            tok.type = TOKEN_KEYWORD_INT;
        else if (tok.length == 4 && strncmp(start, "list", 4) == 0)
            tok.type = TOKEN_KEYWORD_LIST;
        else if (tok.length == 5 && strncmp(start, "array", 5) == 0)
            tok.type = TOKEN_KEYWORD_ARRAY;
        else if (tok.length == 2 && strncmp(start, "if", 2) == 0)
            tok.type = TOKEN_IF;
        else if (tok.length == 4 && strncmp(start, "elif", 4) == 0)
            tok.type = TOKEN_ELIF;
        else if (tok.length == 4 && strncmp(start, "else", 4) == 0)
            tok.type = TOKEN_ELSE;
        else if (tok.length == 3 && strncmp(start, "for", 3) == 0)
            tok.type = TOKEN_FOR;
        else if (tok.length == 5 && strncmp(start, "while", 5) == 0)
            tok.type = TOKEN_WHILE;

        print_token(tok, debug);
        return tok;
    }

    if (isdigit(current_char())) {
        while (isdigit(current_char())) advance();
        tok.type = TOKEN_NUMBER;
        tok.lexeme = start;
        tok.length = &src[pos] - start;
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '"') {
        advance();
        const char *str_start = &src[pos];
        while (current_char() != '\0' && current_char() != '"') {
            advance();
        }
        size_t len = &src[pos] - str_start;
        if (current_char() == '"') advance();

        tok.type = TOKEN_STRING;
        tok.lexeme = str_start;
        tok.length = len;
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '=' && src[pos + 1] == '=') {
        advance(); advance();
        tok.type = TOKEN_OPERATOR_EQ;
        tok.lexeme = start;
        tok.length = 2;
        print_token(tok, debug);
        return tok;
    }
    if (current_char() == '!' && src[pos + 1] == '=') {
        advance(); advance();
        tok.type = TOKEN_OPERATOR_NEQ;
        tok.lexeme = start;
        tok.length = 2;
        print_token(tok, debug);
        return tok;
    }
    if (current_char() == '<' && src[pos + 1] == '=') {
        advance(); advance();
        tok.type = TOKEN_OPERATOR_LTE;
        tok.lexeme = start;
        tok.length = 2;
        print_token(tok, debug);
        return tok;
    }
    if (current_char() == '>' && src[pos + 1] == '=') {
        advance(); advance();
        tok.type = TOKEN_OPERATOR_GTE;
        tok.lexeme = start;
        tok.length = 2;
        print_token(tok, debug);
        return tok;
    }
    if (current_char() == '+' && src[pos + 1] == '+') {
        advance(); advance();
        tok.type = TOKEN_INC;
        tok.lexeme = start;
        tok.length = 2;
        print_token(tok, debug);
        return tok;
    }
    if (current_char() == '-' && src[pos + 1] == '-') {
        advance(); advance();
        tok.type = TOKEN_DEC;
        tok.lexeme = start;
        tok.length = 2;
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '<') {
        advance();
        tok.type = TOKEN_OPERATOR_LT;
        tok.lexeme = start;
        tok.length = 1;
        print_token(tok, debug);
        return tok;
    }
    if (current_char() == '>') {
        advance();
        tok.type = TOKEN_OPERATOR_GT;
        tok.lexeme = start;
        tok.length = 1;
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '[') {
        advance();
        tok.type = TOKEN_SYMBOL;
        tok.lexeme = start;
        tok.length = 1;
        print_token(tok, debug);
        return tok;
    }
    if (current_char() == ']') {
        advance();
        tok.type = TOKEN_SYMBOL;
        tok.lexeme = start;
        tok.length = 1;
        print_token(tok, debug);
        return tok;
    }

    tok.type = TOKEN_SYMBOL;
    tok.lexeme = start;
    tok.length = 1;
    advance();
    print_token(tok, debug);
    return tok;
}