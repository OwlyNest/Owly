#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "owlylexer.h"
#include "parser.h"
#include "memutils.h"

const char* name;

static FILE *out;
int debug = 0;

void scan(const char *source) {
    lexer_init(source);

    Token tok;
    do {
        tok = lexer_next_token(debug);
    } while (tok.type != TOKEN_EOF);
}

int main(int argc, char *argv[]) {
    FILE *f = fopen("list.tok", "w");
    fclose(f);
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

    scan(source);

    // Parser parser;
    parser_init();

    fclose(out);
    free(source);
    return 0;
}