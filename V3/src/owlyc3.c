/*
	* src/owlyc3.c - The main driver. Owly wakes up here, reads your code, and tries not to judge too hard.
	* Author:   Amity
	* Date:     Tue Feb 17 09:50:44 2026
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

/* --- Includes ---*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdx/stdx_json_to_tree.h>

#include "front/lexer.h"
#include "front/parser.h"
#include "middle/SA.h"
#include "middle/ir.h"
#include "memutils.h"
/* --- Typedefs - Structs - Enums ---*/

/* --- Globals ---*/
const char* name;
int debug = 0;
/* --- Prototypes ---*/
int main(int argc, char *argv[]);
void spill_debug();
/* --- Main ---*/
int main(int argc, char *argv[]) {
    FILE *f = fopen("out/list.tok", "w");
    fclose(f);
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s <input.owly> [-d]\n", argv[0]);
        fprintf(stderr, "  Owly demands a file to hoot at. Optional -d for maximum drama.\n");
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

    if (argc == 3 && strcmp(argv[2], "-d") == 0) {
        debug = 1;
		fprintf(stderr, "[Owly] Debug mode activated. Prepare for verbose hooting.\n");
    }

    // Lexer
    scan(source);

    // Parser
    Node *ast = parser_init();

    // Semantic Analyzer
    SemanticContext *ctx = analyze_semantics(ast);

    // IR
    IRModule *ir = generate_ir(ast, ctx);

    /* Debug output only in main, no stray prints in different files*/
    spill_debug();

    int result = ctx->error_count > 0 ? 1 : 0;

    /* Free everything right here */
    free_parser(ast);
    free_semantic_context(ctx);
    ir_free_module(ir);
    xfree(source);
    
    return result;
}
/* --- Functions ---*/

void spill_debug() {
    if (!debug) {
        puts("[Owly]: Silent mode engaged. Check out/ for the juicy bits. Hoot!");
        return;
    }

    puts("\n[Owly] AST tree incoming...\n");
    json_file_to_tree("[AST]", "out/ast.json", stdout);

    puts("\n[Owly] Symbol table hoot...\n");
    json_file_to_tree("[ST]", "out/symbols.json", stdout);

    puts("\n[Owly] Raw IR; let's see what chaos we brewed:");
    if (system("cat out/ir.ir") != 0) {
        puts("[Owly] No IR file? The void stares back... 🪶");
    }

    putchar('\n');
}