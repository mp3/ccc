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

typedef struct StaticVariable {
    char *global_name;
    char *local_name;
    char *type;
    int initial_value;
    bool has_initializer;
    struct StaticVariable *next;
} StaticVariable;

typedef struct StructMember {
    char *name;
    char *type;
    int offset;
    int size;
} StructMember;

typedef struct StructType {
    char *name;
    StructMember *members;
    int member_count;
    int total_size;
    struct StructType *next;
} StructType;

typedef struct TypedefEntry {
    char *name;
    char *base_type;
    struct TypedefEntry *next;
} TypedefEntry;

typedef struct {
    FILE *output;
    int temp_counter;
    int label_counter;
    int string_counter;
    SymbolTable *symtab;
    char *current_function_return_type;
    char *current_function_name;
    StringLiteral *string_literals;
    StaticVariable *static_variables;
    // Loop control labels for break/continue
    char *current_loop_end_label;
    char *current_loop_continue_label;
    // Enum storage
    ASTNode **enums;
    int enum_count;
    // Struct type registry
    StructType *struct_types;
    // Typedef registry
    TypedefEntry *typedefs;
} CodeGenerator;

CodeGenerator *codegen_create(FILE *output);
void codegen_destroy(CodeGenerator *gen);
void codegen_generate(CodeGenerator *gen, ASTNode *ast);

#endif
