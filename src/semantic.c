#include "semantic.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

static void analyze_node(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_expression(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_statement(SemanticAnalyzer *analyzer, ASTNode *node);
static void analyze_function(SemanticAnalyzer *analyzer, ASTNode *node);

SemanticAnalyzer *semantic_create(ErrorManager *error_manager) {
    SemanticAnalyzer *analyzer = malloc(sizeof(SemanticAnalyzer));
    analyzer->error_manager = error_manager;
    analyzer->current_scope = NULL;
    analyzer->current_function = NULL;
    analyzer->has_return_stmt = false;
    return analyzer;
}

void semantic_destroy(SemanticAnalyzer *analyzer) {
    free(analyzer);
}

void semantic_analyze(SemanticAnalyzer *analyzer, ASTNode *ast) {
    if (!ast) return;
    
    // Create global scope
    analyzer->current_scope = symtab_create(NULL);
    
    // Analyze the program
    analyze_node(analyzer, ast);
    
    // Check for unused globals
    semantic_check_unused_variables(analyzer, analyzer->current_scope);
    
    // Clean up
    symtab_destroy(analyzer->current_scope);
}

static void analyze_node(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            // Analyze all functions
            for (int i = 0; i < node->data.program.function_count; i++) {
                analyze_function(analyzer, node->data.program.functions[i]);
            }
            break;
            
        case AST_FUNCTION:
            analyze_function(analyzer, node);
            break;
            
        default:
            analyze_statement(analyzer, node);
            break;
    }
}

static void analyze_function(SemanticAnalyzer *analyzer, ASTNode *node) {
    // Set current function context
    const char *prev_function = analyzer->current_function;
    analyzer->current_function = node->data.function.name;
    analyzer->has_return_stmt = false;
    
    LOG_DEBUG("Analyzing function: %s (returns %s)", 
              node->data.function.name, node->data.function.return_type);
    
    // Create function scope
    SymbolTable *prev_scope = analyzer->current_scope;
    analyzer->current_scope = symtab_create(prev_scope);
    
    // Add parameters to scope
    for (int i = 0; i < node->data.function.param_count; i++) {
        ASTNode *param = node->data.function.params[i];
        Symbol *sym = symtab_insert(analyzer->current_scope, 
                                   param->data.param_decl.name,
                                   SYM_VARIABLE,
                                   param->data.param_decl.type);
        if (sym) {
            sym->is_param = true;
            sym->is_initialized = true;  // Parameters are initialized
            sym->decl_line = param->line;
            sym->decl_column = param->column;
        }
    }
    
    // Analyze function body
    analyze_statement(analyzer, node->data.function.body);
    
    // Check for missing return statement (for non-void functions)
    LOG_DEBUG("Function %s: has_return=%d, return_type=%s", 
              node->data.function.name, analyzer->has_return_stmt, node->data.function.return_type);
    if (strcmp(node->data.function.return_type, "void") != 0 && !analyzer->has_return_stmt) {
        ErrorContext ctx = {
            .filename = NULL,
            .line = node->line,
            .column = node->column,
            .length = strlen(node->data.function.name)
        };
        warning_missing_return(analyzer->error_manager, &ctx, node->data.function.name);
    }
    
    // Check for unused variables
    semantic_check_unused_variables(analyzer, analyzer->current_scope);
    
    // Restore previous context
    symtab_destroy(analyzer->current_scope);
    analyzer->current_scope = prev_scope;
    analyzer->current_function = prev_function;
}

static void analyze_statement(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_COMPOUND_STMT: {
            // Create new scope for compound statement
            SymbolTable *prev_scope = analyzer->current_scope;
            analyzer->current_scope = symtab_create(prev_scope);
            
            // Analyze all statements
            for (int i = 0; i < node->data.compound.statement_count; i++) {
                analyze_statement(analyzer, node->data.compound.statements[i]);
            }
            
            // Check for unused variables in this scope
            semantic_check_unused_variables(analyzer, analyzer->current_scope);
            
            // Restore previous scope
            symtab_destroy(analyzer->current_scope);
            analyzer->current_scope = prev_scope;
            break;
        }
        
        case AST_VAR_DECL: {
            // Add variable to current scope
            Symbol *sym = symtab_insert(analyzer->current_scope,
                                       node->data.var_decl.name,
                                       SYM_VARIABLE,
                                       node->data.var_decl.type);
            if (sym) {
                sym->decl_line = node->line;
                sym->decl_column = node->column;
                
                // Check if initialized
                if (node->data.var_decl.initializer) {
                    sym->is_initialized = true;
                    analyze_expression(analyzer, node->data.var_decl.initializer);
                }
                LOG_DEBUG("Variable %s declared: initialized=%d", 
                          node->data.var_decl.name, sym->is_initialized);
            }
            break;
        }
        
        case AST_RETURN_STMT:
            analyzer->has_return_stmt = true;
            if (node->data.return_stmt.expression) {
                analyze_expression(analyzer, node->data.return_stmt.expression);
            }
            break;
            
        case AST_IF_STMT:
            analyze_expression(analyzer, node->data.if_stmt.condition);
            analyze_statement(analyzer, node->data.if_stmt.then_stmt);
            if (node->data.if_stmt.else_stmt) {
                analyze_statement(analyzer, node->data.if_stmt.else_stmt);
            }
            break;
            
        case AST_WHILE_STMT:
            analyze_expression(analyzer, node->data.while_stmt.condition);
            analyze_statement(analyzer, node->data.while_stmt.body);
            break;
            
        case AST_DO_WHILE_STMT:
            analyze_statement(analyzer, node->data.do_while_stmt.body);
            analyze_expression(analyzer, node->data.do_while_stmt.condition);
            break;
            
        case AST_FOR_STMT: {
            // Create new scope for for loop
            SymbolTable *prev_scope = analyzer->current_scope;
            analyzer->current_scope = symtab_create(prev_scope);
            
            // Analyze init, condition, update, and body
            if (node->data.for_stmt.init) {
                analyze_statement(analyzer, node->data.for_stmt.init);
            }
            if (node->data.for_stmt.condition) {
                analyze_expression(analyzer, node->data.for_stmt.condition);
            }
            if (node->data.for_stmt.update) {
                analyze_expression(analyzer, node->data.for_stmt.update);
            }
            analyze_statement(analyzer, node->data.for_stmt.body);
            
            // Check for unused variables in for scope
            semantic_check_unused_variables(analyzer, analyzer->current_scope);
            
            // Restore scope
            symtab_destroy(analyzer->current_scope);
            analyzer->current_scope = prev_scope;
            break;
        }
            
        case AST_EXPR_STMT:
            if (node->data.expr_stmt.expression) {
                analyze_expression(analyzer, node->data.expr_stmt.expression);
            }
            break;
            
        case AST_BREAK_STMT:
        case AST_CONTINUE_STMT:
            // Check for unreachable code after break/continue
            // This would require tracking control flow more carefully
            break;
            
        default:
            // Other statement types
            break;
    }
}

static void analyze_expression(SemanticAnalyzer *analyzer, ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_IDENTIFIER: {
            // Mark variable as used
            semantic_mark_used(analyzer->current_scope, node->data.identifier.name);
            
            // Check if initialized
            Symbol *sym = symtab_lookup(analyzer->current_scope, node->data.identifier.name);
            if (sym && sym->type == SYM_VARIABLE) {
                LOG_DEBUG("Variable %s used: initialized=%d, is_param=%d", 
                          node->data.identifier.name, sym->is_initialized, sym->is_param);
                if (!sym->is_initialized && !sym->is_param) {
                    ErrorContext ctx = {
                        .filename = NULL,
                        .line = node->line,
                        .column = node->column,
                        .length = strlen(node->data.identifier.name)
                    };
                    warning_uninitialized_variable(analyzer->error_manager, &ctx, 
                                                 node->data.identifier.name);
                }
            }
            break;
        }
        
        case AST_ASSIGNMENT: {
            // Mark left side as initialized
            semantic_mark_initialized(analyzer->current_scope, node->data.assignment.name);
            semantic_mark_used(analyzer->current_scope, node->data.assignment.name);
            
            // Analyze right side
            analyze_expression(analyzer, node->data.assignment.value);
            break;
        }
        
        case AST_BINARY_OP:
            analyze_expression(analyzer, node->data.binary_op.left);
            analyze_expression(analyzer, node->data.binary_op.right);
            break;
            
        case AST_UNARY_OP:
            analyze_expression(analyzer, node->data.unary_op.operand);
            break;
            
        case AST_FUNCTION_CALL:
            // Mark function as used
            semantic_mark_used(analyzer->current_scope, node->data.function_call.name);
            
            // Analyze arguments
            for (int i = 0; i < node->data.function_call.argument_count; i++) {
                analyze_expression(analyzer, node->data.function_call.arguments[i]);
            }
            break;
            
        case AST_ARRAY_ACCESS:
            analyze_expression(analyzer, node->data.array_access.array);
            analyze_expression(analyzer, node->data.array_access.index);
            break;
            
        case AST_MEMBER_ACCESS:
            analyze_expression(analyzer, node->data.member_access.object);
            break;
            
        case AST_CAST:
            analyze_expression(analyzer, node->data.cast.expression);
            // Could check for suspicious casts here
            break;
            
        case AST_TERNARY:
            analyze_expression(analyzer, node->data.ternary.condition);
            analyze_expression(analyzer, node->data.ternary.true_expr);
            analyze_expression(analyzer, node->data.ternary.false_expr);
            break;
            
        case AST_ADDRESS_OF:
        case AST_DEREFERENCE:
            analyze_expression(analyzer, node->data.unary_op.operand);
            break;
            
        default:
            // Literals and other expressions
            break;
    }
}

void semantic_check_unused_variables(SemanticAnalyzer *analyzer, SymbolTable *scope) {
    Symbol *sym = scope->symbols;
    while (sym) {
        if (sym->type == SYM_VARIABLE && !sym->is_used && !sym->is_param) {
            ErrorContext ctx = {
                .filename = NULL,
                .line = sym->decl_line,
                .column = sym->decl_column,
                .length = strlen(sym->name)
            };
            warning_unused_variable(analyzer->error_manager, &ctx, sym->name);
        }
        sym = sym->next;
    }
}

void semantic_mark_used(SymbolTable *scope, const char *name) {
    Symbol *sym = symtab_lookup(scope, name);
    if (sym) {
        sym->is_used = true;
    }
}

void semantic_mark_initialized(SymbolTable *scope, const char *name) {
    Symbol *sym = symtab_lookup(scope, name);
    if (sym) {
        sym->is_initialized = true;
    }
}
