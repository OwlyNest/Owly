#ifndef OWLYLEXER_H
#define OWLYLEXER_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_UNKNOWN,

    TOKEN_KEYWORD_AUTO,
    TOKEN_KEYWORD_BREAK,
    TOKEN_KEYWORD_CASE,
    TOKEN_KEYWORD_CHAR,
    TOKEN_KEYWORD_CONST,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_KEYWORD_DEFAULT,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_DOUBLE,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_ENUM,
    TOKEN_KEYWORD_EXTERN,
    TOKEN_KEYWORD_FLOAT,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_FUNC,
    TOKEN_KEYWORD_GOTO,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_INLINE,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_LONG,
    TOKEN_KEYWORD_REGISTER,
    TOKEN_KEYWORD_RESTRICT,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_SHORT,
    TOKEN_KEYWORD_SIGNED,
    TOKEN_KEYWORD_SIZEOF,
    TOKEN_KEYWORD_STATIC,
    TOKEN_KEYWORD_STRUCT,
    TOKEN_KEYWORD_SWITCH,
    TOKEN_KEYWORD_TYPEDEF,
    TOKEN_KEYWORD_UNION,
    TOKEN_KEYWORD_UNSIGNED,
    TOKEN_KEYWORD_VAR,
    TOKEN_KEYWORD_VOID,
    TOKEN_KEYWORD_VOLATILE,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_BOOL,
    TOKEN_KEYWORD_COMPLEX,
    TOKEN_KEYWORD_IMAGINARY,

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
    TOKEN_OPERATOR_ELLIPSIS,           // ...

    TOKEN_LPAREN,                      // (
    TOKEN_RPAREN,                      // )
    TOKEN_LBRACKET,                    // [
    TOKEN_RBRACKET,                    // ]
    TOKEN_LBRACE,                      // {
    TOKEN_RBRACE,                      // }

    TOKEN_COMMA,                       // ,
    TOKEN_COLON,                       // :
    TOKEN_SEMICOLON,                   // ;
    TOKEN_QUESTION,                    // ?
    TOKEN_HASH,                        // #
    
    TOKEN_IDENTIFIER,

    TOKEN_LITERAL_STRING,
    TOKEN_LITERAL_CHAR,
    TOKEN_LITERAL_INT,
    TOKEN_LITERAL_FLOAT,
} TokenType;

// Token structure
typedef struct {
    TokenType type;
    const char *lexeme;
    size_t length;
} Token;

typedef struct {
    Token *tokens;
    size_t count;
    size_t capacity;
} TokenList;

// Lexer functions
const char* token_type_to_string(TokenType type);
void lexer_init(const char *source_code);
Token lexer_next_token(int debug);

#endif // OWLYLEXER_H