#include <stdio.h>
#include <string.h>
#include "ast.h"
#include "codegen.h"

static void emit_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) {
        fputc(' ', out);
    }
}

void codegen(const ASTNode *node, FILE *out, int indent) {
    if (!node) return;

    switch (node->type) {
    case AST_PROGRAM:
        fprintf(out, "int main(void) {\n");
        for (size_t i = 0; i < node->program.count; i++) {
            codegen(node->program.statements[i], out, 4);
        }
        fprintf(out, "    return 0;\n}\n");
        break;

    case AST_FUNCTION:
        if (strcmp(node->function.name, "main") == 0) {
            for (size_t i = 0; i < node->function.count; i++) {
                codegen(node->function.body[i], out, 4);
            }
            break;
        }
        fprintf(out, "void %s(void) {\n", node->function.name);
        for (size_t i = 0; i < node->function.count; i++) {
            codegen(node->function.body[i], out, 4);
        }
        fprintf(out, "}\n");
        break;

    case AST_PRINT:
        emit_indent(out, indent);
        fprintf(out, "printf(");
        codegen(node->print.value, out, 0);
        fprintf(out, ");\n");
        break;

    case AST_LITERAL_STRING:
        fprintf(out, "\"%s\"", node->literal_string.value);
        break;

    case AST_LITERAL_NUMBER:
        fprintf(out, "%s", node->literal_number.value);
        break;

    case AST_IDENTIFIER:
        fprintf(out, "%s", node->identifier.name);
        break;

    case AST_VAR_DECL:
        emit_indent(out, indent);
        // Simple version: assume all ints for now
        fprintf(out, "int %s", node->var_decl.name);
        if (node->var_decl.value) {
            fprintf(out, " = ");
            codegen(node->var_decl.value, out, 0);
        }
        fprintf(out, ";\n");
        break;

    case AST_IF:
        emit_indent(out, indent);
        fprintf(out, "if (");
        codegen(node->if_stmt.condition, out, 0);
        fprintf(out, ") {\n");
        for (size_t i = 0; i < node->if_stmt.count; i++) {
            codegen(node->if_stmt.body[i], out, indent + 4);
        }
        emit_indent(out, indent);
        fprintf(out, "}");

        for (size_t i = 0; i < node->if_stmt.elif_count; i++) {
            fprintf(out, " else if (");
            codegen(node->if_stmt.elif_conditions[i], out, 0);
            fprintf(out, ") {\n");
            for (size_t j = 0; j < node->if_stmt.elif_body_counts[i]; j++) {
                codegen(node->if_stmt.elif_bodies[i][j], out, indent + 4);
            }
            emit_indent(out, indent);
            fprintf(out, "}");
        }

        if (node->if_stmt.else_count > 0) {
            fprintf(out, " else {\n");
            for (size_t i = 0; i < node->if_stmt.else_count; i++) {
                codegen(node->if_stmt.else_body[i], out, indent + 4);
            }
            emit_indent(out, indent);
            fprintf(out, "}");
        }
        fprintf(out, "\n");
        break;

    case AST_WHILE:
        emit_indent(out, indent);
        fprintf(out, "while (");
        codegen(node->while_stmt.condition, out, 0);
        fprintf(out, ") {\n");
        for (size_t i = 0; i < node->while_stmt.count; i++) {
            codegen(node->while_stmt.body[i], out, indent + 4);
        }
        emit_indent(out, indent);
        fprintf(out, "}\n");
        break;

    case AST_FOR:
        emit_indent(out, indent);
        fprintf(out, "for (");
        codegen(node->for_stmt.init, out, 0);
        // strip semicolon added by decl
        fprintf(out, " ");
        codegen(node->for_stmt.condition, out, 0);
        fprintf(out, "; ");
        codegen(node->for_stmt.post, out, 0);
        fprintf(out, ") {\n");
        for (size_t i = 0; i < node->for_stmt.count; i++) {
            codegen(node->for_stmt.body[i], out, indent + 4);
        }
        emit_indent(out, indent);
        fprintf(out, "}\n");
        break;

    case AST_BINARY_EXPR:
        codegen(node->binary_expr.left, out, 0);
        fprintf(out, " %s ", node->binary_expr.op);
        codegen(node->binary_expr.right, out, 0);
        break;

    case AST_UNARY_EXPR:
        if (strcmp(node->unary_expr.op, "++") == 0 || strcmp(node->unary_expr.op, "--") == 0) {
            codegen(node->unary_expr.operand, out, indent);
            fprintf(out, "%s", node->unary_expr.op);
        } else {
            fprintf(out, "%s", node->unary_expr.op);
            codegen(node->unary_expr.operand, out, indent); 
        }
        fprintf(out, ";\n");
        break;

    default:
        emit_indent(out, indent);
        fprintf(out, "/* Unhandled node type %d */\n", node->type);
        break;
    }
}
