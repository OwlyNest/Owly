#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"

void codegen(const ASTNode *node, FILE *out, int ident);

#endif