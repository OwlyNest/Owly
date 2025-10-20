
#include "owlylexer.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#define TOKEN_TYPE_WIDTH 24
#define MAX_DEBUG_LINE_WIDTH 75

static const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF:             return "EOF";                   //done (I think)
        case TOKEN_IDENTIFIER:      return "IDENTIFIER";            //done (used at least)
        case TOKEN_NUMBER:          return "NUMBER";                // ^
        case TOKEN_STRING:          return "STRING";                // |
        case TOKEN_SYMBOL:          return "SYMBOL";                // |
        case TOKEN_KEYWORD_FUNC:    return "KEYWORD_FUNC";          //done
        case TOKEN_KEYWORD_PRINT:   return "KEYWORD_PRINT";         //done
        case TOKEN_KEYWORD_STR:     return "KEYWORD_STR";           //done
        case TOKEN_KEYWORD_INT:     return "KEYWORD_INT";           //done
        case TOKEN_KEYWORD_DOUBLE:  return "KEYWORD_double";        //done
        case TOKEN_KEYWORD_LIST:    return "KEYWORD_LIST";          //done
        case TOKEN_KEYWORD_ARRAY:   return "KEYWORD_ARRAY";         //done
        case TOKEN_IF:              return "IF";                    //done
        case TOKEN_ELIF:            return "ELIF";                  //done
        case TOKEN_ELSE:            return "ELSE";                  //done
        case TOKEN_FOR:             return "FOR";                   //done
        case TOKEN_WHILE:           return "WHILE";                 //done
        case TOKEN_OPERATOR_LT:     return "TOKEN_OPERATOR_LT";     //done
        case TOKEN_OPERATOR_GT:     return "TOKEN_OPERATOR_GT";     //done
        case TOKEN_OPERATOR_LTE:    return "TOKEN_OPERATOR_LTE";    //done
        case TOKEN_OPERATOR_GTE:    return "TOKEN_OPERATOR_GTE";    //done
        case TOKEN_OPERATOR_EQ:     return "TOKEN_OPERATOR_EQ";     //done
        case TOKEN_OPERATOR_NEQ:    return "TOKEN_OPERATOR_NEQ";    //done
        case TOKEN_INC:             return "TOKEN_INCREMENT";       //done
        case TOKEN_DEC:             return "TOKEN_DECREMENT";       //done
        case TOKEN_PLUS:            return "TOKEN_PLUS";            //done
        case TOKEN_MINUS:           return "TOKEN_MINUS";           //done
        case TOKEN_MULT:            return "TOKEN_MULTIPLY";        //done
        case TOKEN_POW:             return "TOKEN_POWER";           //done
        case TOKEN_DIV:             return "TOKEN_DIVIDE";          //done
        case TOKEN_MOD:             return "TOKEN_MODULO";          //done
        case TOKEN_BIT_AND:         return "TOKEN_BIT_AND";         //done
        case TOKEN_BIT_OR:          return "TOKEN_BIT_OR";          //done
        case TOKEN_BIT_XOR:         return "TOKEN_BIT_XOR";         //done
        case TOKEN_BIT_NOR:         return "TOKEN_BIT_NOR";         //done
        case TOKEN_BIT_NOT:         return "TOKEN_BIT_NOT";         //done
        case TOKEN_SHIFT_LEFT:      return "TOKEN_SHIFT_LEFT";      //done
        case TOKEN_SHIFT_RIGHT:     return "TOKEN_SHIFT_RIGHT";     //done
        case TOKEN_AND_ASSIGN:      return "TOKEN_AND_ASSIGN";      //done
        case TOKEN_OR_ASSIGN :      return "TOKEN_OR_ASSIGN";       //done
        case TOKEN_XOR_ASSIGN:      return "TOKEN_XOR_ASSIGN";      //done
        case TOKEN_SHL_ASSIGN:      return "TOKEN_SHL_ASSIGN";      //done
        case TOKEN_SHR_ASSIGN:      return "TOKEN_SHR_ASSIGN";      //done
        case TOKEN_LOGICAL_AND:     return "TOKEN_LOGICAL_AND";     //done
        case TOKEN_LOGICAL_OR:      return "TOKEN_LOGICAL_OR";      //done
        default: return "UNKNOWN";                                  //not needed
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

        // Calculate remaining space until alignment point
        int padding = MAX_DEBUG_LINE_WIDTH - n;
        if (padding < 0) padding = 0;

        // Print final aligned output
        printf("%s%*s}\n", buffer, padding, "");
    }
}

// Lexer state: source pointer and current position
static const char *src = NULL;
static size_t pos = 0;

// Return current char or 0 at end
static char current_char() {
    return src[pos];
}

// Advance position
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
    // First, skip whitespace BEFORE printing debug or anything else
    skip_whitespace();

    // Check if we reached the end after skipping whitespace
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

    // Identifier or keyword
    if (isalpha(current_char()) || current_char() == '_') {
        while (isalnum(current_char()) || current_char() == '_') advance();
        tok.type = TOKEN_IDENTIFIER;
        tok.lexeme = start;
        tok.length = &src[pos] - start;

        // Keywords check
        if (tok.length == 4 && strncmp(start, "func", 4) == 0)
            tok.type = TOKEN_KEYWORD_FUNC;
        else if (tok.length == 5 && strncmp(start, "print", 5) == 0)
            tok.type = TOKEN_KEYWORD_PRINT;
        else if (tok.length == 3 && strncmp(start, "str", 3) == 0)
            tok.type = TOKEN_KEYWORD_STR;
        else if (tok.length == 3 && strncmp(start, "int", 3) == 0)
            tok.type = TOKEN_KEYWORD_INT;
        else if (tok.length == 6 && strncmp(start, "double", 6) == 0)
            tok.type = TOKEN_KEYWORD_DOUBLE;
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

    // Number literal (supports decimals)
    if (isdigit(current_char())) {
        int has_dot = 0;
        while (isdigit(current_char()) || (current_char() == '.' && !has_dot)) {
            if (current_char() == '.') has_dot = 1;
            advance();
        }
        tok.type = TOKEN_NUMBER;
        tok.lexeme = start;
        tok.length = &src[pos] - start;
        print_token(tok, debug);
        return tok;
    }   

    // String literal
    if (current_char() == '"') {
        advance(); // skip opening quote
        const char *str_start = &src[pos];
        while (current_char() != '\0' && current_char() != '"') {
            advance();
        }
        size_t len = &src[pos] - str_start;
        if (current_char() == '"') advance(); // skip closing quote

        tok.type = TOKEN_STRING;
        tok.lexeme = str_start;
        tok.length = len;
        print_token(tok, debug);
        return tok;
    }

    // Multi-character operators
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
    } else if (current_char() == '+') {
        advance();
        tok.type = TOKEN_PLUS;
        tok.lexeme = start;
        tok.length = 1;
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
    } else if (current_char() == '-') {
        advance();
        tok.type = TOKEN_MINUS;
        tok.lexeme = start;
        tok.length = 1;
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '*' && src[pos + 1] == '*') {
        advance(); advance();
        tok.type = TOKEN_POW;
        tok.lexeme = start;
        tok.length = 2;
        print_token(tok, debug);
        return tok;
    } else if (current_char() == '*') {
        advance();
        tok.type = TOKEN_MULT;
        tok.lexeme = start;
        tok.length = 1;
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '/') {
        advance();
        tok.type= TOKEN_DIV;
        tok.length = 1;
        print_token(tok, debug);
        return tok;

    }

    if (current_char() == '%') {
        advance();
        tok.type= TOKEN_MOD;
        tok.length = 1;
        print_token(tok, debug);
        return tok;

    }

    // Bitwise and shift operators
    if (current_char() == '&') {
        if (src[pos + 1] == '&') {
            // Logical AND (if you want it, like C's &&)
            advance(); advance();
            tok.type = TOKEN_LOGICAL_AND;
            tok.lexeme = start;
            tok.length = 2;
        } else if (src[pos + 1] == '=') {
            advance(); advance();
            tok.type = TOKEN_AND_ASSIGN;
            tok.lexeme = start;
            tok.length = 2;
        } else {
            advance();
            tok.type = TOKEN_BIT_AND;
            tok.lexeme = start;
            tok.length = 1;
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '|') {
        if (src[pos + 1] == '|') {
            // Logical OR (if you want it, like C's ||)
            advance(); advance();
            tok.type = TOKEN_LOGICAL_OR;
            tok.lexeme = start;
            tok.length = 2;
        } else if (src[pos + 1] == '=') {
            advance(); advance();
            tok.type = TOKEN_OR_ASSIGN;
            tok.lexeme = start;
            tok.length = 2;
        } else {
            advance();
            tok.type = TOKEN_BIT_OR;
            tok.lexeme = start;
            tok.length = 1;
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '^') {
        if (src[pos + 1] == '=') {
            advance(); advance();
            tok.type = TOKEN_XOR_ASSIGN;
            tok.lexeme = start;
            tok.length = 2;
        } else {
            advance();
            tok.type = TOKEN_BIT_XOR;
            tok.lexeme = start;
            tok.length = 1;
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '~') {
        advance();
        tok.type = TOKEN_BIT_NOT;
        tok.lexeme = start;
        tok.length = 1;
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '<' && src[pos + 1] == '<') {
        if (src[pos + 2] == '=') {
            advance(); advance(); advance();
            tok.type = TOKEN_SHL_ASSIGN;
            tok.lexeme = start;
            tok.length = 3;
        } else {
            advance(); advance();
            tok.type = TOKEN_SHIFT_LEFT;
            tok.lexeme = start;
            tok.length = 2;
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '>' && src[pos + 1] == '>') {
        if (src[pos + 2] == '=') {
            advance(); advance(); advance();
            tok.type = TOKEN_SHR_ASSIGN;
            tok.lexeme = start;
            tok.length = 3;
        } else {
            advance(); advance();
            tok.type = TOKEN_SHIFT_RIGHT;
            tok.lexeme = start;
            tok.length = 2;
        }
        print_token(tok, debug);
        return tok;
    }

    // Single-character operators
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

    // Symbols like [ and ]
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

    // Single-character symbol fallback
    tok.type = TOKEN_SYMBOL;
    tok.lexeme = start;
    tok.length = 1;
    advance();
    print_token(tok, debug);
    return tok;
}
