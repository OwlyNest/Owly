
#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "owlylexer.h"

typedef struct {
    Token *tokens;
    size_t count;
    size_t pos;
} Parser;

void parser_init(Parser *parser, Token *tokens, size_t count);

ASTNode *parse_program(Parser *parser);

#endif
