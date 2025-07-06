#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdbool.h>

typedef enum {
    SYM_VARIABLE,
    SYM_FUNCTION
} SymbolType;

typedef struct Symbol {
    char *name;
    SymbolType type;
    char *data_type;  // "int", etc.
    int offset;       // For local variables (stack offset)
    bool is_param;    // True if this is a function parameter
    bool is_array;    // True if this is an array
    int array_size;   // Size of the array (0 if not an array)
    // For functions
    char **param_types;
    char **param_names;
    int param_count;
    struct Symbol *next;
} Symbol;

typedef struct SymbolTable {
    Symbol *symbols;
    struct SymbolTable *parent;  // For nested scopes
    int next_offset;             // Next available stack offset
} SymbolTable;

SymbolTable *symtab_create(SymbolTable *parent);
void symtab_destroy(SymbolTable *table);
Symbol *symtab_insert(SymbolTable *table, const char *name, SymbolType type, const char *data_type);
Symbol *symtab_insert_array(SymbolTable *table, const char *name, const char *data_type, int size);
Symbol *symtab_insert_function(SymbolTable *table, const char *name, const char *return_type, 
                              char **param_types, char **param_names, int param_count);
Symbol *symtab_lookup(SymbolTable *table, const char *name);
Symbol *symtab_lookup_local(SymbolTable *table, const char *name);

#endif
