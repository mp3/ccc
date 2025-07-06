#ifndef CODEGEN_H
#define CODEGEN_H

#include "parser.h"
#include "symtab.h"
#include <stdio.h>

typedef struct {
    FILE *output;
    int temp_counter;
    int label_counter;
    SymbolTable *symtab;
} CodeGenerator;

CodeGenerator *codegen_create(FILE *output);
void codegen_destroy(CodeGenerator *gen);
void codegen_generate(CodeGenerator *gen, ASTNode *ast);

#endif
