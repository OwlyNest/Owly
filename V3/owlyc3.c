#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "owlylexer.h"
#include "ast.h"
#include "parser.h"
#include "memutils.h"
#include "codegen.h"

const char* name;

static FILE *out;
int debug = 0;

TokenList scan(const char *source) {
    lexer_init(source);

    TokenList list = {
        .tokens = xmalloc(16 * sizeof(Token)),
        .count = 0,
        .capacity = 16
    };

    Token tok;
    do {
        tok = lexer_next_token(debug);

        if (list.count >= list.capacity) {
            list.capacity *= 2;
            list.tokens = xrealloc(list.tokens, list.capacity * sizeof(Token));
        }

        list.tokens[list.count++] = tok;
    } while (tok.type != TOKEN_EOF);

    lexer_cleanup();
    return list;
}

int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s input.owly output.c -d\n", argv[0]);
        return 1;
    }
    name = argv[1];

    FILE *fin = fopen(name, "rb");
    if (!fin) {
        perror("Failed to open input file");
        return 1;
    }

    fseek(fin, 0, SEEK_END);
    long length = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    char *source = xmalloc(length + 1);
    fread(source, 1, length, fin);
    source[length] = '\0';
    fclose(fin);

    out = fopen(argv[2], "w");

    if (argc == 4 && strcmp(argv[3], "-d") == 0) {
        debug = 1;
    }

    TokenList token_list = scan(source);

    Parser parser;
    parser_init(&parser, token_list.tokens, token_list.count);

    ASTNode *ast = parse_program(&parser);
    if (!ast) {
        fprintf(stderr, "Failed to parse input file\n");
        free(source);
        free(token_list.tokens);
        return 1;
    }

    if (debug) {
        ast_print(ast);
    }

    fprintf(out, "#include <stdio.h>\n");
    codegen(ast, out, 0);

    ast_free(ast);
    
    fclose(out);
    free(source);
    return 0;
}