#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/*
    * Future Amity: "oh no, oh no no no" *every 2 seconds*
*/

#define MAX_LINE 256
#define MAX_ELEMENTS 16
#define MAX_ELEMENT_LEN 64
#define MAX_VARS 64

char list_vars[MAX_VARS][MAX_ELEMENT_LEN];
int list_var_count = 0;

int parse_list_elements(const char *content, char elements[MAX_ELEMENTS][MAX_ELEMENT_LEN]) {
    int count = 0;
    int i = 0, j = 0;

    while (content[i] && count < MAX_ELEMENTS) {
        while (isspace(content[i])) i++;

        if (content[i] == '"') {
            i++; 
            j = 0;
            while (content[i] && content[i] != '"' && j < MAX_ELEMENT_LEN - 1) {
                elements[count][j++] = content[i++];
            }
            elements[count][j] = '\0';
            if (content[i] == '"') i++; 
            count++;
        } else if (content[i] != ',' && content[i] != '\0') {
            j = 0;
            while (content[i] && content[i] != ',' && !isspace(content[i]) && j < MAX_ELEMENT_LEN - 1) {
                elements[count][j++] = content[i++];
            }
            elements[count][j] = '\0';
            count++;
        }

        while (content[i] && (content[i] == ',' || isspace(content[i]))) i++;
    }

    return count;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s input.owly output.c\n", argv[0]);
        return 1;
    }

    FILE *fin = fopen(argv[1], "r");
    if (!fin) {
        perror("Failed to open input file");
        return 1;
    }

    FILE *fout = fopen(argv[2], "w");
    if (!fout) {
        perror("Failed to open output file");
        fclose(fin);
        return 1;
    }

    fprintf(fout, "#include <stdio.h>\n\n");

    char line[MAX_LINE];
    int in_main = 0;
    int open_blocks = 0;
    int in_func = 0;

    while (fgets(line, sizeof(line), fin)) {
        char *trim = line;
        while (*trim == ' ' || *trim == '\t') trim++;


        if (strncmp(trim, "func ", 5) == 0) {
            char func_name[64] = {0};
            sscanf(trim + 5, "%63[^()](", func_name);

            if (strcmp(func_name, "main") == 0){
                in_main = 1;
                fprintf(fout, "\nint main() {\n");
            } else {
                fprintf(fout, "\nvoid %s() {\n", func_name);
                in_func = 1;
            }
            continue;
        }

        if (in_main || in_func) {
            if (strncmp(trim, "print(", 6) == 0) {
                char *start = strchr(trim, '(');
                char *end = strchr(trim, ')');

                if (start && end) {
                    char content[MAX_LINE];
                    strncpy(content, start + 1, end - start - 1);
                    content[end - start - 1] = '\0';

                    if (content[0] == '"') {
                        fprintf(fout, "    printf(%s\"\\n\");\n", content);
                    }
                    else if (strchr(content, '[') && strchr(content, ']')) {
                        fprintf(fout, "    printf(\"%%s\\n\", %s);\n", content);
                    }
                    else {
                        int is_list = 0;
                        for (int i = 0; i < list_var_count; i++) {
                            if (strcmp(content, list_vars[i]) == 0) {
                                is_list = 1;
                                break;
                            }
                        }

                        if (is_list) {
                            fprintf(fout,
                                "    printf(\"[\");\n"
                                "    for (int i = 0; %s[i] != NULL; i++) {\n"
                                "        printf(\"\\\"%%s\\\"\", %s[i]);\n"
                                "        if (%s[i + 1] != NULL) printf(\", \");\n"
                                "    }\n"
                                "    printf(\"]\\n\");\n", content, content, content);
                        } else {
                            fprintf(fout, "    printf(\"%%d\\n\", %s);\n", content);
                        }
                    }
                }
            }
            else if (strncmp(trim, "int ", 4) == 0 || strncmp(trim, "char ", 5) == 0) {
                fprintf(fout, "    %s", trim);
            }
            else if (strncmp(trim, "list ", 5) == 0) {
                char *eq = strchr(trim, '=');
                if (!eq) continue;

                int var_name_len = eq - (trim + 5);
                char var_name[32] = {0};
                strncpy(var_name, trim + 5, var_name_len);

                for (int k = var_name_len - 1; k >= 0; k--) {
                    if (var_name[k] == ' ' || var_name[k] == '\t') var_name[k] = '\0';
                    else break;
                }

                if (list_var_count < MAX_VARS) {
                    strncpy(list_vars[list_var_count++], var_name, MAX_ELEMENT_LEN - 1);
                }

                char *start = strchr(eq, '[');
                char *end = strchr(eq, ']');

                if (start && end) {
                    char content[MAX_LINE];
                    strncpy(content, start + 1, end - start - 1);
                    content[end - start - 1] = '\0';

                    char elements[MAX_ELEMENTS][MAX_ELEMENT_LEN];
                    int count = parse_list_elements(content, elements);
                    fprintf(fout, "    const char *%s[] = {", var_name);

                    for (int i = 0; i < count; i++) {
                        fprintf(fout, "\"%s\"", elements[i]);
                        if (i < count - 1) fprintf(fout, ", ");
                    }
                    fprintf(fout, ", NULL};\n");
                }
            }
            else if (strncmp(trim, "if (", 4) == 0) {
                char *start = strchr(trim, '(');
                char *end = strchr(trim, ')');

                if (start && end) {
                    char condition[MAX_LINE];
                    strncpy(condition, start + 1, end - start - 1);
                    condition[end - start - 1] = '\0';
                    fprintf(fout, "    if (%s) {\n", condition);
                    open_blocks++;
                }
            }
            else if (strncmp(trim, "else if (", 9) == 0) {
                char *start = strchr(trim, '(');
                char *end = strchr(trim, ')');

                if (start && end) {
                    char condition[MAX_LINE];
                    strncpy(condition, start + 1, end - start - 1);
                    condition[end - start - 1] = '\0';
                    fprintf(fout, "    else if (%s) {\n", condition);
                    open_blocks++;
                }
            }
            else if (strncmp(trim, "else", 4) == 0 && strstr(trim, "else if") == NULL) {
                fprintf(fout, "    else {\n");
                open_blocks++;
                continue;
            }
            else if (strncmp(trim, "for (", 5) == 0) {
                char *start = strchr(trim, '(');
                char *end = strchr(trim, ')');

                if (start && end) {
                    char header[MAX_LINE];
                    strncpy(header, start+1, end - start - 1);
                    header[end - start - 1] = '\0';

                    fprintf(fout, "    for (%s) {\n", header);
                    open_blocks++;
                }

                continue;
            }
            else if (strncmp(trim, "while (", 7) == 0) {
                char *start = strchr(trim, '(');
                char *end = strchr(trim, ')');

                if (start && end) {
                    char condition[MAX_LINE];
                    strncpy(condition, start + 1, end -start - 1);
                    condition[end - start - 1] = '\0';

                    fprintf(fout, "    while (%s){\n", condition);
                    open_blocks++;
                }
                continue;
            }
            else if (strncmp(trim, "}", 1) == 0) {
                if (open_blocks > 0) {
                    fprintf(fout, "    }\n");
                    open_blocks--;
                } else {
                    if (in_main) {
                        fprintf(fout, "    return 0;\n}\n");
                        in_main = 0;
                    } else if (in_func) {
                        fprintf(fout, "}\n");
                        in_func = 0;
                    }
                }
            }
            else {
                // For any other line inside main, just print as is
                // Future Amity: What the FUCK!!!
                fprintf(fout, "    %s", trim);
            }
        } else if (strchr(trim, '(') && strchr(trim, ')') && strchr(trim, ';')) {
            fprintf(fout, "    %s\n", trim);
            continue;
        }
    }

    if (in_main){
        while (open_blocks-- > 0) {
            fprintf(fout, "    }\n");
        }
        fprintf(fout, "    return 0;\n}\n");
    }
    fclose(fin);
    fclose(fout);
    return 0;
}