#include "codegen.h"
#include "logger.h"
#include "lexer.h"
#include <stdlib.h>
#include <string.h>

CodeGenerator *codegen_create(FILE *output) {
    CodeGenerator *gen = malloc(sizeof(CodeGenerator));
    gen->output = output;
    gen->temp_counter = 0;
    gen->label_counter = 0;
    LOG_DEBUG("Created code generator");
    return gen;
}

void codegen_destroy(CodeGenerator *gen) {
    if (gen) {
        free(gen);
    }
}

static char *codegen_next_temp(CodeGenerator *gen) {
    char *temp = malloc(32);
    snprintf(temp, 32, "%%tmp%d", gen->temp_counter++);
    return temp;
}

static char *codegen_expression(CodeGenerator *gen, ASTNode *expr) {
    switch (expr->type) {
        case AST_INT_LITERAL: {
            char *temp = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = add i32 0, %d\n", temp, expr->data.int_literal.value);
            return temp;
        }
        
        case AST_BINARY_OP: {
            char *left = codegen_expression(gen, expr->data.binary_op.left);
            char *right = codegen_expression(gen, expr->data.binary_op.right);
            char *result = codegen_next_temp(gen);
            
            const char *op;
            switch (expr->data.binary_op.op) {
                case TOKEN_PLUS: op = "add"; break;
                case TOKEN_MINUS: op = "sub"; break;
                case TOKEN_STAR: op = "mul"; break;
                case TOKEN_SLASH: op = "sdiv"; break;
                default:
                    LOG_ERROR("Unknown binary operator: %d (%s)", 
                             expr->data.binary_op.op,
                             token_type_to_string(expr->data.binary_op.op));
                    exit(1);
            }
            
            fprintf(gen->output, "  %s = %s i32 %s, %s\n", result, op, left, right);
            free(left);
            free(right);
            return result;
        }
        
        default:
            LOG_ERROR("Unknown expression type in codegen: %d", expr->type);
            exit(1);
    }
}

static void codegen_statement(CodeGenerator *gen, ASTNode *stmt) {
    switch (stmt->type) {
        case AST_RETURN_STMT: {
            char *value = codegen_expression(gen, stmt->data.return_stmt.expression);
            fprintf(gen->output, "  ret i32 %s\n", value);
            free(value);
            break;
        }
        
        case AST_COMPOUND_STMT:
            for (int i = 0; i < stmt->data.compound.statement_count; i++) {
                codegen_statement(gen, stmt->data.compound.statements[i]);
            }
            break;
            
        case AST_EXPR_STMT:
            free(codegen_expression(gen, stmt->data.expr_stmt.expression));
            break;
            
        default:
            LOG_ERROR("Unknown statement type in codegen: %d", stmt->type);
            exit(1);
    }
}

static void codegen_function(CodeGenerator *gen, ASTNode *func) {
    fprintf(gen->output, "define i32 @%s() {\n", func->data.function.name);
    fprintf(gen->output, "entry:\n");
    
    gen->temp_counter = 0;  // Reset temp counter for each function
    codegen_statement(gen, func->data.function.body);
    
    // Add unreachable in case no return statement
    fprintf(gen->output, "  unreachable\n");
    fprintf(gen->output, "}\n\n");
    
    LOG_DEBUG("Generated code for function: %s", func->data.function.name);
}

void codegen_generate(CodeGenerator *gen, ASTNode *ast) {
    if (ast->type != AST_PROGRAM) {
        LOG_ERROR("Expected program node at top level");
        exit(1);
    }
    
    // Generate LLVM IR header
    fprintf(gen->output, "; ModuleID = 'ccc_output'\n");
    fprintf(gen->output, "source_filename = \"ccc_output\"\n");
    fprintf(gen->output, "target datalayout = \"e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128\"\n");
    fprintf(gen->output, "target triple = \"x86_64-unknown-linux-gnu\"\n\n");
    
    // Generate each function
    for (int i = 0; i < ast->data.program.function_count; i++) {
        codegen_function(gen, ast->data.program.functions[i]);
    }
    
    LOG_INFO("Code generation complete");
}
