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
    gen->string_counter = 0;
    gen->symtab = NULL;
    gen->current_function_return_type = NULL;
    gen->string_literals = NULL;
    gen->static_variables = NULL;
    gen->current_loop_end_label = NULL;
    gen->current_loop_continue_label = NULL;
    gen->current_function_name = NULL;
    gen->enums = NULL;
    gen->enum_count = 0;
    LOG_DEBUG("Created code generator");
    return gen;
}

void codegen_destroy(CodeGenerator *gen) {
    if (gen) {
        if (gen->symtab) {
            symtab_destroy(gen->symtab);
        }
        // Free string literals
        StringLiteral *str = gen->string_literals;
        while (str) {
            StringLiteral *next = str->next;
            free(str->label);
            free(str->value);
            free(str);
            str = next;
        }
        // Free static variables
        StaticVariable *svar = gen->static_variables;
        while (svar) {
            StaticVariable *next = svar->next;
            free(svar->global_name);
            free(svar->local_name);
            free(svar->type);
            free(svar);
            svar = next;
        }
        // Free enum array
        if (gen->enums) {
            free(gen->enums);
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

static bool is_global_variable(CodeGenerator *gen, const char *name) {
    // Find the root symbol table (global scope)
    SymbolTable *global_table = gen->symtab;
    while (global_table->parent != NULL) {
        global_table = global_table->parent;
    }
    
    // Check if the variable exists in global scope
    Symbol *global_sym = symtab_lookup_local(global_table, name);
    return (global_sym != NULL);
}

// Helper function to look up enum constant values
static bool get_enum_value(CodeGenerator *gen, const char *name, int *value) {
    for (int i = 0; i < gen->enum_count; i++) {
        ASTNode *enum_node = gen->enums[i];
        for (int j = 0; j < enum_node->data.enum_decl.enumerator_count; j++) {
            if (strcmp(enum_node->data.enum_decl.enumerator_names[j], name) == 0) {
                *value = enum_node->data.enum_decl.enumerator_values[j];
                return true;
            }
        }
    }
    return false;
}

// Helper function to check if a symbol is a static variable and extract its info
static bool is_static_variable(Symbol *sym, char *real_type, char *global_name) {
    if (!sym || !sym->data_type) return false;
    
    // Check if the data_type contains the static marker
    const char *static_marker = ":static:@";
    char *marker_pos = strstr(sym->data_type, static_marker);
    if (!marker_pos) return false;
    
    // Extract the real type (everything before :static:)
    size_t type_len = marker_pos - sym->data_type;
    strncpy(real_type, sym->data_type, type_len);
    real_type[type_len] = '\0';
    
    // Extract the global name (everything after @)
    strcpy(global_name, marker_pos + strlen(static_marker));
    
    return true;
}

static char *c_type_to_llvm_type(const char *c_type) {
    static char llvm_type[256];
    
    // Check for function pointer type: return_type(*)(param_types)
    if (strstr(c_type, "(*)")) {
        // Parse function pointer type
        char type_copy[256];
        strcpy(type_copy, c_type);
        
        // Extract return type
        char *func_ptr_marker = strstr(type_copy, "(*)");
        *func_ptr_marker = '\0';
        char *return_type = type_copy;
        
        // Extract parameter types
        char *params_start = strchr(func_ptr_marker + 3, '(');
        if (params_start) {
            params_start++; // Skip '('
            char *params_end = strchr(params_start, ')');
            if (params_end) {
                *params_end = '\0';
                
                // Build LLVM function pointer type
                char llvm_return_type[64];
                if (strcmp(return_type, "int") == 0) {
                    strcpy(llvm_return_type, "i32");
                } else if (strcmp(return_type, "char") == 0) {
                    strcpy(llvm_return_type, "i8");
                } else {
                    strcpy(llvm_return_type, "i32"); // Default
                }
                
                // Build parameter list
                char llvm_params[256] = "";
                if (strlen(params_start) > 0) {
                    // Simple parameter parsing - assumes int/char types
                    char *param = strtok(params_start, ",");
                    bool first = true;
                    while (param) {
                        if (!first) strcat(llvm_params, ", ");
                        first = false;
                        
                        // Trim whitespace
                        while (*param == ' ') param++;
                        
                        if (strstr(param, "int")) {
                            strcat(llvm_params, "i32");
                        } else if (strstr(param, "char")) {
                            strcat(llvm_params, "i8");
                        } else {
                            strcat(llvm_params, "i32"); // Default
                        }
                        
                        param = strtok(NULL, ",");
                    }
                }
                
                snprintf(llvm_type, sizeof(llvm_type), "%s (%s)*", 
                        llvm_return_type, llvm_params);
                return llvm_type;
            }
        }
    }
    
    if (strcmp(c_type, "int") == 0) {
        strcpy(llvm_type, "i32");
    } else if (strcmp(c_type, "char") == 0) {
        strcpy(llvm_type, "i32");  // Treat char as i32 for simplicity
    } else if (strcmp(c_type, "int*") == 0) {
        strcpy(llvm_type, "i32*");
    } else if (strcmp(c_type, "char*") == 0) {
        strcpy(llvm_type, "i8*");  // char* is pointer to i8
    } else if (strcmp(c_type, "int**") == 0) {
        strcpy(llvm_type, "i32**");
    } else if (strcmp(c_type, "char**") == 0) {
        strcpy(llvm_type, "i8**");
    } else {
        // Generic pointer handling for multiple levels
        strcpy(llvm_type, c_type);
        // Replace int with i32 and char with i8
        char *pos = strstr(llvm_type, "int");
        if (pos) {
            memmove(pos + 3, pos + 3, strlen(pos + 3) + 1);
            memcpy(pos, "i32", 3);
        } else {
            pos = strstr(llvm_type, "char");
            if (pos) {
                memmove(pos + 2, pos + 4, strlen(pos + 4) + 1);
                memcpy(pos, "i8", 2);
            }
        }
    }
    
    return llvm_type;
}

static void emit_static_variables(CodeGenerator *gen) {
    // Emit static variables in reverse order (so they appear in the order they were encountered)
    StaticVariable *reversed = NULL;
    StaticVariable *current = gen->static_variables;
    while (current) {
        StaticVariable *next = current->next;
        current->next = reversed;
        reversed = current;
        current = next;
    }
    
    // Now emit them
    current = reversed;
    while (current) {
        char *llvm_type = c_type_to_llvm_type(current->type);
        if (current->has_initializer) {
            fprintf(gen->output, "@%s = internal global %s %d\n",
                    current->global_name, llvm_type, current->initial_value);
        } else {
            fprintf(gen->output, "@%s = internal global %s 0\n",
                    current->global_name, llvm_type);
        }
        current = current->next;
    }
    
    if (reversed) {
        fprintf(gen->output, "\n");
    }
}

static void emit_string_literals(CodeGenerator *gen) {
    // Emit string literals in reverse order (so they appear in the order they were encountered)
    StringLiteral *reversed = NULL;
    StringLiteral *current = gen->string_literals;
    while (current) {
        StringLiteral *next = current->next;
        current->next = reversed;
        reversed = current;
        current = next;
    }
    
    // Now emit them
    current = reversed;
    while (current) {
        fprintf(gen->output, "%s = private unnamed_addr constant [%d x i8] c\"",
                current->label, current->length);
        
        // Emit string with escape sequences
        const char *s = current->value;
        while (*s) {
            switch (*s) {
                case '\n': fprintf(gen->output, "\\0A"); break;
                case '\t': fprintf(gen->output, "\\09"); break;
                case '\r': fprintf(gen->output, "\\0D"); break;
                case '\\': fprintf(gen->output, "\\5C"); break;
                case '"': fprintf(gen->output, "\\22"); break;
                default:
                    if (*s >= 32 && *s <= 126) {
                        fputc(*s, gen->output);
                    } else {
                        fprintf(gen->output, "\\%02X", (unsigned char)*s);
                    }
            }
            s++;
        }
        fprintf(gen->output, "\\00\"\n");  // null terminator
        current = current->next;
    }
    
    if (reversed) {
        fprintf(gen->output, "\n");  // Extra newline after string literals
    }
}

static char *codegen_expression(CodeGenerator *gen, ASTNode *expr) {
    LOG_TRACE("codegen_expression: type=%d", expr->type);
    switch (expr->type) {
        case AST_INT_LITERAL: {
            char *temp = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = add i32 0, %d\n", temp, expr->data.int_literal.value);
            return temp;
        }
        
        case AST_FLOAT_LITERAL: {
            char *temp = codegen_next_temp(gen);
            // Use fadd for floating point
            fprintf(gen->output, "  %s = fadd double 0.0, %f\n", temp, expr->data.float_literal.value);
            return temp;
        }
        
        case AST_CHAR_LITERAL: {
            char *temp = codegen_next_temp(gen);
            // Generate as i32 instead of i8 for compatibility with function calls
            fprintf(gen->output, "  %s = add i32 0, %d\n", temp, (int)expr->data.char_literal.value);
            return temp;
        }
        
        case AST_STRING_LITERAL: {
            // Create a new string literal entry
            StringLiteral *str_lit = malloc(sizeof(StringLiteral));
            str_lit->label = malloc(32);
            snprintf(str_lit->label, 32, "@.str.%d", gen->string_counter++);
            str_lit->value = strdup(expr->data.string_literal.value);
            str_lit->length = strlen(expr->data.string_literal.value) + 1;
            
            // Add to the list
            str_lit->next = gen->string_literals;
            gen->string_literals = str_lit;
            
            // Return a pointer to the first element
            char *temp = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = getelementptr [%d x i8], [%d x i8]* %s, i32 0, i32 0\n",
                    temp, str_lit->length, str_lit->length, str_lit->label);
            
            return temp;
        }
        
        case AST_IDENTIFIER: {
            // First check if it's an enum constant
            int enum_value;
            if (get_enum_value(gen, expr->data.identifier.name, &enum_value)) {
                // It's an enum constant - return its value as a literal
                char *temp = codegen_next_temp(gen);
                fprintf(gen->output, "  %s = add i32 0, %d\n", temp, enum_value);
                return temp;
            }
            
            Symbol *sym = symtab_lookup(gen->symtab, expr->data.identifier.name);
            if (!sym) {
                LOG_ERROR("Undefined variable: %s", expr->data.identifier.name);
                exit(1);
            }
            
            // If this is a function, return the function pointer directly
            if (sym->type == SYM_FUNCTION) {
                // No need to load, just return the function name with @
                char *func_ref = malloc(strlen(sym->name) + 2);
                sprintf(func_ref, "@%s", sym->name);
                return func_ref;
            }
            
            char *temp = codegen_next_temp(gen);
            char real_type[256];
            char global_name[256];
            
            if (is_static_variable(sym, real_type, global_name)) {
                // This is a static variable
                char *llvm_type = c_type_to_llvm_type(real_type);
                fprintf(gen->output, "  %s = load %s, %s* @%s\n", temp, llvm_type, llvm_type, global_name);
            } else {
                // Regular variable
                char *llvm_type = c_type_to_llvm_type(sym->data_type);
                
                if (is_global_variable(gen, expr->data.identifier.name)) {
                    // This is a global variable
                    fprintf(gen->output, "  %s = load %s, %s* @%s\n", temp, llvm_type, llvm_type, sym->name);
                } else {
                    // This is a local variable
                    fprintf(gen->output, "  %s = load %s, %s* %%%s\n", temp, llvm_type, llvm_type, sym->name);
                }
            }
            return temp;
        }
        
        case AST_ASSIGNMENT: {
            Symbol *sym = symtab_lookup(gen->symtab, expr->data.assignment.name);
            if (!sym) {
                LOG_ERROR("Undefined variable: %s", expr->data.assignment.name);
                exit(1);
            }
            
            // Check if trying to assign to a const variable
            if (sym->is_const) {
                LOG_ERROR("Cannot assign to const variable: %s", expr->data.assignment.name);
                exit(1);
            }
            
            char *value = codegen_expression(gen, expr->data.assignment.value);
            char real_type[256];
            char global_name[256];
            
            if (is_static_variable(sym, real_type, global_name)) {
                // This is a static variable assignment
                char *llvm_type = c_type_to_llvm_type(real_type);
                fprintf(gen->output, "  store %s %s, %s* @%s\n", llvm_type, value, llvm_type, global_name);
            } else {
                // Regular variable
                char *llvm_type = c_type_to_llvm_type(sym->data_type);
                
                if (is_global_variable(gen, expr->data.assignment.name)) {
                    // This is a global variable assignment
                    fprintf(gen->output, "  store %s %s, %s* @%s\n", llvm_type, value, llvm_type, sym->name);
                } else {
                    // This is a local variable assignment
                    fprintf(gen->output, "  store %s %s, %s* %%%s\n", llvm_type, value, llvm_type, sym->name);
                }
            }
            return value;
        }
        
        case AST_BINARY_OP: {
            // Handle assignment to array element or dereference
            if (expr->data.binary_op.op == TOKEN_ASSIGN) {
                if (expr->data.binary_op.left->type == AST_ARRAY_ACCESS) {
                // Array element assignment
                ASTNode *array_access = expr->data.binary_op.left;
                
                // Get array symbol
                if (array_access->data.array_access.array->type != AST_IDENTIFIER) {
                    LOG_ERROR("Array access must be on an identifier");
                    exit(1);
                }
                
                const char *name = array_access->data.array_access.array->data.identifier.name;
                Symbol *sym = symtab_lookup(gen->symtab, name);
                if (!sym) {
                    LOG_ERROR("Undefined variable: %s", name);
                    exit(1);
                }
                
                // Evaluate index and value
                char *index = codegen_expression(gen, array_access->data.array_access.index);
                char *value = codegen_expression(gen, expr->data.binary_op.right);
                
                if (sym->is_array) {
                    // Array element assignment
                    // Generate getelementptr to get address of array element
                    char *addr = codegen_next_temp(gen);
                    const char *llvm_type = strcmp(sym->data_type, "char") == 0 ? "i8" : "i32";
                    
                    // Check if it's a global array
                    bool is_global = is_global_variable(gen, name);
                    const char *prefix = is_global ? "@" : "%";
                    
                    fprintf(gen->output, "  %s = getelementptr [%d x %s], [%d x %s]* %s%s, i32 0, i32 %s\n",
                            addr, sym->array_size, llvm_type, sym->array_size, llvm_type, prefix, sym->name, index);
                    
                    // Store the value - need to check if we need to truncate
                    if (strcmp(llvm_type, "i8") == 0) {
                        // Truncate i32 to i8
                        char *truncated = codegen_next_temp(gen);
                        fprintf(gen->output, "  %s = trunc i32 %s to i8\n", truncated, value);
                        fprintf(gen->output, "  store i8 %s, i8* %s\n", truncated, addr);
                        free(truncated);
                    } else {
                        fprintf(gen->output, "  store %s %s, %s* %s\n", llvm_type, value, llvm_type, addr);
                    }
                    
                    free(index);
                    free(addr);
                    return value;
                } else if (strstr(sym->data_type, "*")) {
                    // Pointer element assignment - p[i] = value
                    // Load the pointer value first
                    char *ptr_value = codegen_next_temp(gen);
                    
                    // Determine types
                    const char *llvm_base_type = "i32";
                    const char *llvm_ptr_type = "i32*";
                    
                    if (strstr(sym->data_type, "char*")) {
                        llvm_base_type = "i8";
                        llvm_ptr_type = "i8*";
                    }
                    
                    // Load the pointer
                    fprintf(gen->output, "  %s = load %s, %s* %%%s\n", 
                            ptr_value, llvm_ptr_type, llvm_ptr_type, sym->name);
                    
                    // Calculate the address using getelementptr
                    char *addr = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = getelementptr %s, %s %s, i32 %s\n",
                            addr, llvm_base_type, llvm_ptr_type, ptr_value, index);
                    
                    // Store the value - need to check if we need to truncate
                    if (strcmp(llvm_base_type, "i8") == 0) {
                        // Truncate i32 to i8
                        char *truncated = codegen_next_temp(gen);
                        fprintf(gen->output, "  %s = trunc i32 %s to i8\n", truncated, value);
                        fprintf(gen->output, "  store i8 %s, i8* %s\n", truncated, addr);
                        free(truncated);
                    } else {
                        fprintf(gen->output, "  store %s %s, %s* %s\n", llvm_base_type, value, llvm_base_type, addr);
                    }
                    
                    free(index);
                    free(ptr_value);
                    free(addr);
                    return value;
                } else {
                    LOG_ERROR("'%s' is not an array or pointer", name);
                    exit(1);
                }
            } else if (expr->data.binary_op.left->type == AST_DEREFERENCE) {
                // Dereference assignment (*p = value)
                char *ptr = codegen_expression(gen, expr->data.binary_op.left->data.unary_op.operand);
                char *value = codegen_expression(gen, expr->data.binary_op.right);
                
                // Store through pointer (assume i32 for now)
                fprintf(gen->output, "  store i32 %s, i32* %s\n", value, ptr);
                
                free(ptr);
                return value;
            }
            }
            
            // For logical operators, we need special handling for short-circuit evaluation
            if (expr->data.binary_op.op == TOKEN_AND || expr->data.binary_op.op == TOKEN_OR) {
                char *left = codegen_expression(gen, expr->data.binary_op.left);
                // Right side will be evaluated conditionally inside the if blocks below
                char *result = codegen_next_temp(gen);
                
                if (expr->data.binary_op.op == TOKEN_AND) {
                    // For &&, if left is false, result is false (don't evaluate right)
                    char *left_bool = codegen_next_temp(gen);
                    char *check_label = codegen_next_label(gen, "and.check.");
                    char *false_label = codegen_next_label(gen, "and.false.");
                    char *end_label = codegen_next_label(gen, "and.end.");
                    
                    // Check if left is zero
                    fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", left_bool, left);
                    fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                            left_bool, check_label, false_label);
                    
                    // Check right side (only if left was true)
                    fprintf(gen->output, "\n%s:\n", check_label);
                    char *right = codegen_expression(gen, expr->data.binary_op.right);
                    char *right_bool = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", right_bool, right);
                    char *right_int = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = zext i1 %s to i32\n", right_int, right_bool);
                    fprintf(gen->output, "  br label %%%s\n", end_label);
                    
                    // False branch
                    fprintf(gen->output, "\n%s:\n", false_label);
                    fprintf(gen->output, "  br label %%%s\n", end_label);
                    
                    // End - phi node to get result
                    fprintf(gen->output, "\n%s:\n", end_label);
                    fprintf(gen->output, "  %s = phi i32 [ 0, %%%s ], [ %s, %%%s ]\n",
                            result, false_label, right_int, check_label);
                    
                    free(left);
                    free(right);
                    free(left_bool);
                    free(right_bool);
                    free(right_int);
                    free(check_label);
                    free(false_label);
                    free(end_label);
                    return result;
                } else { // TOKEN_OR
                    // For ||, if left is true, result is true (don't evaluate right)
                    char *left_bool = codegen_next_temp(gen);
                    char *check_label = codegen_next_label(gen, "or.check.");
                    char *true_label = codegen_next_label(gen, "or.true.");
                    char *end_label = codegen_next_label(gen, "or.end.");
                    
                    // Check if left is non-zero
                    fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", left_bool, left);
                    fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                            left_bool, true_label, check_label);
                    
                    // Check right side (only if left was false)
                    fprintf(gen->output, "\n%s:\n", check_label);
                    char *right = codegen_expression(gen, expr->data.binary_op.right);
                    char *right_bool = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", right_bool, right);
                    char *right_int = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = zext i1 %s to i32\n", right_int, right_bool);
                    fprintf(gen->output, "  br label %%%s\n", end_label);
                    
                    // True branch
                    fprintf(gen->output, "\n%s:\n", true_label);
                    fprintf(gen->output, "  br label %%%s\n", end_label);
                    
                    // End - phi node to get result
                    fprintf(gen->output, "\n%s:\n", end_label);
                    fprintf(gen->output, "  %s = phi i32 [ 1, %%%s ], [ %s, %%%s ]\n",
                            result, true_label, right_int, check_label);
                    
                    free(left);
                    free(right);
                    free(left_bool);
                    free(right_bool);
                    free(right_int);
                    free(check_label);
                    free(true_label);
                    free(end_label);
                    return result;
                }
            }
            
            // For other operators, evaluate both operands normally
            char *left = codegen_expression(gen, expr->data.binary_op.left);
            char *right = codegen_expression(gen, expr->data.binary_op.right);
            char *result = codegen_next_temp(gen);
            
            // Check for pointer arithmetic
            bool left_is_pointer = false;
            bool right_is_pointer = false;
            const char *left_type = "i32";
            const char *right_type = "i32";
            const char *pointed_type = "i32";
            
            // Determine if operands are pointers by checking their AST nodes
            if (expr->data.binary_op.left->type == AST_IDENTIFIER) {
                Symbol *left_sym = symtab_lookup(gen->symtab, expr->data.binary_op.left->data.identifier.name);
                if (left_sym && strstr(left_sym->data_type, "*")) {
                    left_is_pointer = true;
                    left_type = c_type_to_llvm_type(left_sym->data_type);
                    // Extract pointed-to type (remove one level of pointer)
                    if (strstr(left_sym->data_type, "int*")) {
                        pointed_type = "i32";
                    } else if (strstr(left_sym->data_type, "char*")) {
                        pointed_type = "i8";
                    }
                }
            }
            
            if (expr->data.binary_op.right->type == AST_IDENTIFIER) {
                Symbol *right_sym = symtab_lookup(gen->symtab, expr->data.binary_op.right->data.identifier.name);
                if (right_sym && strstr(right_sym->data_type, "*")) {
                    right_is_pointer = true;
                    right_type = c_type_to_llvm_type(right_sym->data_type);
                }
            }
            
            const char *op;
            switch (expr->data.binary_op.op) {
                case TOKEN_PLUS:
                    if (left_is_pointer && !right_is_pointer) {
                        // Pointer + integer: use getelementptr
                        fprintf(gen->output, "  %s = getelementptr %s, %s %s, i32 %s\n", 
                                result, pointed_type, left_type, left, right);
                        free(left);
                        free(right);
                        return result;
                    } else if (!left_is_pointer && right_is_pointer) {
                        // Integer + pointer: use getelementptr with swapped operands
                        fprintf(gen->output, "  %s = getelementptr %s, %s %s, i32 %s\n", 
                                result, pointed_type, right_type, right, left);
                        free(left);
                        free(right);
                        return result;
                    }
                    op = "add"; 
                    break;
                case TOKEN_MINUS:
                    if (left_is_pointer && !right_is_pointer) {
                        // Pointer - integer: use getelementptr with negative offset
                        char *neg_right = codegen_next_temp(gen);
                        fprintf(gen->output, "  %s = sub i32 0, %s\n", neg_right, right);
                        fprintf(gen->output, "  %s = getelementptr %s, %s %s, i32 %s\n", 
                                result, pointed_type, left_type, left, neg_right);
                        free(left);
                        free(right);
                        free(neg_right);
                        return result;
                    } else if (left_is_pointer && right_is_pointer) {
                        // Pointer - pointer: calculate difference in elements
                        char *left_int = codegen_next_temp(gen);
                        char *right_int = codegen_next_temp(gen);
                        fprintf(gen->output, "  %s = ptrtoint %s %s to i64\n", left_int, left_type, left);
                        fprintf(gen->output, "  %s = ptrtoint %s %s to i64\n", right_int, right_type, right);
                        char *diff = codegen_next_temp(gen);
                        fprintf(gen->output, "  %s = sub i64 %s, %s\n", diff, left_int, right_int);
                        
                        // Divide by element size to get element count
                        int element_size = strcmp(pointed_type, "i8") == 0 ? 1 : 4;
                        char *div_result = codegen_next_temp(gen);
                        fprintf(gen->output, "  %s = sdiv i64 %s, %d\n", div_result, diff, element_size);
                        fprintf(gen->output, "  %s = trunc i64 %s to i32\n", result, div_result);
                        
                        free(left);
                        free(right);
                        free(left_int);
                        free(right_int);
                        free(diff);
                        free(div_result);
                        return result;
                    }
                    op = "sub"; 
                    break;
                case TOKEN_STAR: op = "mul"; break;
                case TOKEN_SLASH: op = "sdiv"; break;
                case TOKEN_PERCENT: op = "srem"; break;
                case TOKEN_AMPERSAND: op = "and"; break;
                case TOKEN_PIPE: op = "or"; break;
                case TOKEN_CARET: op = "xor"; break;
                case TOKEN_LSHIFT: op = "shl"; break;
                case TOKEN_RSHIFT: op = "ashr"; break;  // arithmetic shift right for signed integers
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
                case TOKEN_COMMA: {
                    // Comma operator: evaluate left for side effects, return right
                    free(left);  // Discard left result
                    free(result);
                    return right;  // Return right result
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
            
            // Check for built-in/external functions
            const char *func_name = expr->data.function_call.name;
            bool is_external = false;
            bool is_func_ptr = false;
            const char *return_type = "i32";
            const char *param_types[10]; // max 10 params for built-ins
            int expected_params = 0;
            
            if (strcmp(func_name, "putchar") == 0) {
                is_external = true;
                return_type = "i32";
                param_types[0] = "i32";
                expected_params = 1;
            } else if (strcmp(func_name, "getchar") == 0) {
                is_external = true;
                return_type = "i32";
                expected_params = 0;
            } else if (strcmp(func_name, "puts") == 0) {
                is_external = true;
                return_type = "i32";
                param_types[0] = "i8*";
                expected_params = 1;
            } else if (strcmp(func_name, "printf") == 0) {
                is_external = true;
                return_type = "i32";
                param_types[0] = "i8*";
                expected_params = -1; // variadic
            } else if (strcmp(func_name, "malloc") == 0) {
                is_external = true;
                return_type = "i8*";
                param_types[0] = "i64";
                expected_params = 1;
            } else if (strcmp(func_name, "free") == 0) {
                is_external = true;
                return_type = "void";
                param_types[0] = "i8*";
                expected_params = 1;
            } else if (strcmp(func_name, "exit") == 0) {
                is_external = true;
                return_type = "void";
                param_types[0] = "i32";
                expected_params = 1;
            } else if (strcmp(func_name, "strlen") == 0) {
                is_external = true;
                return_type = "i64";
                param_types[0] = "i8*";
                expected_params = 1;
            } else if (strcmp(func_name, "strcpy") == 0) {
                is_external = true;
                return_type = "i8*";
                param_types[0] = "i8*";
                param_types[1] = "i8*";
                expected_params = 2;
            } else if (strcmp(func_name, "strcmp") == 0) {
                is_external = true;
                return_type = "i32";
                param_types[0] = "i8*";
                param_types[1] = "i8*";
                expected_params = 2;
            } else if (strcmp(func_name, "strcat") == 0) {
                is_external = true;
                return_type = "i8*";
                param_types[0] = "i8*";
                param_types[1] = "i8*";
                expected_params = 2;
            } else if (strcmp(func_name, "atoi") == 0) {
                is_external = true;
                return_type = "i32";
                param_types[0] = "i8*";
                expected_params = 1;
            } else if (strcmp(func_name, "memcpy") == 0) {
                is_external = true;
                return_type = "i8*";
                param_types[0] = "i8*";
                param_types[1] = "i8*";
                param_types[2] = "i64";
                expected_params = 3;
            } else if (strcmp(func_name, "memset") == 0) {
                is_external = true;
                return_type = "i8*";
                param_types[0] = "i8*";
                param_types[1] = "i32";
                param_types[2] = "i64";
                expected_params = 3;
            } else if (func_sym && func_sym->type == SYM_VARIABLE && 
                      strstr(func_sym->data_type, "(*)")) {
                // This is a function pointer variable
                is_func_ptr = true;
                // Extract return type from function pointer type
                // Format: "return_type(*)(param_types)"
                char type_copy[256];
                strcpy(type_copy, func_sym->data_type);
                char *paren = strstr(type_copy, "(*)");
                if (paren) {
                    *paren = '\0';
                    return_type = strcmp(type_copy, "int") == 0 ? "i32" : "i8";
                }
            } else if (!func_sym || func_sym->type != SYM_FUNCTION) {
                LOG_ERROR("Undefined function: %s", expr->data.function_call.name);
                exit(1);
            }
            
            // Use function symbol if not external
            if (!is_external && !is_func_ptr) {
                // Check argument count for regular functions
                if (func_sym->is_variadic) {
                    // Variadic functions must have at least the required parameters
                    if (expr->data.function_call.argument_count < func_sym->param_count) {
                        LOG_ERROR("Variadic function '%s' expects at least %d arguments, got %d",
                                 expr->data.function_call.name,
                                 func_sym->param_count,
                                 expr->data.function_call.argument_count);
                        exit(1);
                    }
                } else {
                    // Non-variadic functions must have exact argument count
                    if (expr->data.function_call.argument_count != func_sym->param_count) {
                        LOG_ERROR("Function '%s' expects %d arguments, got %d",
                                 expr->data.function_call.name,
                                 func_sym->param_count,
                                 expr->data.function_call.argument_count);
                        exit(1);
                    }
                }
            } else if (is_external) {
                // Check argument count for external functions
                if (expected_params >= 0 && expr->data.function_call.argument_count != expected_params) {
                    LOG_ERROR("Function '%s' expects %d arguments, got %d",
                             func_name, expected_params,
                             expr->data.function_call.argument_count);
                    exit(1);
                } else if (expected_params == -1 && expr->data.function_call.argument_count < 1) {
                    // Variadic functions need at least one argument
                    LOG_ERROR("Variadic function '%s' expects at least 1 argument, got %d",
                             func_name, expr->data.function_call.argument_count);
                    exit(1);
                }
            }
            // For function pointers, we can't check argument count at compile time
            
            // Evaluate arguments and convert types if needed
            char **arg_values = malloc(expr->data.function_call.argument_count * sizeof(char*));
            for (int i = 0; i < expr->data.function_call.argument_count; i++) {
                arg_values[i] = codegen_expression(gen, expr->data.function_call.arguments[i]);
                
                // Convert types for external functions if needed
                if (is_external && i < expected_params && expected_params >= 0) {
                    if (strcmp(param_types[i], "i64") == 0) {
                        // Need to extend i32 to i64
                        char *extended = codegen_next_temp(gen);
                        fprintf(gen->output, "  %s = sext i32 %s to i64\n", extended, arg_values[i]);
                        free(arg_values[i]);
                        arg_values[i] = extended;
                    }
                }
            }
            
            // Generate function call
            char *result = codegen_next_temp(gen);
            
            if (is_func_ptr) {
                // For function pointers, load the pointer value first
                char *func_ptr_val = codegen_next_temp(gen);
                
                // Determine function pointer type
                char func_ptr_type[256];
                // Use c_type_to_llvm_type to convert the whole function pointer type
                strcpy(func_ptr_type, c_type_to_llvm_type(func_sym->data_type));
                
                fprintf(gen->output, "  %s = load %s, %s* %%%s\n", 
                        func_ptr_val, func_ptr_type, func_ptr_type, func_name);
                
                // Call through function pointer
                fprintf(gen->output, "  %s = call %s %s(", result, return_type, func_ptr_val);
                for (int i = 0; i < expr->data.function_call.argument_count; i++) {
                    if (i > 0) fprintf(gen->output, ", ");
                    fprintf(gen->output, "i32 %s", arg_values[i]);
                }
                fprintf(gen->output, ")\n");
            } else {
                // Regular function call
                if (strcmp(return_type, "void") == 0) {
                    fprintf(gen->output, "  call void @%s(", func_name);
                } else {
                    fprintf(gen->output, "  %s = call %s @%s(", result, return_type, func_name);
                }
                for (int i = 0; i < expr->data.function_call.argument_count; i++) {
                    if (i > 0) fprintf(gen->output, ", ");
                    if (is_external) {
                        // For variadic functions like printf, we need to handle args differently
                        if (expected_params == -1 && i >= 1) {
                            // For variadic args beyond the first, assume i32 for now
                            fprintf(gen->output, "i32 %s", arg_values[i]);
                        } else if (i < expected_params || expected_params == -1) {
                            fprintf(gen->output, "%s %s", param_types[i], arg_values[i]);
                        }
                    } else {
                        fprintf(gen->output, "i32 %s", arg_values[i]);
                    }
                }
                if (is_external && expected_params == -1) {
                    // No extra syntax needed for variadic in call
                }
                fprintf(gen->output, ")\n");
            }
            
            // Free argument values
            for (int i = 0; i < expr->data.function_call.argument_count; i++) {
                free(arg_values[i]);
            }
            free(arg_values);
            
            // For void functions, return a dummy value (0)
            if (strcmp(return_type, "void") == 0) {
                free(result);
                char *dummy = codegen_next_temp(gen);
                fprintf(gen->output, "  %s = add i32 0, 0  ; void function result\n", dummy);
                return dummy;
            }
            
            return result;
        }
        
        case AST_ARRAY_ACCESS: {
            // Get array/pointer symbol
            if (expr->data.array_access.array->type != AST_IDENTIFIER) {
                LOG_ERROR("Array access must be on an identifier");
                exit(1);
            }
            
            const char *name = expr->data.array_access.array->data.identifier.name;
            Symbol *sym = symtab_lookup(gen->symtab, name);
            if (!sym) {
                LOG_ERROR("Undefined variable: %s", name);
                exit(1);
            }
            
            // Evaluate index
            char *index = codegen_expression(gen, expr->data.array_access.index);
            
            if (sym->is_array) {
                // Array access
                // Generate getelementptr to get address of array element
                char *addr = codegen_next_temp(gen);
                const char *llvm_type = strcmp(sym->data_type, "char") == 0 ? "i8" : "i32";
                
                // Check if it's a global array
                bool is_global = is_global_variable(gen, name);
                const char *prefix = is_global ? "@" : "%";
                
                fprintf(gen->output, "  %s = getelementptr [%d x %s], [%d x %s]* %s%s, i32 0, i32 %s\n",
                        addr, sym->array_size, llvm_type, sym->array_size, llvm_type, prefix, sym->name, index);
                
                // Load the value
                char *value = codegen_next_temp(gen);
                fprintf(gen->output, "  %s = load %s, %s* %s\n", value, llvm_type, llvm_type, addr);
                
                free(index);
                free(addr);
                return value;
            } else if (strstr(sym->data_type, "*")) {
                // Pointer access - p[i] is equivalent to *(p + i)
                // Load the pointer value first
                char *ptr_value = codegen_next_temp(gen);
                
                // Determine the base type (what the pointer points to)
                const char *llvm_base_type = "i32";
                const char *llvm_ptr_type = "i32*";
                
                if (strstr(sym->data_type, "char*")) {
                    llvm_base_type = "i8";
                    llvm_ptr_type = "i8*";
                }
                
                // Load the pointer
                fprintf(gen->output, "  %s = load %s, %s* %%%s\n", 
                        ptr_value, llvm_ptr_type, llvm_ptr_type, sym->name);
                
                // Calculate the address using getelementptr
                char *addr = codegen_next_temp(gen);
                fprintf(gen->output, "  %s = getelementptr %s, %s %s, i32 %s\n",
                        addr, llvm_base_type, llvm_ptr_type, ptr_value, index);
                
                // Load the value at that address
                char *value = codegen_next_temp(gen);
                fprintf(gen->output, "  %s = load %s, %s* %s\n", 
                        value, llvm_base_type, llvm_base_type, addr);
                
                free(index);
                free(ptr_value);
                free(addr);
                return value;
            } else {
                LOG_ERROR("'%s' is not an array or pointer", name);
                exit(1);
            }
        }
        
        case AST_ADDRESS_OF: {
            // Address-of operator - return pointer to the operand
            if (expr->data.unary_op.operand->type == AST_IDENTIFIER) {
                Symbol *sym = symtab_lookup(gen->symtab, expr->data.unary_op.operand->data.identifier.name);
                if (!sym) {
                    LOG_ERROR("Undefined variable: %s", expr->data.unary_op.operand->data.identifier.name);
                    exit(1);
                }
                // Just return the address (which is the variable name in LLVM)
                char *result = malloc(256);
                snprintf(result, 256, "%%%s", sym->name);
                return result;
            } else if (expr->data.unary_op.operand->type == AST_ARRAY_ACCESS) {
                // Address of array element
                ASTNode *array_access = expr->data.unary_op.operand;
                if (array_access->data.array_access.array->type != AST_IDENTIFIER) {
                    LOG_ERROR("Array access must be on an identifier");
                    exit(1);
                }
                
                const char *name = array_access->data.array_access.array->data.identifier.name;
                Symbol *sym = symtab_lookup(gen->symtab, name);
                if (!sym) {
                    LOG_ERROR("Undefined variable: %s", name);
                    exit(1);
                }
                
                // Evaluate index
                char *index = codegen_expression(gen, array_access->data.array_access.index);
                
                if (sym->is_array) {
                    // Generate getelementptr to get address of array element
                    char *addr = codegen_next_temp(gen);
                    const char *llvm_type = strcmp(sym->data_type, "char") == 0 ? "i8" : "i32";
                    fprintf(gen->output, "  %s = getelementptr [%d x %s], [%d x %s]* %%%s, i32 0, i32 %s\n",
                            addr, sym->array_size, llvm_type, sym->array_size, llvm_type, sym->name, index);
                    
                    free(index);
                    return addr;
                } else if (strstr(sym->data_type, "*")) {
                    // For pointers, we need to calculate the address
                    char *ptr_value = codegen_next_temp(gen);
                    const char *llvm_base_type = "i32";
                    const char *llvm_ptr_type = "i32*";
                    
                    if (strstr(sym->data_type, "char*")) {
                        llvm_base_type = "i8";
                        llvm_ptr_type = "i8*";
                    }
                    
                    // Load the pointer
                    fprintf(gen->output, "  %s = load %s, %s* %%%s\n", 
                            ptr_value, llvm_ptr_type, llvm_ptr_type, sym->name);
                    
                    // Calculate the address using getelementptr
                    char *addr = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = getelementptr %s, %s %s, i32 %s\n",
                            addr, llvm_base_type, llvm_ptr_type, ptr_value, index);
                    
                    free(index);
                    free(ptr_value);
                    return addr;
                } else {
                    LOG_ERROR("Cannot take address of array/pointer access on non-array/pointer");
                    exit(1);
                }
            } else {
                LOG_ERROR("Cannot take address of expression");
                exit(1);
            }
        }
        
        case AST_DEREFERENCE: {
            // Dereference operator - load value from pointer
            char *ptr = codegen_expression(gen, expr->data.unary_op.operand);
            char *temp = codegen_next_temp(gen);
            
            // We need to determine the type being pointed to
            // Try to infer from the operand type
            const char *ptr_type = "i32";  // default
            const char *value_type = "i32";
            
            if (expr->data.unary_op.operand->type == AST_IDENTIFIER) {
                Symbol *sym = symtab_lookup(gen->symtab, expr->data.unary_op.operand->data.identifier.name);
                if (sym && strstr(sym->data_type, "char*")) {
                    ptr_type = "i8";
                    value_type = "i8";
                }
            }
            
            fprintf(gen->output, "  %s = load %s, %s* %s\n", temp, value_type, ptr_type, ptr);
            
            free(ptr);
            return temp;
        }
        
        case AST_UNARY_OP: {
            // Handle unary operators
            if (expr->data.unary_op.op == TOKEN_NOT) {
                char *operand = codegen_expression(gen, expr->data.unary_op.operand);
                char *temp = codegen_next_temp(gen);
                char *result = codegen_next_temp(gen);
                
                // Compare operand with 0 (logical NOT)
                fprintf(gen->output, "  %s = icmp eq i32 %s, 0\n", temp, operand);
                // Convert i1 to i32 (0 or 1)
                fprintf(gen->output, "  %s = zext i1 %s to i32\n", result, temp);
                
                free(operand);
                free(temp);
                return result;
            } else if (expr->data.unary_op.op == TOKEN_TILDE) {
                char *operand = codegen_expression(gen, expr->data.unary_op.operand);
                char *result = codegen_next_temp(gen);
                
                // Bitwise NOT - xor with -1 (all bits set)
                fprintf(gen->output, "  %s = xor i32 %s, -1\n", result, operand);
                
                free(operand);
                return result;
            } else if (expr->data.unary_op.op == TOKEN_INCREMENT || 
                      expr->data.unary_op.op == TOKEN_DECREMENT) {
                // Handle ++/-- operators
                ASTNode *operand_node = expr->data.unary_op.operand;
                char *addr = NULL;
                const char *llvm_type = "i32";
                
                // Get the address of the operand
                if (operand_node->type == AST_IDENTIFIER) {
                    Symbol *sym = symtab_lookup(gen->symtab, operand_node->data.identifier.name);
                    if (!sym) {
                        LOG_ERROR("Undefined variable: %s", operand_node->data.identifier.name);
                        exit(1);
                    }
                    addr = malloc(strlen(sym->name) + 3);
                    sprintf(addr, "%%%s", sym->name);
                    llvm_type = c_type_to_llvm_type(sym->data_type);
                } else if (operand_node->type == AST_DEREFERENCE) {
                    // *p++ or ++*p
                    addr = codegen_expression(gen, operand_node->data.unary_op.operand);
                } else if (operand_node->type == AST_ARRAY_ACCESS) {
                    // arr[i]++ or ++arr[i]
                    ASTNode *array_node = operand_node->data.array_access.array;
                    if (array_node->type != AST_IDENTIFIER) {
                        LOG_ERROR("Array access must be on an identifier");
                        exit(1);
                    }
                    Symbol *sym = symtab_lookup(gen->symtab, array_node->data.identifier.name);
                    if (!sym || !sym->is_array) {
                        LOG_ERROR("Invalid array: %s", array_node->data.identifier.name);
                        exit(1);
                    }
                    char *index = codegen_expression(gen, operand_node->data.array_access.index);
                    addr = codegen_next_temp(gen);
                    llvm_type = strcmp(sym->data_type, "char") == 0 ? "i8" : "i32";
                    fprintf(gen->output, "  %s = getelementptr [%d x %s], [%d x %s]* %%%s, i32 0, i32 %s\n",
                            addr, sym->array_size, llvm_type, sym->array_size, llvm_type, sym->name, index);
                    free(index);
                } else {
                    LOG_ERROR("Invalid operand for increment/decrement operator");
                    exit(1);
                }
                
                // Load current value
                char *old_value = codegen_next_temp(gen);
                fprintf(gen->output, "  %s = load %s, %s* %s\n", old_value, llvm_type, llvm_type, addr);
                
                // Compute new value
                char *new_value = codegen_next_temp(gen);
                const char *op = (expr->data.unary_op.op == TOKEN_INCREMENT) ? "add" : "sub";
                fprintf(gen->output, "  %s = %s %s %s, 1\n", new_value, op, llvm_type, old_value);
                
                // Store new value
                fprintf(gen->output, "  store %s %s, %s* %s\n", llvm_type, new_value, llvm_type, addr);
                
                // Return old value for postfix, new value for prefix
                char *result = expr->data.unary_op.is_prefix ? strdup(new_value) : strdup(old_value);
                
                if (operand_node->type == AST_IDENTIFIER) {
                    free(addr);
                } else if (operand_node->type == AST_ARRAY_ACCESS) {
                    free(addr);
                } else if (operand_node->type == AST_DEREFERENCE) {
                    free(addr);
                }
                free(old_value);
                free(new_value);
                
                return result;
            } else {
                LOG_ERROR("Unknown unary operator: %d", expr->data.unary_op.op);
                exit(1);
            }
        }
        
        case AST_MEMBER_ACCESS: {
            // Member access: struct.member
            if (expr->data.member_access.object->type != AST_IDENTIFIER) {
                LOG_ERROR("Member access must be on an identifier");
                exit(1);
            }
            
            const char *object_name = expr->data.member_access.object->data.identifier.name;
            const char *member_name = expr->data.member_access.member_name;
            
            Symbol *object_sym = symtab_lookup(gen->symtab, object_name);
            if (!object_sym) {
                LOG_ERROR("Undefined variable: %s", object_name);
                exit(1);
            }
            
            // For now, assume struct member access - in a full implementation 
            // we'd need to resolve the struct type and find the member offset
            char *temp = codegen_next_temp(gen);
            
            // Simple implementation: assume member is at a fixed offset
            // In a real implementation, we'd lookup the struct definition
            // and calculate the actual member offset
            fprintf(gen->output, "  %s = getelementptr %%struct.%s, %%struct.%s* %%%s, i32 0, i32 0\\n",
                    temp, "unknown", "unknown", object_name);
            fprintf(gen->output, "  %s = load i32, i32* %s\\n", temp, temp);
            
            LOG_TRACE("Generated member access: %s.%s", object_name, member_name);
            return temp;
        }
        
        case AST_SIZEOF: {
            // sizeof is a compile-time operator - return a constant
            int size = 0;
            
            if (expr->data.sizeof_op.type_name) {
                // sizeof(type)
                const char *type_name = expr->data.sizeof_op.type_name;
                if (strcmp(type_name, "int") == 0) {
                    size = 4;
                } else if (strcmp(type_name, "char") == 0) {
                    size = 1;
                } else if (strstr(type_name, "*")) {
                    // Any pointer type
                    size = 8;  // 64-bit pointers
                } else {
                    LOG_ERROR("Unknown type in sizeof: %s", type_name);
                    exit(1);
                }
            } else if (expr->data.sizeof_op.expression) {
                // sizeof(expression) - determine type of expression
                ASTNode *inner_expr = expr->data.sizeof_op.expression;
                
                if (inner_expr->type == AST_IDENTIFIER) {
                    Symbol *sym = symtab_lookup(gen->symtab, inner_expr->data.identifier.name);
                    if (!sym) {
                        LOG_ERROR("Undefined variable in sizeof: %s", inner_expr->data.identifier.name);
                        exit(1);
                    }
                    
                    if (sym->is_array) {
                        // Array size = element_size * array_size
                        int element_size = strcmp(sym->data_type, "char") == 0 ? 1 : 4;
                        size = element_size * sym->array_size;
                    } else if (strcmp(sym->data_type, "int") == 0) {
                        size = 4;
                    } else if (strcmp(sym->data_type, "char") == 0) {
                        size = 1;  // Even though we use i32 internally, char is conceptually 1 byte
                    } else if (strstr(sym->data_type, "*")) {
                        size = 8;  // Pointer
                    }
                } else if (inner_expr->type == AST_INT_LITERAL) {
                    size = 4;  // int literal
                } else if (inner_expr->type == AST_CHAR_LITERAL) {
                    size = 1;  // char literal
                } else if (inner_expr->type == AST_STRING_LITERAL) {
                    size = strlen(inner_expr->data.string_literal.value) + 1;  // +1 for null terminator
                } else if (inner_expr->type == AST_ARRAY_ACCESS) {
                    // Array access returns the element type
                    // We'd need to analyze the array type
                    size = 4;  // Assume int for now
                } else if (inner_expr->type == AST_DEREFERENCE) {
                    // Dereference returns the pointed-to type
                    size = 4;  // Assume int for now
                } else if (inner_expr->type == AST_ADDRESS_OF) {
                    size = 8;  // Address-of returns a pointer
                } else {
                    size = 4;  // Default to int size
                }
            }
            
            char *temp = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = add i32 0, %d\n", temp, size);
            LOG_TRACE("Generated sizeof: %d", size);
            return temp;
        }
        
        case AST_TERNARY: {
            // Generate code for condition
            char *cond = codegen_expression(gen, expr->data.ternary.condition);
            
            // Create labels
            char *true_label = codegen_next_label(gen, "ternary.true.");
            char *false_label = codegen_next_label(gen, "ternary.false.");
            char *end_label = codegen_next_label(gen, "ternary.end.");
            
            // Check if condition is true (non-zero)
            char *cond_bool = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", cond_bool, cond);
            fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                    cond_bool, true_label, false_label);
            
            // True branch
            fprintf(gen->output, "\n%s:\n", true_label);
            char *true_val = codegen_expression(gen, expr->data.ternary.true_expr);
            fprintf(gen->output, "  br label %%%s\n", end_label);
            
            // False branch
            fprintf(gen->output, "\n%s:\n", false_label);
            char *false_val = codegen_expression(gen, expr->data.ternary.false_expr);
            fprintf(gen->output, "  br label %%%s\n", end_label);
            
            // End - phi node to get result
            fprintf(gen->output, "\n%s:\n", end_label);
            char *result = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = phi i32 [ %s, %%%s ], [ %s, %%%s ]\n",
                    result, true_val, true_label, false_val, false_label);
            
            free(cond);
            free(cond_bool);
            free(true_val);
            free(false_val);
            free(true_label);
            free(false_label);
            free(end_label);
            
            return result;
        }
        
        case AST_CAST: {
            // Generate code for the expression to cast
            char *value = codegen_expression(gen, expr->data.cast.expression);
            char *result = codegen_next_temp(gen);
            
            // Determine source and target types
            const char *target_type = expr->data.cast.target_type;
            
            // Get the source type from the expression
            // For now, we'll assume everything is i32 unless it's a float/double literal
            bool source_is_float = (expr->data.cast.expression->type == AST_FLOAT_LITERAL);
            
            // Handle different cast types
            if (strcmp(target_type, "int") == 0) {
                if (source_is_float) {
                    // Float to int conversion
                    fprintf(gen->output, "  %s = fptosi double %s to i32\n", result, value);
                } else {
                    // Int/char to int - since we treat char as i32, this is a no-op
                    fprintf(gen->output, "  %s = add i32 0, %s\n", result, value);
                }
            } else if (strcmp(target_type, "char") == 0) {
                if (source_is_float) {
                    // Float to char: first convert to int, then mask
                    char *temp = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = fptosi double %s to i32\n", temp, value);
                    fprintf(gen->output, "  %s = and i32 %s, 255\n", result, temp);
                    free(temp);
                } else {
                    // Int to char - truncate to 8 bits by masking with 0xFF
                    fprintf(gen->output, "  %s = and i32 %s, 255\n", result, value);
                }
            } else if (strcmp(target_type, "float") == 0 || strcmp(target_type, "double") == 0) {
                if (source_is_float) {
                    // Float to float/double - just copy
                    fprintf(gen->output, "  %s = fadd double 0.0, %s\n", result, value);
                } else {
                    // Int to float/double conversion
                    fprintf(gen->output, "  %s = sitofp i32 %s to double\n", result, value);
                }
            } else if (strstr(target_type, "*")) {
                // Pointer cast
                if (strstr(target_type, "void*")) {
                    // Cast to void* - in LLVM we can use i8* as void*
                    fprintf(gen->output, "  %s = inttoptr i32 %s to i8*\n", result, value);
                } else {
                    // Other pointer casts - for char* just return the i8* value directly
                    free(result);
                    return value;  // i8* is already the right type for char*
                }
            } else if (strcmp(target_type, "void") == 0) {
                // Cast to void - this is typically an error unless it's discarding a value
                LOG_WARN("Cast to void type");
                free(result);
                result = strdup(value);
            } else {
                LOG_ERROR("Unsupported cast to type: %s", target_type);
                exit(1);
            }
            
            free(value);
            LOG_TRACE("Generated cast to %s", target_type);
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
            Symbol *sym;
            if (stmt->data.var_decl.is_static) {
                // Static variable - collect for later generation
                static int static_var_counter = 0;
                char global_name[256];
                
                // Create unique name for static variable
                snprintf(global_name, sizeof(global_name), "%s.static.%s.%d", 
                         gen->current_function_name ? gen->current_function_name : "global",
                         stmt->data.var_decl.name, static_var_counter++);
                
                // Add to static variables list
                StaticVariable *svar = malloc(sizeof(StaticVariable));
                svar->global_name = strdup(global_name);
                svar->local_name = strdup(stmt->data.var_decl.name);
                svar->type = strdup(stmt->data.var_decl.type);
                svar->has_initializer = false;
                svar->initial_value = 0;
                
                if (stmt->data.var_decl.initializer) {
                    // Static variable with initializer
                    if (stmt->data.var_decl.initializer->type == AST_INT_LITERAL) {
                        svar->has_initializer = true;
                        svar->initial_value = stmt->data.var_decl.initializer->data.int_literal.value;
                    } else if (stmt->data.var_decl.initializer->type == AST_CHAR_LITERAL) {
                        svar->has_initializer = true;
                        svar->initial_value = (int)stmt->data.var_decl.initializer->data.char_literal.value;
                    } else {
                        LOG_ERROR("Static variable initializer must be a constant");
                        exit(1);
                    }
                }
                
                // Add to linked list
                svar->next = gen->static_variables;
                gen->static_variables = svar;
                
                // Register in symbol table with a special marker for static variables
                // We'll store the global name in the symbol's data_type field temporarily
                char type_with_global[512];
                snprintf(type_with_global, sizeof(type_with_global), "%s:static:@%s", 
                         stmt->data.var_decl.type, global_name);
                sym = symtab_insert(gen->symtab, stmt->data.var_decl.name, 
                                   SYM_VARIABLE, type_with_global);
                if (!sym) {
                    LOG_ERROR("Failed to declare static variable: %s", stmt->data.var_decl.name);
                    exit(1);
                }
                
                LOG_DEBUG("Registered static variable: %s as @%s", stmt->data.var_decl.name, global_name);
                
            } else if (stmt->data.var_decl.array_size) {
                // Array declaration
                // First evaluate the array size (must be a constant for now)
                if (stmt->data.var_decl.array_size->type != AST_INT_LITERAL) {
                    LOG_ERROR("Array size must be a constant integer");
                    exit(1);
                }
                int size = stmt->data.var_decl.array_size->data.int_literal.value;
                sym = symtab_insert_array(gen->symtab, stmt->data.var_decl.name, 
                                         stmt->data.var_decl.type, size);
                if (!sym) {
                    LOG_ERROR("Failed to declare array: %s", stmt->data.var_decl.name);
                    exit(1);
                }
                sym->is_const = stmt->data.var_decl.is_const;
                
                // Allocate stack space for the array
                char *base_llvm_type = c_type_to_llvm_type(stmt->data.var_decl.type);
                fprintf(gen->output, "  %%%s = alloca [%d x %s]\n", sym->name, size, base_llvm_type);
            } else {
                // Regular variable
                sym = symtab_insert(gen->symtab, stmt->data.var_decl.name, 
                                   SYM_VARIABLE, stmt->data.var_decl.type);
                if (!sym) {
                    LOG_ERROR("Failed to declare variable: %s", stmt->data.var_decl.name);
                    exit(1);
                }
                sym->is_const = stmt->data.var_decl.is_const;
                
                // Allocate stack space for the variable
                char *llvm_type = c_type_to_llvm_type(stmt->data.var_decl.type);
                fprintf(gen->output, "  %%%s = alloca %s\n", sym->name, llvm_type);
                
                // Initialize if needed
                if (stmt->data.var_decl.initializer) {
                    char *value = codegen_expression(gen, stmt->data.var_decl.initializer);
                    
                    // Determine the type of the initializer value
                    const char *value_type = llvm_type;  // default to variable type
                    
                    // If this is a function call, the result is the return type, not the function type
                    if (stmt->data.var_decl.initializer->type == AST_FUNCTION_CALL) {
                        // Function calls return their return type (i32 for now)
                        value_type = "i32";
                    }
                    
                    // Special handling for pointer types
                    if (strstr(llvm_type, "*") && strstr(value_type, "*")) {
                        // Both are pointers, store with the variable's declared type
                        fprintf(gen->output, "  store %s %s, %s* %%%s\n", llvm_type, value, llvm_type, sym->name);
                    } else {
                        fprintf(gen->output, "  store %s %s, %s* %%%s\n", value_type, value, value_type, sym->name);
                    }
                    free(value);
                }
            }
            break;
        }
        
        case AST_RETURN_STMT: {
            char *value = codegen_expression(gen, stmt->data.return_stmt.expression);
            char *ret_llvm_type = c_type_to_llvm_type(gen->current_function_return_type);
            
            // If returning char as int, need to extend
            if (strcmp(ret_llvm_type, "i32") == 0 && 
                stmt->data.return_stmt.expression->type == AST_IDENTIFIER) {
                Symbol *sym = symtab_lookup(gen->symtab, stmt->data.return_stmt.expression->data.identifier.name);
                if (sym && strcmp(sym->data_type, "char") == 0) {
                    char *extended = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = sext i8 %s to i32\n", extended, value);
                    free(value);
                    value = extended;
                }
            }
            
            fprintf(gen->output, "  ret %s %s\n", ret_llvm_type, value);
            free(value);
            break;
        }
        
        case AST_BREAK_STMT: {
            if (!gen->current_loop_end_label) {
                LOG_ERROR("break statement outside of loop");
                exit(1);
            }
            fprintf(gen->output, "  br label %%%s\n", gen->current_loop_end_label);
            break;
        }
        
        case AST_CONTINUE_STMT: {
            if (!gen->current_loop_continue_label) {
                LOG_ERROR("continue statement outside of loop");
                exit(1);
            }
            fprintf(gen->output, "  br label %%%s\n", gen->current_loop_continue_label);
            break;
        }
        
        case AST_SWITCH_STMT: {
            // Evaluate switch expression
            char *switch_value = codegen_expression(gen, stmt->data.switch_stmt.expression);
            
            // If the switch expression is a char, we need to extend it to i32
            ASTNode *switch_expr = stmt->data.switch_stmt.expression;
            if (switch_expr->type == AST_IDENTIFIER) {
                Symbol *sym = symtab_lookup(gen->symtab, switch_expr->data.identifier.name);
                if (sym && strcmp(sym->data_type, "char") == 0) {
                    char *extended = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = sext i8 %s to i32\n", extended, switch_value);
                    free(switch_value);
                    switch_value = extended;
                }
            } else if (switch_expr->type == AST_CHAR_LITERAL) {
                // Char literals are already handled as i32 in codegen
            }
            
            // Generate labels for each case and the end
            char *end_label = codegen_next_label(gen, "switch.end.");
            char **case_labels = malloc(stmt->data.switch_stmt.case_count * sizeof(char*));
            for (int i = 0; i < stmt->data.switch_stmt.case_count; i++) {
                case_labels[i] = codegen_next_label(gen, "switch.case.");
            }
            char *default_label = stmt->data.switch_stmt.default_case ? 
                                 codegen_next_label(gen, "switch.default.") : end_label;
            
            // Save current loop end label and set it for break statements
            char *saved_end_label = gen->current_loop_end_label;
            gen->current_loop_end_label = end_label;
            
            // Generate comparison chain
            for (int i = 0; i < stmt->data.switch_stmt.case_count; i++) {
                ASTNode *case_node = stmt->data.switch_stmt.cases[i];
                
                // Case value must be a constant
                if (case_node->data.case_stmt.value->type != AST_INT_LITERAL &&
                    case_node->data.case_stmt.value->type != AST_CHAR_LITERAL) {
                    LOG_ERROR("Case value must be a constant");
                    exit(1);
                }
                
                int case_val;
                if (case_node->data.case_stmt.value->type == AST_INT_LITERAL) {
                    case_val = case_node->data.case_stmt.value->data.int_literal.value;
                } else {
                    case_val = case_node->data.case_stmt.value->data.char_literal.value;
                }
                
                char *cmp_result = codegen_next_temp(gen);
                fprintf(gen->output, "  %s = icmp eq i32 %s, %d\n", cmp_result, switch_value, case_val);
                
                char *next_label = (i < stmt->data.switch_stmt.case_count - 1) ? 
                                  codegen_next_label(gen, "switch.next.") : default_label;
                fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                       cmp_result, case_labels[i], next_label);
                
                if (i < stmt->data.switch_stmt.case_count - 1) {
                    fprintf(gen->output, "\n%s:\n", next_label);
                    free(next_label);
                }
                
                free(cmp_result);
            }
            
            // If no cases matched, jump to default or end
            if (stmt->data.switch_stmt.case_count == 0) {
                fprintf(gen->output, "  br label %%%s\n", default_label);
            }
            
            // Generate case blocks
            for (int i = 0; i < stmt->data.switch_stmt.case_count; i++) {
                fprintf(gen->output, "\n%s:\n", case_labels[i]);
                ASTNode *case_node = stmt->data.switch_stmt.cases[i];
                
                // Generate statements for this case
                for (int j = 0; j < case_node->data.case_stmt.statement_count; j++) {
                    codegen_statement(gen, case_node->data.case_stmt.statements[j]);
                }
                
                // Fall through to next case (or end if no break)
                if (i < stmt->data.switch_stmt.case_count - 1) {
                    fprintf(gen->output, "  br label %%%s\n", case_labels[i + 1]);
                } else if (stmt->data.switch_stmt.default_case) {
                    fprintf(gen->output, "  br label %%%s\n", default_label);
                } else {
                    fprintf(gen->output, "  br label %%%s\n", end_label);
                }
                
                free(case_labels[i]);
            }
            
            // Generate default block if present
            if (stmt->data.switch_stmt.default_case) {
                fprintf(gen->output, "\n%s:\n", default_label);
                ASTNode *default_node = stmt->data.switch_stmt.default_case;
                
                for (int i = 0; i < default_node->data.default_stmt.statement_count; i++) {
                    codegen_statement(gen, default_node->data.default_stmt.statements[i]);
                }
                
                fprintf(gen->output, "  br label %%%s\n", end_label);
                free(default_label);
            }
            
            // End label
            fprintf(gen->output, "\n%s:\n", end_label);
            
            // Restore outer loop end label
            gen->current_loop_end_label = saved_end_label;
            
            free(switch_value);
            free(case_labels);
            free(end_label);
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
            // Need to check if the condition is a pointer type
            if (stmt->data.if_stmt.condition->type == AST_IDENTIFIER) {
                Symbol *sym = symtab_lookup(gen->symtab, stmt->data.if_stmt.condition->data.identifier.name);
                if (sym && strstr(sym->data_type, "*")) {
                    // Pointer comparison - use ptrtoint
                    char *int_val = codegen_next_temp(gen);
                    fprintf(gen->output, "  %s = ptrtoint i8* %s to i64\n", int_val, cond_value);
                    fprintf(gen->output, "  %s = icmp ne i64 %s, 0\n", cond_bool, int_val);
                    free(int_val);
                } else {
                    fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", cond_bool, cond_value);
                }
            } else {
                fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", cond_bool, cond_value);
            }
            
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
        
        case AST_DO_WHILE_STMT: {
            char *body_label = codegen_next_label(gen, "do.body.");
            char *cond_label = codegen_next_label(gen, "do.cond.");
            char *end_label = codegen_next_label(gen, "do.end.");
            
            // Jump to body first (do-while executes body at least once)
            fprintf(gen->output, "  br label %%%s\n", body_label);
            
            // Body block
            fprintf(gen->output, "\n%s:\n", body_label);
            
            // Save outer loop labels and set current ones
            char *saved_end_label = gen->current_loop_end_label;
            char *saved_continue_label = gen->current_loop_continue_label;
            gen->current_loop_end_label = end_label;
            gen->current_loop_continue_label = cond_label;
            
            codegen_statement(gen, stmt->data.do_while_stmt.body);
            
            // Restore outer loop labels
            gen->current_loop_end_label = saved_end_label;
            gen->current_loop_continue_label = saved_continue_label;
            
            fprintf(gen->output, "  br label %%%s\n", cond_label);
            
            // Condition block
            fprintf(gen->output, "\n%s:\n", cond_label);
            char *cond_value = codegen_expression(gen, stmt->data.do_while_stmt.condition);
            char *cond_bool = codegen_next_temp(gen);
            fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", cond_bool, cond_value);
            fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                   cond_bool, body_label, end_label);
            
            // End label
            fprintf(gen->output, "\n%s:\n", end_label);
            
            free(cond_value);
            free(cond_bool);
            free(body_label);
            free(cond_label);
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
            
            // Save outer loop labels and set current ones
            char *saved_end_label = gen->current_loop_end_label;
            char *saved_continue_label = gen->current_loop_continue_label;
            gen->current_loop_end_label = end_label;
            gen->current_loop_continue_label = cond_label;
            
            codegen_statement(gen, stmt->data.while_stmt.body);
            
            // Restore outer loop labels
            gen->current_loop_end_label = saved_end_label;
            gen->current_loop_continue_label = saved_continue_label;
            
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
        
        case AST_FOR_STMT: {
            // For loop is equivalent to:
            // init;
            // while (condition) {
            //     body;
            //     update;
            // }
            
            char *cond_label = codegen_next_label(gen, "for.cond.");
            char *body_label = codegen_next_label(gen, "for.body.");
            char *update_label = codegen_next_label(gen, "for.update.");
            char *end_label = codegen_next_label(gen, "for.end.");
            
            // Generate init statement if present
            if (stmt->data.for_stmt.init) {
                if (stmt->data.for_stmt.init->type == AST_VAR_DECL) {
                    codegen_statement(gen, stmt->data.for_stmt.init);
                } else {
                    // It's an expression, evaluate and discard result
                    char *init_val = codegen_expression(gen, stmt->data.for_stmt.init);
                    free(init_val);
                }
            }
            
            // Jump to condition check
            fprintf(gen->output, "  br label %%%s\n", cond_label);
            
            // Condition block
            fprintf(gen->output, "\n%s:\n", cond_label);
            if (stmt->data.for_stmt.condition) {
                char *cond_value = codegen_expression(gen, stmt->data.for_stmt.condition);
                char *cond_bool = codegen_next_temp(gen);
                fprintf(gen->output, "  %s = icmp ne i32 %s, 0\n", cond_bool, cond_value);
                fprintf(gen->output, "  br i1 %s, label %%%s, label %%%s\n", 
                       cond_bool, body_label, end_label);
                free(cond_value);
                free(cond_bool);
            } else {
                // No condition means infinite loop
                fprintf(gen->output, "  br label %%%s\n", body_label);
            }
            
            // Body block
            fprintf(gen->output, "\n%s:\n", body_label);
            
            // Save outer loop labels and set current ones
            char *saved_end_label = gen->current_loop_end_label;
            char *saved_continue_label = gen->current_loop_continue_label;
            gen->current_loop_end_label = end_label;
            gen->current_loop_continue_label = update_label;
            
            codegen_statement(gen, stmt->data.for_stmt.body);
            
            // Restore outer loop labels
            gen->current_loop_end_label = saved_end_label;
            gen->current_loop_continue_label = saved_continue_label;
            
            fprintf(gen->output, "  br label %%%s\n", update_label);
            
            // Update block
            fprintf(gen->output, "\n%s:\n", update_label);
            if (stmt->data.for_stmt.update) {
                char *update_val = codegen_expression(gen, stmt->data.for_stmt.update);
                free(update_val);
            }
            fprintf(gen->output, "  br label %%%s\n", cond_label);
            
            // End label
            fprintf(gen->output, "\n%s:\n", end_label);
            
            free(cond_label);
            free(body_label);
            free(update_label);
            free(end_label);
            break;
        }
        
        case AST_STRUCT_DECL: {
            // Register struct in symbol table for later use
            Symbol **member_symbols = malloc(stmt->data.struct_decl.member_count * sizeof(Symbol*));
            
            for (int i = 0; i < stmt->data.struct_decl.member_count; i++) {
                ASTNode *member = stmt->data.struct_decl.members[i];
                Symbol *mem_sym = malloc(sizeof(Symbol));
                mem_sym->name = strdup(member->data.var_decl.name);
                mem_sym->type = SYM_VARIABLE;
                mem_sym->data_type = strdup(member->data.var_decl.type);
                mem_sym->is_param = false;
                mem_sym->is_array = false;
                mem_sym->array_size = 0;
                mem_sym->is_const = false;
                member_symbols[i] = mem_sym;
            }
            
            Symbol *struct_sym = symtab_insert_struct(gen->symtab, stmt->data.struct_decl.name, 
                                                     member_symbols, stmt->data.struct_decl.member_count);
            if (!struct_sym) {
                LOG_ERROR("Failed to declare struct: %s", stmt->data.struct_decl.name);
                exit(1);
            }
            
            // Generate LLVM struct type declaration
            fprintf(gen->output, "  ; struct %s definition (members: %d)\n", 
                    stmt->data.struct_decl.name, stmt->data.struct_decl.member_count);
            
            free(member_symbols);
            LOG_DEBUG("Generated struct declaration: %s", stmt->data.struct_decl.name);
            break;
        }
        
        case AST_UNION_DECL: {
            // Register union in symbol table for later use
            // Unions are similar to structs but all members share the same memory
            Symbol **member_symbols = malloc(stmt->data.struct_decl.member_count * sizeof(Symbol*));
            
            for (int i = 0; i < stmt->data.struct_decl.member_count; i++) {
                ASTNode *member = stmt->data.struct_decl.members[i];
                Symbol *mem_sym = malloc(sizeof(Symbol));
                mem_sym->name = strdup(member->data.var_decl.name);
                mem_sym->type = SYM_VARIABLE;
                mem_sym->data_type = strdup(member->data.var_decl.type);
                mem_sym->is_param = false;
                mem_sym->is_array = false;
                mem_sym->array_size = 0;
                mem_sym->is_const = false;
                member_symbols[i] = mem_sym;
            }
            
            // For now, treat unions like structs in the symbol table
            Symbol *union_sym = symtab_insert_struct(gen->symtab, stmt->data.struct_decl.name, 
                                                    member_symbols, stmt->data.struct_decl.member_count);
            if (!union_sym) {
                LOG_ERROR("Failed to declare union: %s", stmt->data.struct_decl.name);
                exit(1);
            }
            
            // Generate LLVM union type declaration (treated as struct for simplicity)
            fprintf(gen->output, "  ; union %s definition (members: %d)\n", 
                    stmt->data.struct_decl.name, stmt->data.struct_decl.member_count);
            
            free(member_symbols);
            LOG_DEBUG("Generated union declaration: %s", stmt->data.struct_decl.name);
            break;
        }
            
        default:
            LOG_ERROR("Unknown statement type in codegen: %d", stmt->type);
            exit(1);
    }
}

static void codegen_function(CodeGenerator *gen, ASTNode *func) {
    // Set current function return type and name
    gen->current_function_return_type = func->data.function.return_type;
    gen->current_function_name = func->data.function.name;
    
    // Generate function signature
    const char *ret_llvm_type = strcmp(func->data.function.return_type, "char") == 0 ? "i8" : "i32";
    const char *linkage = func->data.function.is_static ? "internal " : "";
    fprintf(gen->output, "define %s%s @%s(", linkage, ret_llvm_type, func->data.function.name);
    for (int i = 0; i < func->data.function.param_count; i++) {
        if (i > 0) fprintf(gen->output, ", ");
        const char *param_llvm_type = strcmp(func->data.function.params[i]->data.param_decl.type, "char") == 0 ? "i8" : "i32";
        fprintf(gen->output, "%s %%%s.param", param_llvm_type, func->data.function.params[i]->data.param_decl.name);
    }
    if (func->data.function.is_variadic) {
        if (func->data.function.param_count > 0) fprintf(gen->output, ", ");
        fprintf(gen->output, "...");
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
            const char *param_llvm_type = strcmp(param->data.param_decl.type, "char") == 0 ? "i8" : "i32";
            fprintf(gen->output, "  %%%s = alloca %s\n", param->data.param_decl.name, param_llvm_type);
            fprintf(gen->output, "  store %s %%%s.param, %s* %%%s\n", 
                    param_llvm_type, param->data.param_decl.name, param_llvm_type, param->data.param_decl.name);
        }
    }
    
    // Generate function body
    codegen_statement(gen, func->data.function.body);
    
    // Only add unreachable if the last instruction wasn't a terminator
    // This is a simple heuristic - in production you'd track this properly
    if (strcmp(ret_llvm_type, "i8") == 0) {
        fprintf(gen->output, "  ret i8 0  ; default return\n");
    } else {
        fprintf(gen->output, "  ret i32 0  ; default return\n");
    }
    fprintf(gen->output, "}\n\n");
    
    // Restore global scope
    symtab_destroy(gen->symtab);
    gen->symtab = old_symtab;
    
    // Clear current function name
    gen->current_function_name = NULL;
    
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
    
    // Use generic target triple that works on multiple platforms
    // The specific target will be determined by the LLVM tools at compile time
    fprintf(gen->output, "\n");
    
    // Declare external functions
    fprintf(gen->output, "declare i32 @putchar(i32)\n");
    fprintf(gen->output, "declare i32 @getchar()\n");
    fprintf(gen->output, "declare i32 @puts(i8*)\n");
    fprintf(gen->output, "declare i32 @printf(i8*, ...)\n");
    fprintf(gen->output, "declare i8* @malloc(i64)\n");
    fprintf(gen->output, "declare void @free(i8*)\n");
    fprintf(gen->output, "declare void @exit(i32)\n");
    fprintf(gen->output, "declare i32 @atoi(i8*)\n");
    fprintf(gen->output, "declare i64 @strlen(i8*)\n");
    fprintf(gen->output, "declare i8* @strcpy(i8*, i8*)\n");
    fprintf(gen->output, "declare i32 @strcmp(i8*, i8*)\n");
    fprintf(gen->output, "declare i8* @strcat(i8*, i8*)\n");
    fprintf(gen->output, "declare i8* @memcpy(i8*, i8*, i64)\n");
    fprintf(gen->output, "declare i8* @memset(i8*, i32, i64)\n\n");
    
    // Create global symbol table
    gen->symtab = symtab_create(NULL);
    
    // First pass: generate global variables
    for (int i = 0; i < ast->data.program.global_var_count; i++) {
        ASTNode *var = ast->data.program.global_vars[i];
        
        // Register global variable in symbol table
        Symbol *sym = symtab_insert(gen->symtab, var->data.var_decl.name, 
                                   SYM_VARIABLE, var->data.var_decl.type);
        if (!sym) {
            LOG_ERROR("Failed to declare global variable: %s", var->data.var_decl.name);
            exit(1);
        }
        
        // Check if it's an array and set the appropriate flags
        if (var->data.var_decl.array_size) {
            sym->is_array = true;
            if (var->data.var_decl.array_size->type == AST_INT_LITERAL) {
                sym->array_size = var->data.var_decl.array_size->data.int_literal.value;
            }
        }
        
        // Generate global variable declaration
        char *llvm_type = c_type_to_llvm_type(var->data.var_decl.type);
        
        if (var->data.var_decl.array_size) {
            // Global array declaration
            int array_size = 0;
            if (var->data.var_decl.array_size->type == AST_INT_LITERAL) {
                array_size = var->data.var_decl.array_size->data.int_literal.value;
            }
            fprintf(gen->output, "@%s = global [%d x %s] zeroinitializer\n", 
                    var->data.var_decl.name, array_size, llvm_type);
        } else if (var->data.var_decl.initializer) {
            // Global variable with initializer
            // For now, only support constant initializers
            if (var->data.var_decl.initializer->type == AST_INT_LITERAL) {
                fprintf(gen->output, "@%s = global %s %d\n", 
                        var->data.var_decl.name, llvm_type, 
                        var->data.var_decl.initializer->data.int_literal.value);
            } else if (var->data.var_decl.initializer->type == AST_CHAR_LITERAL) {
                fprintf(gen->output, "@%s = global %s %d\n", 
                        var->data.var_decl.name, llvm_type, 
                        (int)var->data.var_decl.initializer->data.char_literal.value);
            } else {
                LOG_ERROR("Global variable initializer must be a constant");
                exit(1);
            }
        } else {
            // Global variable without initializer (zero-initialized)
            fprintf(gen->output, "@%s = global %s 0\n", var->data.var_decl.name, llvm_type);
        }
        
        LOG_DEBUG("Generated global variable: %s", var->data.var_decl.name);
    }
    
    if (ast->data.program.global_var_count > 0) {
        fprintf(gen->output, "\n");
    }
    
    // Second pass: register all functions
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
                              func->data.function.param_count,
                              func->data.function.is_variadic);
        
        free(param_types);
        free(param_names);
    }
    
    // Process enums - store them in the code generator for later lookup
    gen->enum_count = ast->data.program.enum_count;
    if (gen->enum_count > 0) {
        gen->enums = malloc(gen->enum_count * sizeof(ASTNode*));
        for (int i = 0; i < gen->enum_count; i++) {
            gen->enums[i] = ast->data.program.enums[i];
        }
    }
    
    // Second pass: generate code for each function
    for (int i = 0; i < ast->data.program.function_count; i++) {
        codegen_function(gen, ast->data.program.functions[i]);
    }
    
    // Emit static variables after all functions
    emit_static_variables(gen);
    
    // Emit string literals at the end
    emit_string_literals(gen);
    
    // Clean up global symbol table
    symtab_destroy(gen->symtab);
    gen->symtab = NULL;
    
    LOG_INFO("Code generation complete");
}
