/*
	* include/lexer.h - [Enter description]
	* Author:   Amity
	* Date:     Mon Feb 16 15:05:09 2026
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
#ifndef LEXER_H
#define LEXER_H
/* --- Includes ---*/
#include <stddef.h>
#include <stdbool.h>
/* --- Typedefs - Structs - Enums ---*/
typedef enum TokenType {
    TOKEN_EOF,
    TOKEN_UNKNOWN,

    TOKEN_KEYWORD_ARR,                 // done
    TOKEN_KEYWORD_AUTO,                // done
    TOKEN_KEYWORD_BREAK,               // done
    TOKEN_KEYWORD_CASE,                // done
    TOKEN_KEYWORD_CHAR,                // done
    TOKEN_KEYWORD_CONST,               // done
    TOKEN_KEYWORD_CONTINUE,            // done
    TOKEN_KEYWORD_DEFAULT,             // done
    TOKEN_KEYWORD_DO,                  // done
    TOKEN_KEYWORD_DOUBLE,              // done
    TOKEN_KEYWORD_ELSE,                // done
    TOKEN_KEYWORD_ENUM,                // done
    TOKEN_KEYWORD_EXTERN,              // done
    TOKEN_KEYWORD_FLOAT,               // done
    TOKEN_KEYWORD_FOR,                 // done
    TOKEN_KEYWORD_FUNC,                // done
    TOKEN_KEYWORD_IF,                  // done
    TOKEN_KEYWORD_INLINE,              // done
    TOKEN_KEYWORD_INT,                 // done
    TOKEN_KEYWORD_LONG,                // done
    TOKEN_KEYWORD_REGISTER,            // done
    TOKEN_KEYWORD_RESTRICT,            // done
    TOKEN_KEYWORD_RETURN,              // done
    TOKEN_KEYWORD_SHORT,               // done
    TOKEN_KEYWORD_SIGNED,              // done
    TOKEN_KEYWORD_SIZEOF,              // done
    TOKEN_KEYWORD_STATIC,              // done
    TOKEN_KEYWORD_STRUCT,              // done
    TOKEN_KEYWORD_SWITCH,              // done
    TOKEN_KEYWORD_TYPEDEF,             // done
    TOKEN_KEYWORD_UNION,               // done
    TOKEN_KEYWORD_UNSIGNED,            // done
    TOKEN_KEYWORD_VAR,                 // done
    TOKEN_KEYWORD_VOID,                // done
    TOKEN_KEYWORD_VOLATILE,            // done
    TOKEN_KEYWORD_WHILE,               // done
    TOKEN_KEYWORD_BOOL,                // done
    TOKEN_KEYWORD_COMPLEX,             // done
    TOKEN_KEYWORD_IMAGINARY,           // done

    TOKEN_OPERATOR_PLUS,               // +
    TOKEN_OPERATOR_MINUS,              // -
    TOKEN_OPERATOR_STAR,               // *
    TOKEN_OPERATOR_SLASH,              // /
    TOKEN_OPERATOR_PERCENT,            // %
    TOKEN_OPERATOR_INCREMENT,          // ++
    TOKEN_OPERATOR_DECREMENT,          // --
    TOKEN_OPERATOR_ASSIGN,             // =
    TOKEN_OPERATOR_PLUSASSIGN,         // +=
    TOKEN_OPERATOR_MINUSASSIGN,        // -=
    TOKEN_OPERATOR_STARASSIGN,         // *=
    TOKEN_OPERATOR_SLASHASSIGN,        // /=
    TOKEN_OPERATOR_PERCENTASSIGN,      // %=
    TOKEN_OPERATOR_EQUAL,              // ==
    TOKEN_OPERATOR_NEQUAL,             // !=
    TOKEN_OPERATOR_GREATER,            // >
    TOKEN_OPERATOR_LOWER,              // <
    TOKEN_OPERATOR_GEQ,                // >=
    TOKEN_OPERATOR_LEQ,                // <=
    TOKEN_OPERATOR_NOT,                // !
    TOKEN_OPERATOR_AND,                // &&
    TOKEN_OPERATOR_OR,                 // ||
    TOKEN_OPERATOR_AMP,                // &
    TOKEN_OPERATOR_BITOR,              // |
    TOKEN_OPERATOR_BITXOR,             // ^
    TOKEN_OPERATOR_BITNOT,             // ~
    TOKEN_OPERATOR_BITSHL,             // <<
    TOKEN_OPERATOR_BITSHR,             // >>
    TOKEN_OPERATOR_BITANDASSIGN,       // &=
    TOKEN_OPERATOR_BITORASSIGN,        // |=
    TOKEN_OPERATOR_BITXORASSIGN,       // ^=
    TOKEN_OPERATOR_BITSHLASSIGN,       // <<=
    TOKEN_OPERATOR_BITSHRASSIGN,       // >>=
    TOKEN_OPERATOR_POINT,              // .
    TOKEN_OPERATOR_ARROW,              // ->
    TOKEN_OPERATOR_ELLIPSIS,           // ... // not done

    TOKEN_LPAREN,                      // (
    TOKEN_RPAREN,                      // )
    TOKEN_LBRACKET,                    // [ // not done
    TOKEN_RBRACKET,                    // ] // not done
    TOKEN_LBRACE,                      // {
    TOKEN_RBRACE,                      // }

    TOKEN_COMMA,                       // ,
    TOKEN_COLON,                       // :
    TOKEN_SEMICOLON,                   // ;
    TOKEN_QUESTION,                    // ? // done
    TOKEN_HASH,                        // # // not done
    
    TOKEN_IDENTIFIER,

    TOKEN_LITERAL_STRING,
    TOKEN_LITERAL_CHAR,
    TOKEN_LITERAL_INT,
    TOKEN_LITERAL_FLOAT,
} TokenType;

// Token structure
typedef struct Token {
    TokenType type;
    const char *lexeme;
    size_t length;
} Token;

typedef struct TokenList {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenList;
/* --- Globals ---*/

/* --- Prototypes ---*/
void scan(const char *source);
const char* token_type_to_string(TokenType type);
void print_token(Token tok, int debug);
char current_char(void);
void advance(void);
void lexer_init(const char *source_code);
void skip_whitespace();
Token lexer_next_token(int debug);

#endif