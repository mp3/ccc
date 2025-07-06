#include "symtab.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

SymbolTable *symtab_create(SymbolTable *parent) {
    SymbolTable *table = malloc(sizeof(SymbolTable));
    table->symbols = NULL;
    table->parent = parent;
    table->next_offset = 0;
    LOG_DEBUG("Created symbol table with parent %p", parent);
    return table;
}

void symtab_destroy(SymbolTable *table) {
    if (!table) return;
    
    Symbol *sym = table->symbols;
    while (sym) {
        Symbol *next = sym->next;
        free(sym->name);
        free(sym->data_type);
        free(sym);
        sym = next;
    }
    
    free(table);
}

Symbol *symtab_insert(SymbolTable *table, const char *name, SymbolType type, const char *data_type) {
    // Check if already exists in local scope
    if (symtab_lookup_local(table, name)) {
        LOG_ERROR("Symbol '%s' already defined in this scope", name);
        return NULL;
    }
    
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = type;
    sym->data_type = strdup(data_type);
    
    if (type == SYM_VARIABLE) {
        // Allocate space on the stack (assuming 4 bytes for int)
        sym->offset = table->next_offset;
        table->next_offset += 4;
    } else {
        sym->offset = 0;
    }
    
    // Insert at the beginning of the list
    sym->next = table->symbols;
    table->symbols = sym;
    
    LOG_DEBUG("Inserted symbol '%s' (type: %s, offset: %d)", 
              name, data_type, sym->offset);
    return sym;
}

Symbol *symtab_lookup_local(SymbolTable *table, const char *name) {
    Symbol *sym = table->symbols;
    while (sym) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

Symbol *symtab_lookup(SymbolTable *table, const char *name) {
    while (table) {
        Symbol *sym = symtab_lookup_local(table, name);
        if (sym) {
            return sym;
        }
        table = table->parent;
    }
    return NULL;
}
