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
        if (sym->type == SYM_FUNCTION) {
            for (int i = 0; i < sym->param_count; i++) {
                free(sym->param_types[i]);
                free(sym->param_names[i]);
            }
            free(sym->param_types);
            free(sym->param_names);
        } else if (sym->type == SYM_STRUCT) {
            for (int i = 0; i < sym->member_count; i++) {
                free(sym->struct_members[i]->name);
                free(sym->struct_members[i]->data_type);
                free(sym->struct_members[i]);
            }
            free(sym->struct_members);
        }
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
    sym->is_param = false;
    sym->is_array = false;
    sym->array_size = 0;
    sym->is_const = false;
    sym->param_types = NULL;
    sym->param_names = NULL;
    sym->param_count = 0;
    
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

Symbol *symtab_insert_array(SymbolTable *table, const char *name, const char *data_type, int size) {
    // Check if already exists in local scope
    if (symtab_lookup_local(table, name)) {
        LOG_ERROR("Symbol '%s' already defined in this scope", name);
        return NULL;
    }
    
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = SYM_VARIABLE;
    sym->data_type = strdup(data_type);
    sym->is_param = false;
    sym->is_array = true;
    sym->array_size = size;
    sym->is_const = false;
    sym->param_types = NULL;
    sym->param_names = NULL;
    sym->param_count = 0;
    
    // Allocate space on the stack for the array
    // Each element is 4 bytes for int, 1 byte for char
    int element_size = strcmp(data_type, "char") == 0 ? 1 : 4;
    sym->offset = table->next_offset;
    table->next_offset += size * element_size;
    
    // Insert at the beginning of the list
    sym->next = table->symbols;
    table->symbols = sym;
    
    LOG_DEBUG("Inserted array '%s' (type: %s[%d], offset: %d)", 
              name, data_type, size, sym->offset);
    return sym;
}

Symbol *symtab_insert_function(SymbolTable *table, const char *name, const char *return_type,
                              char **param_types, char **param_names, int param_count) {
    // Check if already exists in local scope
    if (symtab_lookup_local(table, name)) {
        LOG_ERROR("Function '%s' already defined in this scope", name);
        return NULL;
    }
    
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = SYM_FUNCTION;
    sym->data_type = strdup(return_type);
    sym->is_param = false;
    sym->is_array = false;
    sym->array_size = 0;
    sym->offset = 0;
    
    // Copy parameter information
    sym->param_count = param_count;
    if (param_count > 0) {
        sym->param_types = malloc(param_count * sizeof(char*));
        sym->param_names = malloc(param_count * sizeof(char*));
        for (int i = 0; i < param_count; i++) {
            sym->param_types[i] = strdup(param_types[i]);
            sym->param_names[i] = strdup(param_names[i]);
        }
    } else {
        sym->param_types = NULL;
        sym->param_names = NULL;
    }
    
    // Insert at the beginning of the list
    sym->next = table->symbols;
    table->symbols = sym;
    
    LOG_DEBUG("Inserted function '%s' (returns: %s, params: %d)", 
              name, return_type, param_count);
    return sym;
}

Symbol *symtab_insert_struct(SymbolTable *table, const char *name, struct Symbol **members, int member_count) {
    // Check if already exists in local scope
    if (symtab_lookup_local(table, name)) {
        LOG_ERROR("Symbol '%s' already defined in this scope", name);
        return NULL;
    }
    
    Symbol *sym = malloc(sizeof(Symbol));
    sym->name = strdup(name);
    sym->type = SYM_STRUCT;
    sym->data_type = strdup("struct");
    sym->is_param = false;
    sym->is_array = false;
    sym->array_size = 0;
    sym->offset = 0;
    
    // Copy struct members
    sym->member_count = member_count;
    if (member_count > 0) {
        sym->struct_members = malloc(member_count * sizeof(Symbol*));
        for (int i = 0; i < member_count; i++) {
            sym->struct_members[i] = malloc(sizeof(Symbol));
            sym->struct_members[i]->name = strdup(members[i]->name);
            sym->struct_members[i]->type = members[i]->type;
            sym->struct_members[i]->data_type = strdup(members[i]->data_type);
            sym->struct_members[i]->is_param = false;
            sym->struct_members[i]->is_array = members[i]->is_array;
            sym->struct_members[i]->array_size = members[i]->array_size;
            sym->struct_members[i]->offset = i * 4; // Simple offset calculation for now
        }
    } else {
        sym->struct_members = NULL;
    }
    
    // Insert at the beginning of the list
    sym->next = table->symbols;
    table->symbols = sym;
    
    LOG_DEBUG("Inserted struct '%s' with %d members", name, member_count);
    return sym;
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
