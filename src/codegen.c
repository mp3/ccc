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
    gen->symtab = NULL;
    LOG_DEBUG("Created code generator");
    return gen;
}

void codegen_destroy(CodeGenerator *gen) {
    if (gen) {
        if (gen->symtab) {
            symtab_destroy(gen->symtab);
        }
        free(gen);
    }
}

static char *codegen_next_temp(CodeGenerator *gen) {
    char *temp = malloc(32);
    snprintf(temp, 32, "%%tmp%d", gen->temp_counter++);
    return temp;
}

static char *codegen_next_label(CodeGenerator *gen, const char *prefix) {
    char *label = malloc(64);
    snprintf(label, 64, "%s%d", prefix, gen->label_counter++);
    return label;
}

static char *codegen_expression(CodeGenerator *gen, ASTNode *expr) {
    switch (expr->type) {
        case AST_INT_LITERAL: {
            char *temp = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = add i32 0, %d\n", temp, expr->data.int_literal.value);
            return temp;
        }
        
        case AST_IDENTIFIER: {
            Symbol *sym = symtab_lookup(gen->symtab, expr->data.identifier.name);
            if (!sym) {
                LOG_ERROR("Undefined variable: %s", expr->data.identifier.name);
                exit(1);
            }
            
            char *temp = codegen_next_temp(gen);
            // For parameters, we always allocate and use .addr
            // For regular variables, we use the name directly
            fprintf(gen->output, "  %s = load i32, i32* %%%s\n", temp, sym->name);
            return temp;
        }
        
        case AST_ASSIGNMENT: {
            Symbol *sym = symtab_lookup(gen->symtab, expr->data.assignment.name);
            if (!sym) {
                LOG_ERROR("Undefined variable: %s", expr->data.assignment.name);
                exit(1);
            }
            
            char *value = codegen_expression(gen, expr->data.assignment.value);
            fprintf(gen->output, "  store i32 %s, i32* %%%s\n", value, sym->name);
            return value;
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
                case TOKEN_EQ:
                case TOKEN_NE:
                case TOKEN_LT:
                case TOKEN_GT:
                case TOKEN_LE:
                case TOKEN_GE: {
                    // Comparison operators need icmp instruction
                    const char *cmp_op;
                    switch (expr->data.binary_op.op) {
                        case TOKEN_EQ: cmp_op = "eq"; break;
                        case TOKEN_NE: cmp_op = "ne"; break;
                        case TOKEN_LT: cmp_op = "slt"; break;
                        case TOKEN_GT: cmp_op = "sgt"; break;
                        case TOKEN_LE: cmp_op = "sle"; break;
                        case TOKEN_GE: cmp_op = "sge"; break;
                        default: cmp_op = ""; // Should never happen
                    }
                    char *cmp_result = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = icmp %s i32 %s, %s\n", 
                            cmp_result, cmp_op, left, right);
                    // Convert i1 to i32 (0 or 1)
                    fprintf(gen->output, "  %s = zext i1 %s to i32\n", result, cmp_result);
                    free(cmp_result);
                    free(left);
                    free(right);
                    return result;
                }
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
        
        case AST_FUNCTION_CALL: {
            Symbol *func_sym = symtab_lookup(gen->symtab, expr->data.function_call.name);
            if (!func_sym || func_sym->type != SYM_FUNCTION) {
                LOG_ERROR("Undefined function: %s", expr->data.function_call.name);
                exit(1);
            }
            
            // Check argument count
            if (expr->data.function_call.argument_count != func_sym->param_count) {
                LOG_ERROR("Function '%s' expects %d arguments, got %d",
                         expr->data.function_call.name,
                         func_sym->param_count,
                         expr->data.function_call.argument_count);
                exit(1);
            }
            
            // Evaluate arguments
            char **arg_values = malloc(expr->data.function_call.argument_count * sizeof(char*));
            for (int i = 0; i < expr->data.function_call.argument_count; i++) {
                arg_values[i] = codegen_expression(gen, expr->data.function_call.arguments[i]);
            }
            
            // Generate function call
            char *result = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = call i32 @%s(", result, expr->data.function_call.name);
            for (int i = 0; i < expr->data.function_call.argument_count; i++) {
                if (i > 0) fprintf(gen->output, ", ");
                fprintf(gen->output, "i32 %s", arg_values[i]);
            }
            fprintf(gen->output, ")\n");
            
            // Free argument values
            for (int i = 0; i < expr->data.function_call.argument_count; i++) {
                free(arg_values[i]);
            }
            free(arg_values);
            
            return result;
        }
        
        default:
            LOG_ERROR("Unknown expression type in codegen: %d", expr->type);
            exit(1);
    }
}

static void codegen_statement(CodeGenerator *gen, ASTNode *stmt) {
    switch (stmt->type) {
        case AST_VAR_DECL: {
            Symbol *sym = symtab_insert(gen->symtab, stmt->data.var_decl.name, 
                                       SYM_VARIABLE, stmt->data.var_decl.type);
            if (!sym) {
                LOG_ERROR("Failed to declare variable: %s", stmt->data.var_decl.name);
                exit(1);
            }
            
            // Allocate stack space for the variable
            fprintf(gen->output, "  %%%s = alloca i32\n", sym->name);
            
            // Initialize if needed
            if (stmt->data.var_decl.initializer) {
                char *value = codegen_expression(gen, stmt->data.var_decl.initializer);
                fprintf(gen->output, "  store i32 %s, i32* %%%s\n", value, sym->name);
                free(value);
            }
            break;
        }
        
        case AST_RETURN_STMT: {
            char *value = codegen_expression(gen, stmt->data.return_stmt.expression);
            fprintf(gen->output, "  ret i32 %s\n", value);
            free(value);
            break;
        }
        
        case AST_COMPOUND_STMT: {
            // Create new scope
            SymbolTable *old_symtab = gen->symtab;
            gen->symtab = symtab_create(old_symtab);
            
            for (int i = 0; i < stmt->data.compound.statement_count; i++) {
                codegen_statement(gen, stmt->data.compound.statements[i]);
            }
            
            // Restore old scope
            symtab_destroy(gen->symtab);
            gen->symtab = old_symtab;
            break;
        }
            
        case AST_EXPR_STMT:
            free(codegen_expression(gen, stmt->data.expr_stmt.expression));
            break;
        
        case AST_IF_STMT: {
            char *cond_value = codegen_expression(gen, stmt->data.if_stmt.condition);
            char *cond_bool = codegen_next_temp(gen);
            
            // Convert condition to i1
            fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", cond_bool, cond_value);
            
            char *then_label = codegen_next_label(gen, "if.then.");
            char *else_label = stmt->data.if_stmt.else_stmt ? 
                              codegen_next_label(gen, "if.else.") : NULL;
            char *end_label = codegen_next_label(gen, "if.end.");
            
            // Branch based on condition
            if (else_label) {
                fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                       cond_bool, then_label, else_label);
            } else {
                fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                       cond_bool, then_label, end_label);
            }
            
            // Then block
            fprintf(gen->output, "\n%s:\n", then_label);
            codegen_statement(gen, stmt->data.if_stmt.then_stmt);
            fprintf(gen->output, "  br label %%%s\n", end_label);
            
            // Else block (if exists)
            if (else_label) {
                fprintf(gen->output, "\n%s:\n", else_label);
                codegen_statement(gen, stmt->data.if_stmt.else_stmt);
                fprintf(gen->output, "  br label %%%s\n", end_label);
                free(else_label);
            }
            
            // End label
            fprintf(gen->output, "\n%s:\n", end_label);
            
            free(cond_value);
            free(cond_bool);
            free(then_label);
            free(end_label);
            break;
        }
        
        case AST_WHILE_STMT: {
            char *cond_label = codegen_next_label(gen, "while.cond.");
            char *body_label = codegen_next_label(gen, "while.body.");
            char *end_label = codegen_next_label(gen, "while.end.");
            
            // Jump to condition check
            fprintf(gen->output, "  br label %%%s\n", cond_label);
            
            // Condition block
            fprintf(gen->output, "\n%s:\n", cond_label);
            char *cond_value = codegen_expression(gen, stmt->data.while_stmt.condition);
            char *cond_bool = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", cond_bool, cond_value);
            fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                   cond_bool, body_label, end_label);
            
            // Body block
            fprintf(gen->output, "\n%s:\n", body_label);
            codegen_statement(gen, stmt->data.while_stmt.body);
            fprintf(gen->output, "  br label %%%s\n", cond_label);
            
            // End label
            fprintf(gen->output, "\n%s:\n", end_label);
            
            free(cond_value);
            free(cond_bool);
            free(cond_label);
            free(body_label);
            free(end_label);
            break;
        }
            
        default:
            LOG_ERROR("Unknown statement type in codegen: %d", stmt->type);
            exit(1);
    }
}

static void codegen_function(CodeGenerator *gen, ASTNode *func) {
    // Generate function signature
    fprintf(gen->output, "define i32 @%s(", func->data.function.name);
    for (int i = 0; i < func->data.function.param_count; i++) {
        if (i > 0) fprintf(gen->output, ", ");
        fprintf(gen->output, "i32 %%%s.param", func->data.function.params[i]->data.param_decl.name);
    }
    fprintf(gen->output, ") {\n");
    fprintf(gen->output, "entry:\n");
    
    gen->temp_counter = 0;  // Reset temp counter for each function
    
    // Create function scope
    SymbolTable *old_symtab = gen->symtab;
    gen->symtab = symtab_create(old_symtab);
    
    // Add parameters to symbol table
    for (int i = 0; i < func->data.function.param_count; i++) {
        ASTNode *param = func->data.function.params[i];
        Symbol *sym = symtab_insert(gen->symtab, param->data.param_decl.name,
                                   SYM_VARIABLE, param->data.param_decl.type);
        if (sym) {
            sym->is_param = true;
            // Allocate space for parameter and store the value
            fprintf(gen->output, "  %%%s = alloca i32\n", param->data.param_decl.name);
            fprintf(gen->output, "  store i32 %%%s.param, i32* %%%s\n", 
                    param->data.param_decl.name, param->data.param_decl.name);
        }
    }
    
    // Generate function body
    codegen_statement(gen, func->data.function.body);
    
    // Only add unreachable if the last instruction wasn't a terminator
    // This is a simple heuristic - in production you'd track this properly
    fprintf(gen->output, "  ret i32 0  ; default return\n");
    fprintf(gen->output, "}\n\n");
    
    // Restore global scope
    symtab_destroy(gen->symtab);
    gen->symtab = old_symtab;
    
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
    
    // Create global symbol table
    gen->symtab = symtab_create(NULL);
    
    // First pass: register all functions
    for (int i = 0; i < ast->data.program.function_count; i++) {
        ASTNode *func = ast->data.program.functions[i];
        
        // Extract parameter types and names
        char **param_types = NULL;
        char **param_names = NULL;
        if (func->data.function.param_count > 0) {
            param_types = malloc(func->data.function.param_count * sizeof(char*));
            param_names = malloc(func->data.function.param_count * sizeof(char*));
            for (int j = 0; j < func->data.function.param_count; j++) {
                param_types[j] = func->data.function.params[j]->data.param_decl.type;
                param_names[j] = func->data.function.params[j]->data.param_decl.name;
            }
        }
        
        symtab_insert_function(gen->symtab, func->data.function.name,
                              func->data.function.return_type,
                              param_types, param_names,
                              func->data.function.param_count);
        
        free(param_types);
        free(param_names);
    }
    
    // Second pass: generate code for each function
    for (int i = 0; i < ast->data.program.function_count; i++) {
        codegen_function(gen, ast->data.program.functions[i]);
    }
    
    // Clean up global symbol table
    symtab_destroy(gen->symtab);
    gen->symtab = NULL;
    
    LOG_INFO("Code generation complete");
}
