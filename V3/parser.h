
#ifndef PARSER_H
#define PARSER_H

#include "owlylexer.h"

typedef struct {
    Token *tokens;
    size_t count;
    size_t pos;
} Parser;

void parser_init(void);

#endif
