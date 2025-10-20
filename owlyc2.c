#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "owlylexer.h"

#define MAX_LISTS 64
#define MAX_ARRAYS 64
#define MAX_VAR_LEN 128
#define MAX_FUNCTIONS 64

static FILE *out;
static Token current_token;
bool debug = false;

char list_vars[MAX_LISTS][MAX_VAR_LEN];
int list_var_count = 0;

char array_vars[MAX_ARRAYS][MAX_VAR_LEN];
int array_var_count = 0;

char func_names[MAX_FUNCTIONS][MAX_VAR_LEN];
int func_count = 0;

void do_next();

void register_function(const char *name) {
    if (func_count < MAX_FUNCTIONS) {
        strncpy(func_names[func_count], name, MAX_VAR_LEN - 1);
        func_names[func_count][MAX_VAR_LEN - 1] = '\0';
        func_count++;
    }
}

int is_array_var(const char *name) {
    for (int i = 0; i < array_var_count; i++) {
        if (strcmp(array_vars[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

int is_list_var(const char *name) {
    for (int i = 0; i < list_var_count; i++) {
        if (strcmp(list_vars[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

void error(const char *msg) {
    fprintf(stderr, "Error: %s at token '", msg);
    fwrite(current_token.lexeme, 1, current_token.length, stderr);
    fprintf(stderr, "'\n");
    exit(1);
}

void next_token() {
    current_token = lexer_next_token(debug);
}

void expect(TokenType type, const char *msg) {
    if (current_token.type != type) {
        error(msg);
    }
    next_token();
}
void parse_print_statement() {
    expect(TOKEN_KEYWORD_PRINT, "expected 'print'");
    expect(TOKEN_SYMBOL, "expected '(' after print");

    if (current_token.type == TOKEN_STRING) {
        fprintf(out, "    printf(\"%%s\\n\", \"%.*s\");\n",
            (int)current_token.length, current_token.lexeme);
        next_token();
    } else if (current_token.type == TOKEN_NUMBER) {
        fprintf(out, "    printf(\"%%d\\n\", %.*s);\n",
            (int)current_token.length, current_token.lexeme);
        next_token();
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        char varname[MAX_VAR_LEN];
        memset(varname, 0, sizeof(varname));
        int len = current_token.length < MAX_VAR_LEN - 1 ? current_token.length : MAX_VAR_LEN - 1;
        memcpy(varname, current_token.lexeme, len);
        varname[len] = '\0';

        if (is_list_var(varname)) {
            // Print list as a C string array
            fprintf(out,
                "    printf(\"[\");\n"
                "    for (int i = 0; %s[i] != NULL; i++) {\n"
                "        printf(\"\\\"%%s\\\"\", %s[i]);\n"
                "        if (%s[i + 1] != NULL) printf(\", \");\n"
                "    }\n"
                "    printf(\"]\\n\");\n", varname, varname, varname);
        } else if (is_array_var(varname)) {
            fprintf(out,
                "    printf(\"[\");\n"
                "    for (int i = 0; %s[i] != -1; i++) {\n"
                "        printf(\"%%d\", %s[i]);\n"
                "        if (%s[i + 1] != -1) printf(\", \");\n"
                "    }\n"
                "    printf(\"]\\n\");\n",
                varname, varname, varname);
        } else {
            fprintf(out, "    printf(\"%%d\\n\", %s);\n", varname);
        }
        next_token();
    } else {
        error("Invalid expression inside print()");
    }

    expect(TOKEN_SYMBOL, "expected ')' after print argument");
}

void parse_int_declaration() {
    expect(TOKEN_KEYWORD_INT, "expected 'int'");
    if (current_token.type != TOKEN_IDENTIFIER) {
        error("expected identifier after int");
    }
    char varname[128] = {0};
    memset(varname, 0, sizeof(varname));
    int len = current_token.length < MAX_VAR_LEN - 1 ? current_token.length : MAX_VAR_LEN - 1;
    memcpy(varname, current_token.lexeme, len);
    varname[len] = '\0';
    next_token();

    expect(TOKEN_SYMBOL, "expected '=' after variable name");

    if (current_token.type != TOKEN_NUMBER && current_token.type != TOKEN_IDENTIFIER) {
        error("expected number or identifier after '='");
    }

    fprintf(out, "    int %s = %.*s;\n", varname, (int)current_token.length, current_token.lexeme);
    next_token();
}

void parse_list_declaration() {
    expect(TOKEN_KEYWORD_LIST, "expected 'list'");
    if (current_token.type != TOKEN_IDENTIFIER) {
        error("expected identifier after list");
    }

    char varname[MAX_VAR_LEN] = {0};
    memset(varname, 0, sizeof(varname));
    int len = current_token.length < MAX_VAR_LEN - 1 ? current_token.length : MAX_VAR_LEN - 1;
    memcpy(varname, current_token.lexeme, len);
    varname[len] = '\0';
    next_token();

    expect(TOKEN_SYMBOL, "expected '=' after list variable name");
    // Expect '[' symbol to start list elements
    if (current_token.lexeme[0] != '[') {
        error("expected '[' to start list elements");
    }
    next_token();

    // Collect elements as strings until ']'
    char elements[64][MAX_VAR_LEN];
    int count = 0;

    while (current_token.type != TOKEN_SYMBOL || current_token.lexeme[0] != ']') {
        if (current_token.type != TOKEN_STRING) {
            error("expected string elements inside list");
        }
        if (count >= 64) {
            error("list too large");
        }
        snprintf(elements[count], MAX_VAR_LEN, "%.*s", (int)current_token.length, current_token.lexeme);
        count++;

        next_token();

        // If next token is ',' then skip it
        if (current_token.type == TOKEN_SYMBOL && current_token.lexeme[0] == ',') {
            next_token();
        }
    }

    // Consume the closing ']'
    if (current_token.lexeme[0] != ']') {
        error("expected ']' at end of list elements");
    }
    next_token();

    // Add variable to list registry
    if (list_var_count < MAX_LISTS) {
        strncpy(list_vars[list_var_count++], varname, MAX_VAR_LEN - 1);
    } else {
        error("too many lists declared");
    }

    // Output C array definition for the list
    fprintf(out, "    const char *%s[] = {", varname);
    for (int i = 0; i < count; i++) {
        fprintf(out, "\"%s\"", elements[i]);
        if (i < count - 1) fprintf(out, ", ");
    }
    fprintf(out, ", NULL};\n");
}

void parse_array_declaration() {
    expect(TOKEN_KEYWORD_ARRAY, "expected array");
    if (current_token.type != TOKEN_IDENTIFIER) {
        error("expected identifier after array");
    }

    char varname[MAX_VAR_LEN] = {0};
    memset(varname, 0, sizeof(varname));
    int len = current_token.length < MAX_VAR_LEN - 1 ? current_token.length : MAX_VAR_LEN - 1;
    memcpy(varname, current_token.lexeme, len);
    varname[len] = '\0';
    next_token();

    expect(TOKEN_SYMBOL, "expected = after array variable name");

    if (current_token.lexeme[0] != '[') {
        error("Expected '[' to start array elements");
    }
    next_token();

    int elements[64];
    int count = 0;

    while (current_token.type != TOKEN_SYMBOL || current_token.lexeme[0] != ']') {
        if (current_token.type != TOKEN_NUMBER) {
            error("expected number elements inside array");
        }
        if (count >= 64) {
            error("array too large");
        }
        elements[count++] = atoi(current_token.lexeme);
        next_token();

        if (current_token.type == TOKEN_SYMBOL && current_token.lexeme[0] == ',') {
            next_token();
        }
    }
    if (current_token.lexeme[0] != ']') {
        error("expected ']' to close array elements");
    }
    next_token();

    fprintf(out, "    int %s[] = {", varname);
    for (int i = 0; i < count; i++) {
        fprintf(out, "%d", elements[i]);
        if (i < count - 1) fprintf(out, ",");
    }
    fprintf(out, ", -1};\n");

    if (array_var_count < MAX_ARRAYS) {
        strncpy(array_vars[array_var_count++], varname, MAX_VAR_LEN - 1);
    } else {
        error("too many arrays declared");
    }
}

void parse_condition_expression() {
    fprintf(out, "(");

    // First token must be identifier or number
    if (current_token.type != TOKEN_IDENTIFIER && current_token.type != TOKEN_NUMBER) {
        error("expected identifier or number in condition");
    }

    fprintf(out, "%.*s", (int)current_token.length, current_token.lexeme);

    // Save for possible postfix use
    int is_postfix_target = (current_token.type == TOKEN_IDENTIFIER);
    next_token();

    // Handle postfix increment/decrement (e.g., i++ or i--)
    if (is_postfix_target &&
        (current_token.type == TOKEN_INC || current_token.type == TOKEN_DEC)) {

        if (current_token.type == TOKEN_INC) {
            fprintf(out, "++");
        } else if (current_token.type == TOKEN_DEC) {
            fprintf(out, "--");
        }

        next_token();
        fprintf(out, ")");
        return;
    }

    // Otherwise, expect a comparison operator
    switch (current_token.type) {
        case TOKEN_OPERATOR_EQ:    fprintf(out, " == "); break;
        case TOKEN_OPERATOR_NEQ:   fprintf(out, " != "); break;
        case TOKEN_OPERATOR_LT:    fprintf(out, " < "); break;
        case TOKEN_OPERATOR_LTE:   fprintf(out, " <= "); break;
        case TOKEN_OPERATOR_GT:    fprintf(out, " > "); break;
        case TOKEN_OPERATOR_GTE:   fprintf(out, " >= "); break;
        default:
            error("expected comparison operator in condition");
    }

    next_token();

    // Expect right-hand side of condition
    if (current_token.type != TOKEN_IDENTIFIER && current_token.type != TOKEN_NUMBER) {
        error("expected identifier or number after comparison operator");
    }

    fprintf(out, "%.*s)", (int)current_token.length, current_token.lexeme);
    next_token();
}

void parse_while_statement() {
    expect(TOKEN_WHILE, "expected while");
    expect(TOKEN_SYMBOL, "expected '(' after while");

    fprintf(out, "    while(");
    parse_condition_expression();
    expect(TOKEN_SYMBOL, "expected ')' after if condition");
    expect(TOKEN_SYMBOL, "expected '{' to start if block");

    fprintf(out, ") {\n");

    while (current_token.type != TOKEN_SYMBOL || current_token.lexeme[0] != '}') {
        do_next();
    }
    expect(TOKEN_SYMBOL, "Expected '}' to close if block");
    fprintf(out, "    }\n");
}
void parse_if_statement() {
    expect(TOKEN_IF, "expected if");
    expect(TOKEN_SYMBOL, "expected '(' after if");

    fprintf(out, "    if(");
    parse_condition_expression();
    expect(TOKEN_SYMBOL, "expected ')' after if condition");
    expect(TOKEN_SYMBOL, "expected '{' to start if block");

    fprintf(out, ") {\n");

    while (current_token.type != TOKEN_SYMBOL || current_token.lexeme[0] != '}') {
        do_next();
    }
    expect(TOKEN_SYMBOL, "Expected '}' to close if block");
    fprintf(out, "    }");

    while (current_token.type == TOKEN_ELIF || current_token.type == TOKEN_ELSE) {
        if (current_token.type == TOKEN_ELIF) {
            next_token();
            expect(TOKEN_SYMBOL, "expected '(' after elif");

            fprintf(out, " else if(");
            parse_condition_expression();
            expect(TOKEN_SYMBOL, "expected ')' after elif condition");
            expect(TOKEN_SYMBOL, "expected '{' to start elif block");

            fprintf(out, ") {\n");

            while (current_token.type != TOKEN_SYMBOL || current_token.lexeme[0] != '}') {
                do_next();
            }
            expect(TOKEN_SYMBOL, "expected '}' to close elif block");
            fprintf(out, "    }");

        } else if (current_token.type == TOKEN_ELSE) {
            next_token();
            expect(TOKEN_SYMBOL, "expected '{' after 'else'");
            fprintf(out, " else {\n");

            while (current_token.type != TOKEN_SYMBOL || current_token.lexeme[0] != '}') {
                do_next();
            }
            expect(TOKEN_SYMBOL, "expected '}' to close else block");
            fprintf(out, "    }");
        }
    }

    fprintf(out, "\n");
}

void parse_for_statement() {
    expect(TOKEN_FOR, "expected 'for'");
    expect(TOKEN_SYMBOL, "expected '(' after 'for'");

    fprintf(out, "    for (");

    // Instead of calling parse_int_declaration(), parse initializer inline:
    if (current_token.type == TOKEN_KEYWORD_INT) {
        // parse "int i = 0" manually here, without consuming semicolon
        next_token(); // consume 'int'

        if (current_token.type != TOKEN_IDENTIFIER) {
            error("expected identifier after int in for loop initializer");
        }
        char varname[MAX_VAR_LEN] = {0};
        int len = current_token.length < MAX_VAR_LEN - 1 ? current_token.length : MAX_VAR_LEN - 1;
        memcpy(varname, current_token.lexeme, len);
        varname[len] = '\0';
        next_token();

        expect(TOKEN_SYMBOL, "expected '=' after variable name in for loop initializer");
        if (current_token.type != TOKEN_NUMBER && current_token.type != TOKEN_IDENTIFIER) {
            error("expected number or identifier after '=' in for loop initializer");
        }

        fprintf(out, "int %s = %.*s; ", varname, (int)current_token.length, current_token.lexeme);
        next_token();
    } else if (current_token.type == TOKEN_SYMBOL && current_token.lexeme[0] == ';') {
        // Allow empty initializer
        fprintf(out, "; ");
        next_token();
    } else {
        error("unexpected token in for loop initializer");
    }
    next_token();
    // Parse condition expression
    if (current_token.type == TOKEN_SYMBOL && current_token.lexeme[0] == ';') {
        // Empty condition is allowed: e.g., for (; i < 10; i++)
        fprintf(out, "; ");
        next_token();
    } else {
        parse_condition_expression();
        expect(TOKEN_SYMBOL, "expected ';' after for loop condition");
    }

    fprintf(out, "; ");

    // Parse increment expression
    parse_condition_expression();

    expect(TOKEN_SYMBOL, "expected ')' after for loop");
    fprintf(out, ") {\n");

    expect(TOKEN_SYMBOL, "expected '{' to start for block");

    while (current_token.type != TOKEN_SYMBOL || current_token.lexeme[0] != '}') {
        do_next();
    }
    expect(TOKEN_SYMBOL, "expected '}' to close for block");
    fprintf(out, "    }\n");
}

void parse_expression() {
    if (current_token.type == TOKEN_IDENTIFIER) {
        char varname[MAX_VAR_LEN] = {0};
        int len = current_token.length < MAX_VAR_LEN - 1 ? current_token.length : MAX_VAR_LEN - 1;
        memcpy(varname, current_token.lexeme, len);
        varname[len] = '\0';

        next_token();

        if (current_token.type == TOKEN_SYMBOL && current_token.lexeme[0] == '(') {
            // function call
            next_token(); // skip '('
            expect(TOKEN_SYMBOL, "expected ')' after function call");
            fprintf(out, "    %s();\n", varname);
            return;
        }

        fprintf(out, "    %s", varname);

        if (current_token.type == TOKEN_INC) {
            fprintf(out, "++;\n");
            next_token();
            return;
        } else if (current_token.type == TOKEN_DEC) {
            fprintf(out, "--;\n");
            next_token();
            return;
        }

        if (current_token.type == TOKEN_SYMBOL && current_token.lexeme[0] == '=') {
            fprintf(out, " = ");
            next_token();

            if (current_token.type == TOKEN_IDENTIFIER || current_token.type == TOKEN_NUMBER) {
                fprintf(out, "%.*s", (int)current_token.length, current_token.lexeme);
                next_token();

                if (current_token.type == TOKEN_SYMBOL &&
                    (current_token.lexeme[0] == '+' || current_token.lexeme[0] == '-')) {
                    fprintf(out, " %c ", current_token.lexeme[0]);
                    next_token();

                    if (current_token.type != TOKEN_NUMBER) {
                        error("expected number after '+' or '-'");
                    }

                    fprintf(out, "%.*s", (int)current_token.length, current_token.lexeme);
                    next_token();
                }

                fprintf(out, ";\n");
            } else {
                error("expected identifier or number after '='");
            }

            return;
        }

        error("expected '++', '--', '=', or '(' after identifier");
    } else {
        error("expected identifier");
    }
}

void parse_function_body() {
    expect(TOKEN_SYMBOL, "expected '{' at function body start");
    while (!(current_token.type == TOKEN_SYMBOL && current_token.lexeme[0] == '}')) {
        do_next();
    }
    expect(TOKEN_SYMBOL, "expected '}' at function body end");
}

void parse_function() {
    expect(TOKEN_KEYWORD_FUNC, "expected 'func'");

    if (current_token.type != TOKEN_IDENTIFIER) {
        error("expected function name after func");
    }

    char funcname[128] = {0};
    snprintf(funcname, sizeof(funcname), "%.*s", (int)current_token.length, current_token.lexeme);
    next_token();

    expect(TOKEN_SYMBOL, "expected '(' after function name");
    expect(TOKEN_SYMBOL, "expected ')' after '('");

    if (strcmp(funcname, "main") == 0) {
        fprintf(out, "int main() {\n");
    } else {
        fprintf(out, "void %s() {\n", funcname);
    }

    parse_function_body();

    if (strcmp(funcname, "main") == 0) {
        fprintf(out, "    return 0;\n");
    }
    fprintf(out, "}\n\n");
}

void parse_program() {
    next_token();
    while (current_token.type != TOKEN_EOF) {
        if (current_token.type == TOKEN_KEYWORD_FUNC) {
            parse_function();
        } else {
            error("expected 'func' at top level");
        }
    }
}

void do_next() {
    if (current_token.type == TOKEN_KEYWORD_PRINT) {
        parse_print_statement();
    } else if (current_token.type == TOKEN_KEYWORD_INT) {
        parse_int_declaration();
    } else if (current_token.type == TOKEN_KEYWORD_LIST) {
        parse_list_declaration();
    } else if (current_token.type == TOKEN_KEYWORD_ARRAY) {
        parse_array_declaration();
    } else if (current_token.type == TOKEN_FOR ) {
        parse_for_statement();
    } else if (current_token.type == TOKEN_IF) {
        parse_if_statement();
    } else if (current_token.type == TOKEN_WHILE) {
        parse_while_statement();
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        parse_expression();
    } else {
        error("unexpected token");
    }
}

int main(int argc, char *argv[]) {
    list_var_count = 0;
    array_var_count = 0;
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s input.owly output.c -d\n", argv[0]);
        return 1;
    }

    FILE *fin = fopen(argv[1], "rb");
    if (!fin) {
        perror("Failed to open input file");
        return 1;
    }

    fseek(fin, 0, SEEK_END);
    long length = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    char *source = malloc(length + 1);
    if (!source) {
        perror("malloc failed");
        fclose(fin);
        return 1;
    }
    fread(source, 1, length, fin);
    source[length] = '\0';
    fclose(fin);

    out = fopen(argv[2], "w");
    if (!out) {
        perror("Failed to open output file");
        free(source);
        return 1;
    }
    if (argc == 4) {
        char *arg = argv[3];
        if (strcmp(arg, "-d") == 0) {
            debug = true;
        }
    }
    fprintf(out, "#include <stdio.h>\n\n");

    lexer_init(source);
    parse_program();
    lexer_cleanup();

    fclose(out);
    free(source);

    return 0;
}