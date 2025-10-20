
#ifndef OWLYLEXER_H
#define OWLYLEXER_H

#include <stddef.h>
#include <stdbool.h>

// Token types enumeration
typedef enum {
    TOKEN_EOF,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_SYMBOL,
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_FOR,
    TOKEN_WHILE,

    TOKEN_KEYWORD_FUNC,
    TOKEN_KEYWORD_PRINT,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_LIST,
    TOKEN_KEYWORD_ARRAY,

    TOKEN_OPERATOR_EQ,    //==
    TOKEN_OPERATOR_NEQ,   //!=
    TOKEN_OPERATOR_LT,    //<
    TOKEN_OPERATOR_LTE,   //<=
    TOKEN_OPERATOR_GT,    //>
    TOKEN_OPERATOR_GTE,   //>=

    TOKEN_INC,             //++
    TOKEN_DEC              //--

} TokenType;

// Token structure
typedef struct {
    TokenType type;
    const char *lexeme;  // Pointer into source text (not malloc'd)
    size_t length;       // Length of the lexeme substring
} Token;

// Lexer functions
void lexer_init(const char *source_code);
void lexer_cleanup(void);
Token lexer_next_token(int debug);

#endif // OWLYLEXER_H