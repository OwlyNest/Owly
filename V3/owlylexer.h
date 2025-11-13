#ifndef OWLYLEXER_H
#define OWLYLEXER_H

#include <stddef.h>
#include <stdbool.h>

typedef enum TokenType {
    TOKEN_EOF,
    TOKEN_UNKNOWN,

    TOKEN_KEYWORD_AUTO,                // done
    TOKEN_KEYWORD_BREAK,               // not done
    TOKEN_KEYWORD_CASE,                // not done
    TOKEN_KEYWORD_CHAR,                // done
    TOKEN_KEYWORD_CONST,               // done
    TOKEN_KEYWORD_CONTINUE,            // not done
    TOKEN_KEYWORD_DEFAULT,             // not done
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
    TOKEN_KEYWORD_SIZEOF,              // not done
    TOKEN_KEYWORD_STATIC,              // done
    TOKEN_KEYWORD_STRUCT,              // done
    TOKEN_KEYWORD_SWITCH,              // not done
    TOKEN_KEYWORD_TYPEDEF,             // not done
    TOKEN_KEYWORD_UNION,               // not done
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

// Lexer functions
const char* token_type_to_string(TokenType type);
void lexer_init(const char *source_code);
Token lexer_next_token(int debug);

#endif // OWLYLEXER_H