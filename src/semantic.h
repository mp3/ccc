#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"
#include "symtab.h"
#include "error.h"

// Semantic analyzer for warnings and additional checks
typedef struct {
    ErrorManager *error_manager;
    SymbolTable *current_scope;
    const char *current_function;  // Name of current function being analyzed
    bool has_return_stmt;          // Track if current function has return
} SemanticAnalyzer;

// Create and destroy semantic analyzer
SemanticAnalyzer *semantic_create(ErrorManager *error_manager);
void semantic_destroy(SemanticAnalyzer *analyzer);

// Analyze AST for warnings
void semantic_analyze(SemanticAnalyzer *analyzer, ASTNode *ast);

// Check for unused variables in a scope
void semantic_check_unused_variables(SemanticAnalyzer *analyzer, SymbolTable *scope);

// Mark a symbol as used
void semantic_mark_used(SymbolTable *scope, const char *name);

// Mark a symbol as initialized
void semantic_mark_initialized(SymbolTable *scope, const char *name);

#endif // SEMANTIC_H
