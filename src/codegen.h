#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "symtab.h"
#include <stdio.h>

typedef struct StringLiteral {
    char *label;
    char *value;
    int length;
    struct StringLiteral *next;
} StringLiteral;

typedef struct {
    FILE *output;
    int temp_counter;
    int label_counter;
    int string_counter;
    SymbolTable *symtab;
    char *current_function_return_type;
    StringLiteral *string_literals;
    // Loop control labels for break/continue
    char *current_loop_end_label;
    char *current_loop_continue_label;
} CodeGenerator;

CodeGenerator *codegen_create(FILE *output);
void codegen_destroy(CodeGenerator *gen);
void codegen_generate(CodeGenerator *gen, ASTNode *ast);

#endif
