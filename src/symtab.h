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
Symbol *symtab_lookup(SymbolTable *table, const char *name);
Symbol *symtab_lookup_local(SymbolTable *table, const char *name);

#endif
