/*
	* src/front/lexer.c - Owly's tokenizer: turns text into shiny tokens. Owly's eagle eyes spotting keywords and operators.
	* Author:   Amity
	* Date:     Tue Feb 17 09:58:17 2026
	* Copyright © 2026 OwlyNest
*/

/* --- Styling Instructions ---
	* Encoding:                      UTF-8, Unix line endings
	* Text font:                     Monospace
	* Line width:                    Max 80 characters
	* Indentation:                   Use 4 spaces
	* Brace style:                   Same line as control statement
	* Inline comments:               Column 40, wherever possible, else, whole multiple of 20
	* Section headers:               Use 3 '-' characters before and after
	* Pointer notation:              Next to variable name, not type
	* Binary operations:             Space around operator
	* Empty parameter list:          Use (void) instead of ()
	* Statements and declarations:   Max one per line
*/

/* --- Macros ---*/
#define TOKEN_TYPE_WIDTH 24
#define MAX_DEBUG_LINE_WIDTH 75
/* --- Includes ---*/
#include <ctype.h>
#include <string.h>
#include <stdio.h>

#include "front/lexer.h"
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
extern int debug;
/* --- Prototypes ---*/
void scan(const char *source);
const char* token_type_to_string(TokenType type);
void print_token(Token tok, int debug);
char current_char(void);
void advance(void);
void lexer_init(const char *source_code);
void skip_whitespace(void);
Token lexer_next_token(int debug);
/* --- Functions ---*/

void scan(const char *source) {
    lexer_init(source);

    Token tok;
    do {
        tok = lexer_next_token(debug);
    } while (tok.type != TOKEN_EOF);
}

const char* token_type_to_string(TokenType type) {
    switch (type) {
        case TOKEN_EOF:                     return "EOF";
        case TOKEN_UNKNOWN:                 return "UNKNOWN";

        case TOKEN_KEYWORD_ARR:             return "ARR";        
        case TOKEN_KEYWORD_AUTO:            return "AUTO";
        case TOKEN_KEYWORD_BREAK:           return "BREAK";
        case TOKEN_KEYWORD_CASE:            return "CASE";
        case TOKEN_KEYWORD_CHAR:            return "CHAR";
        case TOKEN_KEYWORD_CONST:           return "CONST";
        case TOKEN_KEYWORD_CONTINUE:        return "CONTINUE";
        case TOKEN_KEYWORD_DEFAULT:         return "DEFAULT";
        case TOKEN_KEYWORD_DO:              return "DO";
        case TOKEN_KEYWORD_DOUBLE:          return "DOUBLE";
        case TOKEN_KEYWORD_ELSE:            return "ELSE";
        case TOKEN_KEYWORD_ENUM:            return "ENUM";
        case TOKEN_KEYWORD_EXTERN:          return "EXTERN";
        case TOKEN_KEYWORD_FLOAT:           return "FLOAT";
        case TOKEN_KEYWORD_FOR:             return "FOR";
        case TOKEN_KEYWORD_FUNC:            return "FUNC";
        case TOKEN_KEYWORD_IF:              return "IF";
        case TOKEN_KEYWORD_INLINE:          return "INLINE";
        case TOKEN_KEYWORD_INT:             return "INT";
        case TOKEN_KEYWORD_LONG:            return "LONG";
        case TOKEN_KEYWORD_REGISTER:        return "REGISTER";
        case TOKEN_KEYWORD_RESTRICT:        return "RESTRICT";
        case TOKEN_KEYWORD_RETURN:          return "RETURN";
        case TOKEN_KEYWORD_SHORT:           return "SHORT";
        case TOKEN_KEYWORD_SIGNED:          return "SIGNED";
        case TOKEN_KEYWORD_SIZEOF:          return "SIZEOF";
        case TOKEN_KEYWORD_STATIC:          return "STATIC";
        case TOKEN_KEYWORD_STRUCT:          return "STRUCT";
        case TOKEN_KEYWORD_SWITCH:          return "SWITCH";
        case TOKEN_KEYWORD_TYPEDEF:         return "TYPEDEF";
        case TOKEN_KEYWORD_UNION:           return "UNION";
        case TOKEN_KEYWORD_UNSIGNED:        return "UNSIGNED";
        case TOKEN_KEYWORD_VAR:             return "VAR";
        case TOKEN_KEYWORD_VOID:            return "VOID";
        case TOKEN_KEYWORD_VOLATILE:        return "VOLATILE";
        case TOKEN_KEYWORD_WHILE:           return "WHILE";
        case TOKEN_KEYWORD_BOOL:            return "_BOOL";
        case TOKEN_KEYWORD_COMPLEX:         return "_COMPLEX";
        case TOKEN_KEYWORD_IMAGINARY:       return "_IMAGINARY";
    
        case TOKEN_OPERATOR_PLUS:           return "PLUS";
        case TOKEN_OPERATOR_MINUS:          return "MINUS";
        case TOKEN_OPERATOR_STAR:           return "STAR";
        case TOKEN_OPERATOR_SLASH:          return "SLASH";
        case TOKEN_OPERATOR_PERCENT:        return "PERCENT";
        case TOKEN_OPERATOR_INCREMENT:      return "INCREMENT";
        case TOKEN_OPERATOR_DECREMENT:      return "DECREMENT";
        case TOKEN_OPERATOR_ASSIGN:         return "ASSIGN";
        case TOKEN_OPERATOR_PLUSASSIGN:     return "PLUS_ASSIGN";
        case TOKEN_OPERATOR_MINUSASSIGN:    return "MINUS_ASSIGN";
        case TOKEN_OPERATOR_STARASSIGN:     return "STAR_ASSIGN";
        case TOKEN_OPERATOR_SLASHASSIGN:    return "SLASH_ASSIGN";
        case TOKEN_OPERATOR_PERCENTASSIGN:  return "PERCENT_ASSIGN";
        case TOKEN_OPERATOR_EQUAL:          return "EQUAL";
        case TOKEN_OPERATOR_NEQUAL:         return "NOT_EQUAL";
        case TOKEN_OPERATOR_GREATER:        return "GREATER";
        case TOKEN_OPERATOR_LOWER:          return "LOWER";
        case TOKEN_OPERATOR_GEQ:            return "GREATER_EQUAL";
        case TOKEN_OPERATOR_LEQ:            return "LOWER_EQUAL";
        case TOKEN_OPERATOR_NOT:            return "NOT";
        case TOKEN_OPERATOR_AND:            return "AND";
        case TOKEN_OPERATOR_OR:             return "OR";
        case TOKEN_OPERATOR_AMP:            return "AMPERSAND";
        case TOKEN_OPERATOR_BITOR:          return "BIT_OR";
        case TOKEN_OPERATOR_BITXOR:         return "BIT_XOR";
        case TOKEN_OPERATOR_BITNOT:         return "BIT_NOT";
        case TOKEN_OPERATOR_BITSHL:         return "SHL";
        case TOKEN_OPERATOR_BITSHR:         return "SHR";
        case TOKEN_OPERATOR_BITANDASSIGN:   return "AND_ASSIGN";
        case TOKEN_OPERATOR_BITORASSIGN:    return "OR_ASSIGN";
        case TOKEN_OPERATOR_BITXORASSIGN:   return "BITXOR_ASSIGN";
        case TOKEN_OPERATOR_BITSHLASSIGN:   return "SHL_ASSIGN";
        case TOKEN_OPERATOR_BITSHRASSIGN:   return "SHR_ASSIGN";
        case TOKEN_OPERATOR_POINT:          return "POINT";
        case TOKEN_OPERATOR_ARROW:          return "ARROW";
        case TOKEN_OPERATOR_ELLIPSIS:        return "ELLIPSIS";
    
        case TOKEN_LPAREN:                  return "LEFT_PARENTHESIS";
        case TOKEN_RPAREN:                  return "RIGHT_PARENTHESIS";
        case TOKEN_LBRACKET:                return "LEFT_BRACKET";
        case TOKEN_RBRACKET:                return "RIGHT_BRACKET";
        case TOKEN_LBRACE:                  return "LEFT_BRACE";
        case TOKEN_RBRACE:                  return "RIGHT_BRACE";
    
        case TOKEN_COMMA:                   return "COMMA";
        case TOKEN_COLON:                   return "COLON";
        case TOKEN_SEMICOLON:               return "SEMICOLON";
        case TOKEN_QUESTION:                return "QMARK";
        case TOKEN_HASH:                    return "HASH";
        
        case TOKEN_IDENTIFIER:              return "IDENTIFIER";
        
        case TOKEN_LITERAL_STRING:          return "STRING_LITERAL";
        case TOKEN_LITERAL_CHAR:            return "CHAR_LITERAL";
        case TOKEN_LITERAL_INT:             return "INT_LITERAL";
        case TOKEN_LITERAL_FLOAT:           return "FLOAT_LITERAL";
        default:                            return "UNKNOWN";
    }
}

void print_token(Token tok, int debug) {
    const char* type_str = token_type_to_string(tok.type);
    int lexeme_len = (int)tok.length;
    
    // First, print the left part into a buffer
    char buffer[256];
    int n = snprintf(buffer, sizeof(buffer), "token = { type: %-*s, lexeme: '%.*s'", TOKEN_TYPE_WIDTH, type_str, lexeme_len, tok.lexeme);

    // Calculate remaining space until alignment point
    int padding = MAX_DEBUG_LINE_WIDTH - n;
    if (padding < 0) padding = 0;

    if (debug) {
        printf("[DEBUG]: %s%*s}\n", buffer, padding, "");
    }
    FILE *tokfile = fopen("out/list.tok", "a");
    fprintf(tokfile, "%s, \"%.*s\";\n", type_str, lexeme_len, tok.lexeme);
    fclose(tokfile);
}

static const char *src = NULL;
static size_t pos = 0;

char current_char(void) {
    return src[pos];
}

// Advance position
void advance(void) {
    if (src[pos] != '\0') pos++;
}

void lexer_init(const char *source_code) {
    src = source_code;
    pos = 0;
}

void skip_whitespace(void) {
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

    /* Identifiers and keywords */
    if (isalpha(current_char()) || current_char() == '_') {
        while (isalnum(current_char()) || current_char() == '_') advance();
        tok.type = TOKEN_IDENTIFIER;
        tok.lexeme = start;
        tok.length = &src[pos] - start;

        // Keywords check
        if (tok.length == 3 && strncmp(start, "arr", 3) == 0 ) {
            tok.type = TOKEN_KEYWORD_ARR;
        } else if (tok.length == 4 && strncmp(start, "auto", 4) == 0)
            tok.type = TOKEN_KEYWORD_AUTO;
        else if (tok.length == 5 && strncmp(start, "break", 5) == 0)
            tok.type = TOKEN_KEYWORD_BREAK;
        else if (tok.length == 4 && strncmp(start, "case", 4) == 0)    
            tok.type = TOKEN_KEYWORD_CASE;
        else if (tok.length == 4 && strncmp(start, "char", 4) == 0)
            tok.type = TOKEN_KEYWORD_CHAR;
        else if (tok.length == 5 && strncmp(start, "const", 5) == 0)
            tok.type = TOKEN_KEYWORD_CONST;
        else if (tok.length == 8 && strncmp(start, "continue", 8) == 0)
            tok.type = TOKEN_KEYWORD_CONTINUE;
        else if (tok.length == 7 && strncmp(start, "default", 7) == 0)
            tok.type = TOKEN_KEYWORD_DEFAULT;
        else if (tok.length == 2 && strncmp(start, "do", 2) == 0)
            tok.type = TOKEN_KEYWORD_DO;
        else if (tok.length == 6 && strncmp(start, "double", 6) == 0)
            tok.type = TOKEN_KEYWORD_DOUBLE;
        else if (tok.length == 4 && strncmp(start, "else", 4) == 0)
            tok.type = TOKEN_KEYWORD_ELSE;
        else if (tok.length == 4 && strncmp(start, "enum", 4) == 0)
            tok.type = TOKEN_KEYWORD_ENUM;
        else if (tok.length == 6 && strncmp(start, "extern", 6) == 0)
            tok.type = TOKEN_KEYWORD_EXTERN;
        else if (tok.length == 5 && strncmp(start, "float", 5) == 0)
            tok.type = TOKEN_KEYWORD_FLOAT;
        else if (tok.length == 3 && strncmp(start, "for", 3) == 0)
            tok.type = TOKEN_KEYWORD_FOR;
        else if (tok.length == 4 && strncmp(start, "func", 4) == 0)
            tok.type = TOKEN_KEYWORD_FUNC;
        else if (tok.length == 2 && strncmp(start, "if", 2) == 0)
            tok.type = TOKEN_KEYWORD_IF;
        else if (tok.length == 6 && strncmp(start, "inline", 6) == 0)
            tok.type = TOKEN_KEYWORD_INLINE;
        else if (tok.length == 3 && strncmp(start, "int", 3) == 0)
            tok.type = TOKEN_KEYWORD_INT;
        else if (tok.length == 4 && strncmp(start, "long", 4) == 0)
            tok.type = TOKEN_KEYWORD_LONG;
        else if (tok.length == 8 && strncmp(start, "register", 8) == 0)
            tok.type = TOKEN_KEYWORD_REGISTER;
        else if (tok.length == 8 && strncmp(start, "restrict", 8) == 0)
            tok.type = TOKEN_KEYWORD_RESTRICT;
        else if (tok.length == 3 && strncmp(start, "ret", 3) == 0)
            tok.type = TOKEN_KEYWORD_RETURN;
        else if (tok.length == 5 && strncmp(start, "short", 5) == 0)
            tok.type = TOKEN_KEYWORD_SHORT;
        else if (tok.length == 6 && strncmp(start, "signed", 6) == 0)
            tok.type = TOKEN_KEYWORD_SIGNED;
        else if (tok.length == 6 && strncmp(start, "sizeof", 6) == 0)
            tok.type = TOKEN_KEYWORD_SIZEOF;
        else if (tok.length == 6 && strncmp(start, "static", 6) == 0)
            tok.type = TOKEN_KEYWORD_STATIC;
        else if (tok.length == 6 && strncmp(start, "struct", 6) == 0)
            tok.type = TOKEN_KEYWORD_STRUCT;
        else if (tok.length == 6 && strncmp(start, "switch", 6) == 0)
            tok.type = TOKEN_KEYWORD_SWITCH;
        else if (tok.length == 7 && strncmp(start, "typedef", 7) == 0)
            tok.type = TOKEN_KEYWORD_TYPEDEF;
        else if (tok.length == 5 && strncmp(start, "union", 5) == 0)
            tok.type = TOKEN_KEYWORD_UNION;
        else if (tok.length == 8 && strncmp(start, "unsigned", 8) == 0)
            tok.type = TOKEN_KEYWORD_UNSIGNED;
        else if (tok.length == 3 && strncmp(start, "var", 3) == 0)
            tok.type = TOKEN_KEYWORD_VAR;
        else if (tok.length == 4 && strncmp(start, "void", 4) == 0)
            tok.type = TOKEN_KEYWORD_VOID;
        else if (tok.length == 8 && strncmp(start, "volatile", 8) == 0)
            tok.type = TOKEN_KEYWORD_VOLATILE;
        else if (tok.length == 5 && strncmp(start, "while", 5) == 0)
            tok.type = TOKEN_KEYWORD_WHILE;
        else if (tok.length == 5 && strncmp(start, "_Bool", 5) == 0)
            tok.type = TOKEN_KEYWORD_BOOL;
        else if (tok.length == 8 && strncmp(start, "_Complex", 8) == 0)
            tok.type = TOKEN_KEYWORD_COMPLEX;
        else if (tok.length == 10 && strncmp(start, "_Imaginary", 10) == 0)
            tok.type = TOKEN_KEYWORD_IMAGINARY;

        print_token(tok, debug);
        return tok;
    }

    /* Literals */
    // Int and float literal
    if (isdigit(current_char())) {
        const char *start = &src[pos];
        int is_float = 0;

        // Handle base prefixes: 0x, 0b, 0
        if (current_char() == '0') {
            advance();
            if (tolower(current_char()) == 'x') { // hex
                advance();
                while (isxdigit(current_char())) advance();
                tok.type = TOKEN_LITERAL_INT;
            } else if (tolower(current_char()) == 'b') { // binary
                advance();
                while (current_char() == '0' || current_char() == '1') advance();
                tok.type = TOKEN_LITERAL_INT;
            } else if (isdigit(current_char())) { // octal (0...7)
                while (current_char() >= '0' && current_char() <= '7') advance();
                tok.type = TOKEN_LITERAL_INT;
            } else {
                // plain 0
                tok.type = TOKEN_LITERAL_INT;
            }
        } else {
            // Decimal or floating point
            while (isdigit(current_char())) advance();

            // Fractional part
            if (current_char() == '.') {
                is_float = 1;
                advance();
                while (isdigit(current_char())) advance();
            }

            // Exponent part
            if (current_char() == 'e' || current_char() == 'E') {
                is_float = 1;
                advance();
                if (current_char() == '+' || current_char() == '-') advance();
                while (isdigit(current_char())) advance();
            }

            tok.type = is_float ? TOKEN_LITERAL_FLOAT : TOKEN_LITERAL_INT;
        }

        // Optional suffixes (u, l, f, etc.) — skip but don’t classify yet
        while (isalpha(current_char())) advance();
        tok.lexeme = start;
        tok.length = &src[pos] - start;
        print_token(tok, debug);
        return tok;
    }   

    // Char literal
    if (current_char() == '\'') {
        advance(); // skip opening quote
        const char *character = &src[pos];
        
        if (current_char() == '\\') {
            advance();
            if (current_char() != '\0') advance();
        } else if (current_char() != '\0' && current_char() != '\'') {
            advance();
        }

        size_t len = &src[pos] - character;
        
        if (current_char() == '\'') advance(); // skip closing quote
        tok.type = TOKEN_LITERAL_CHAR;
        tok.lexeme = character;
        tok.length = len;
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

        tok.type = TOKEN_LITERAL_STRING;
        tok.lexeme = str_start;
        tok.length = len;
        print_token(tok, debug);
        return tok;
    }

    /* Operators */
    if (current_char() == '+') {
        const char *next = &src[pos + 1];
        if (strncmp(next, "+", 1) == 0) {
            tok.type = TOKEN_OPERATOR_INCREMENT;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_PLUSASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_PLUS;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '&') {
        const char *next = &src[pos + 1];
        if (strncmp(next, "&", 1) == 0) {
            tok.type = TOKEN_OPERATOR_AND;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_BITANDASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_AMP;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '|') {
        const char *next = &src[pos + 1];
        if (strncmp(next, "|", 1) == 0) {
            tok.type = TOKEN_OPERATOR_OR;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_BITORASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_BITOR;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '~') {
        tok.type = TOKEN_OPERATOR_BITNOT;
        tok.lexeme = &src[pos];
        tok.length = 1;
        advance();
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '-') {
        const char *next = &src[pos + 1];
        if (strncmp(next, "-", 1) == 0) {
            tok.type = TOKEN_OPERATOR_DECREMENT;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_MINUSASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else if (strncmp(next, ">", 1) == 0) {
            tok.type = TOKEN_OPERATOR_ARROW;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_MINUS;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '*') {
        const char *next = &src[pos+1];
        if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_STARASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_STAR;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '/') {
        const char *next = &src[pos+1];
        if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_SLASHASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_SLASH;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '%') {
        const char *next = &src[pos+1];
        if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_PERCENTASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_PERCENT;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '=') {
        const char *next = &src[pos+1];
        if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_EQUAL;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_ASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '!') {
        const char *next = &src[pos+1];
        if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_NEQUAL;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_NOT;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '^') {
        const char *next = &src[pos+1];
        if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_BITXORASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_BITXOR;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '>') {
        const char *next = &src[pos + 1];
        if (strncmp(next, ">=", 2) == 0) {
            tok.type = TOKEN_OPERATOR_BITSHRASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 3;
            advance();
            advance();
            advance();
        } else if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_GEQ;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else if (strncmp(next, ">", 1) == 0) {
            tok.type = TOKEN_OPERATOR_BITSHR;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_GREATER;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '<') {
        const char *next = &src[pos + 1];
        if (strncmp(next, "<=", 2) == 0) {
            tok.type = TOKEN_OPERATOR_BITSHLASSIGN;
            tok.lexeme = &src[pos];
            tok.length = 3;
            advance();
            advance();
            advance();
        } else if (strncmp(next, "=", 1) == 0) {
            tok.type = TOKEN_OPERATOR_LEQ;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else if (strncmp(next, "<", 1) == 0) {
            tok.type = TOKEN_OPERATOR_BITSHL;
            tok.lexeme = &src[pos];
            tok.length = 2;
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_LOWER;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    if (current_char() == '.') {
        const char *next = &src[pos + 1];
        if (strncmp(next, "..", 2) == 0) {
            tok.type = TOKEN_OPERATOR_ELLIPSIS;
            tok.lexeme = &src[pos];
            tok.length = 3;
            advance();
            advance();
            advance();
        } else {
            tok.type = TOKEN_OPERATOR_POINT;
            tok.lexeme = &src[pos];
            tok.length = 1;
            advance();
        }
        print_token(tok, debug);
        return tok;
    }

    /* Symbols */
    if (current_char() == '(') {tok.type = TOKEN_LPAREN;}
    else if (current_char() == ')') {tok.type = TOKEN_RPAREN;}
    else if (current_char() == '[') {tok.type = TOKEN_LBRACKET;}
    else if (current_char() == ']') {tok.type = TOKEN_RBRACKET;}
    else if (current_char() == '{') {tok.type = TOKEN_LBRACE;}
    else if (current_char() == '}') {tok.type = TOKEN_RBRACE;}
    else if (current_char() == ',') {tok.type = TOKEN_COMMA;}
    else if (current_char() == ':') {tok.type = TOKEN_COLON;}
    else if (current_char() == ';') {tok.type = TOKEN_SEMICOLON;}
    else if (current_char() == '?') {tok.type = TOKEN_QUESTION;}
    else if (current_char() == '#') {tok.type = TOKEN_HASH;}
    else {tok.type = TOKEN_UNKNOWN;}
    const char *symbol = &src[pos];
    advance();
    tok.lexeme = symbol;
    tok.length = 1;
    print_token(tok, debug);
    return tok;
}