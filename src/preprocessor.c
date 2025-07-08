#include "preprocessor.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#define MAX_INCLUDE_DEPTH 64
#define MAX_LINE_LENGTH 4096
#define MAX_IFDEF_DEPTH 64

// Create a new preprocessor instance
Preprocessor *preprocessor_create(void) {
    Preprocessor *pp = calloc(1, sizeof(Preprocessor));
    if (!pp) {
        LOG_ERROR("Failed to allocate preprocessor");
        return NULL;
    }
    
    pp->include_depth = 0;
    pp->in_include = false;
    pp->cond_depth = 0;
    
    // Initialize the base conditional state (always active)
    pp->cond_stack[0].active = true;
    pp->cond_stack[0].has_else = false;
    pp->cond_stack[0].ever_true = true;
    
    // Add some predefined macros
    preprocessor_define_macro(pp, "__CCC__", "1");
    preprocessor_define_macro(pp, "__STDC__", "1");
    preprocessor_define_macro(pp, "__STDC_VERSION__", "199901L");
    
    LOG_DEBUG("Created preprocessor");
    return pp;
}

// Destroy preprocessor and free resources
void preprocessor_destroy(Preprocessor *pp) {
    if (!pp) return;
    
    // Free include paths
    IncludePath *path = pp->include_paths;
    while (path) {
        IncludePath *next = path->next;
        free(path->path);
        free(path);
        path = next;
    }
    
    // Free macros
    MacroDefinition *macro = pp->macros;
    while (macro) {
        MacroDefinition *next = macro->next;
        free(macro->name);
        free(macro->value);
        if (macro->params) {
            for (int i = 0; i < macro->param_count; i++) {
                free(macro->params[i]);
            }
            free(macro->params);
        }
        free(macro);
        macro = next;
    }
    
    free(pp->current_file);
    if (pp->output && pp->output != stdout) {
        fclose(pp->output);
    }
    
    free(pp);
    LOG_DEBUG("Destroyed preprocessor");
}

// Add an include search path
void preprocessor_add_include_path(Preprocessor *pp, const char *path) {
    IncludePath *new_path = malloc(sizeof(IncludePath));
    if (!new_path) {
        LOG_ERROR("Failed to allocate include path");
        return;
    }
    
    new_path->path = strdup(path);
    new_path->next = NULL;
    
    // Add to end of list to maintain order
    if (!pp->include_paths) {
        pp->include_paths = new_path;
    } else {
        IncludePath *current = pp->include_paths;
        while (current->next) {
            current = current->next;
        }
        current->next = new_path;
    }
    
    LOG_DEBUG("Added include path: %s", path);
}

// Add standard system include paths
void preprocessor_add_system_includes(Preprocessor *pp) {
    // Common system include paths (simplified for now)
    preprocessor_add_include_path(pp, "/usr/include");
    preprocessor_add_include_path(pp, "/usr/local/include");
    
    // Current directory is always searched first for quoted includes
    preprocessor_add_include_path(pp, ".");
}

// Find a file in include paths
static char *find_include_file(Preprocessor *pp, const char *filename, bool is_system) {
    struct stat st;
    static char full_path[MAX_LINE_LENGTH];
    
    // For quoted includes, first check relative to current file
    if (!is_system && pp->current_file) {
        // Get directory of current file
        char *last_slash = strrchr(pp->current_file, '/');
        if (last_slash) {
            size_t dir_len = last_slash - pp->current_file;
            strncpy(full_path, pp->current_file, dir_len);
            full_path[dir_len] = '\0';
            strcat(full_path, "/");
            strcat(full_path, filename);
            
            if (stat(full_path, &st) == 0) {
                return strdup(full_path);
            }
        }
    }
    
    // Search in include paths
    IncludePath *path = pp->include_paths;
    while (path) {
        snprintf(full_path, sizeof(full_path), "%s/%s", path->path, filename);
        if (stat(full_path, &st) == 0) {
            return strdup(full_path);
        }
        path = path->next;
    }
    
    return NULL;
}

// Skip whitespace
static void skip_whitespace(const char **p) {
    while (**p && isspace(**p)) {
        (*p)++;
    }
}

// Forward declarations
static void process_file_internal(Preprocessor *pp, FILE *input);
static char *expand_macros(Preprocessor *pp, const char *line);
static bool is_active_conditional(Preprocessor *pp);

// Parse a preprocessor directive
static void process_directive(Preprocessor *pp, const char *line, FILE *input __attribute__((unused))) {
    const char *p = line + 1; // Skip '#'
    skip_whitespace(&p);
    
    if (strncmp(p, "include", 7) == 0) {
        p += 7;
        skip_whitespace(&p);
        
        char filename[MAX_LINE_LENGTH];
        bool is_system = false;
        
        if (*p == '<') {
            // System include
            is_system = true;
            p++;
            const char *end = strchr(p, '>');
            if (!end) {
                LOG_ERROR("%s:%d: unterminated include directive", 
                         pp->current_file, pp->current_line);
                return;
            }
            size_t len = end - p;
            strncpy(filename, p, len);
            filename[len] = '\0';
        } else if (*p == '"') {
            // Local include
            p++;
            const char *end = strchr(p, '"');
            if (!end) {
                LOG_ERROR("%s:%d: unterminated include directive", 
                         pp->current_file, pp->current_line);
                return;
            }
            size_t len = end - p;
            strncpy(filename, p, len);
            filename[len] = '\0';
        } else {
            LOG_ERROR("%s:%d: invalid include directive", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        // Check include depth
        if (pp->include_depth >= MAX_INCLUDE_DEPTH) {
            LOG_ERROR("%s:%d: include depth exceeded", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        // Find the file
        char *full_path = find_include_file(pp, filename, is_system);
        if (!full_path) {
            LOG_ERROR("%s:%d: cannot find include file: %s", 
                     pp->current_file, pp->current_line, filename);
            return;
        }
        
        // Process the included file
        char *saved_file = pp->current_file;
        int saved_line = pp->current_line;
        int saved_depth = pp->include_depth;
        
        pp->current_file = full_path;
        pp->current_line = 0;
        pp->include_depth++;
        
        FILE *inc_file = fopen(full_path, "r");
        if (!inc_file) {
            LOG_ERROR("%s:%d: cannot open include file: %s", 
                     pp->current_file, pp->current_line, filename);
            pp->current_file = saved_file;
            pp->current_line = saved_line;
            pp->include_depth = saved_depth;
            free(full_path);
            return;
        }
        
        // Emit line directive for debugging
        fprintf(pp->output, "# 1 \"%s\"\n", full_path);
        
        // Process the included file
        process_file_internal(pp, inc_file);
        
        fclose(inc_file);
        
        // Restore context
        pp->current_file = saved_file;
        pp->current_line = saved_line;
        pp->include_depth = saved_depth;
        free(full_path);
        
        // Emit line directive to return to original file
        fprintf(pp->output, "# %d \"%s\"\n", saved_line + 1, saved_file);
        
    } else if (strncmp(p, "define", 6) == 0) {
        p += 6;
        skip_whitespace(&p);
        
        // Parse macro name
        const char *name_start = p;
        while (*p && (isalnum(*p) || *p == '_')) {
            p++;
        }
        
        if (p == name_start) {
            LOG_ERROR("%s:%d: invalid macro name", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        size_t name_len = p - name_start;
        char *name = strndup(name_start, name_len);
        
        // Check if it's a function-like macro
        if (*p == '(') {
            // Function-like macro - parse parameters
            p++; // Skip '('
            
            // Parse parameter list
            char *params[32];
            int param_count = 0;
            
            skip_whitespace(&p);
            if (*p != ')') {
                // Parse parameters
                while (*p && *p != ')' && param_count < 32) {
                    skip_whitespace(&p);
                    
                    const char *param_start = p;
                    while (*p && (isalnum(*p) || *p == '_')) {
                        p++;
                    }
                    
                    if (p > param_start) {
                        size_t param_len = p - param_start;
                        params[param_count] = strndup(param_start, param_len);
                        param_count++;
                    }
                    
                    skip_whitespace(&p);
                    if (*p == ',') {
                        p++;
                    } else if (*p != ')') {
                        LOG_ERROR("%s:%d: invalid macro parameter list", 
                                 pp->current_file, pp->current_line);
                        // Clean up
                        for (int i = 0; i < param_count; i++) {
                            free(params[i]);
                        }
                        free(name);
                        return;
                    }
                }
            }
            
            if (*p != ')') {
                LOG_ERROR("%s:%d: unterminated macro parameter list", 
                         pp->current_file, pp->current_line);
                // Clean up
                for (int i = 0; i < param_count; i++) {
                    free(params[i]);
                }
                free(name);
                return;
            }
            p++; // Skip ')'
            
            // Get the macro body
            skip_whitespace(&p);
            char *value = strdup(p);
            
            // Remove trailing whitespace
            char *end = value + strlen(value) - 1;
            while (end > value && isspace(*end)) {
                *end-- = '\0';
            }
            
            // Create and define the macro with parameters
            preprocessor_define_function_macro(pp, name, params, param_count, value);
            
            // Clean up
            for (int i = 0; i < param_count; i++) {
                free(params[i]);
            }
            free(value);
        } else {
            // Object-like macro
            skip_whitespace(&p);
            char *value = strdup(p);
            
            // Remove trailing whitespace
            char *end = value + strlen(value) - 1;
            while (end > value && isspace(*end)) {
                *end-- = '\0';
            }
            
            preprocessor_define_macro(pp, name, value);
            
            free(value);
        }
        
        free(name);
        
    } else if (strncmp(p, "undef", 5) == 0) {
        p += 5;
        skip_whitespace(&p);
        
        const char *name_start = p;
        while (*p && (isalnum(*p) || *p == '_')) {
            p++;
        }
        
        if (p == name_start) {
            LOG_ERROR("%s:%d: invalid macro name", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        size_t name_len = p - name_start;
        char *name = strndup(name_start, name_len);
        
        preprocessor_undefine_macro(pp, name);
        
        free(name);
        
    } else if (strncmp(p, "ifdef", 5) == 0) {
        p += 5;
        skip_whitespace(&p);
        
        // Parse macro name
        const char *name_start = p;
        while (*p && (isalnum(*p) || *p == '_')) {
            p++;
        }
        
        if (p == name_start) {
            LOG_ERROR("%s:%d: invalid macro name in #ifdef", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        size_t name_len = p - name_start;
        char *name = strndup(name_start, name_len);
        
        // Check if we can nest deeper
        if (pp->cond_depth >= MAX_IFDEF_DEPTH - 1) {
            LOG_ERROR("%s:%d: conditional nesting too deep", 
                     pp->current_file, pp->current_line);
            free(name);
            return;
        }
        
        // Push new conditional state
        pp->cond_depth++;
        bool macro_defined = preprocessor_is_macro_defined(pp, name);
        bool parent_active = pp->cond_stack[pp->cond_depth - 1].active;
        
        pp->cond_stack[pp->cond_depth].active = parent_active && macro_defined;
        pp->cond_stack[pp->cond_depth].has_else = false;
        pp->cond_stack[pp->cond_depth].ever_true = pp->cond_stack[pp->cond_depth].active;
        
        LOG_DEBUG("#ifdef %s: defined=%d, active=%d", name, macro_defined, 
                  pp->cond_stack[pp->cond_depth].active);
        
        free(name);
        
    } else if (strncmp(p, "ifndef", 6) == 0) {
        p += 6;
        skip_whitespace(&p);
        
        // Parse macro name
        const char *name_start = p;
        while (*p && (isalnum(*p) || *p == '_')) {
            p++;
        }
        
        if (p == name_start) {
            LOG_ERROR("%s:%d: invalid macro name in #ifndef", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        size_t name_len = p - name_start;
        char *name = strndup(name_start, name_len);
        
        // Check if we can nest deeper
        if (pp->cond_depth >= MAX_IFDEF_DEPTH - 1) {
            LOG_ERROR("%s:%d: conditional nesting too deep", 
                     pp->current_file, pp->current_line);
            free(name);
            return;
        }
        
        // Push new conditional state
        pp->cond_depth++;
        bool macro_defined = preprocessor_is_macro_defined(pp, name);
        bool parent_active = pp->cond_stack[pp->cond_depth - 1].active;
        
        pp->cond_stack[pp->cond_depth].active = parent_active && !macro_defined;
        pp->cond_stack[pp->cond_depth].has_else = false;
        pp->cond_stack[pp->cond_depth].ever_true = pp->cond_stack[pp->cond_depth].active;
        
        LOG_DEBUG("#ifndef %s: defined=%d, active=%d", name, macro_defined, 
                  pp->cond_stack[pp->cond_depth].active);
        
        free(name);
        
    } else if (strncmp(p, "else", 4) == 0 && !isalnum(p[4])) {
        if (pp->cond_depth == 0) {
            LOG_ERROR("%s:%d: #else without #if", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        if (pp->cond_stack[pp->cond_depth].has_else) {
            LOG_ERROR("%s:%d: multiple #else directives", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        pp->cond_stack[pp->cond_depth].has_else = true;
        bool parent_active = pp->cond_stack[pp->cond_depth - 1].active;
        bool was_true = pp->cond_stack[pp->cond_depth].ever_true;
        
        pp->cond_stack[pp->cond_depth].active = parent_active && !was_true;
        if (pp->cond_stack[pp->cond_depth].active) {
            pp->cond_stack[pp->cond_depth].ever_true = true;
        }
        
        LOG_DEBUG("#else: active=%d", pp->cond_stack[pp->cond_depth].active);
        
    } else if (strncmp(p, "endif", 5) == 0) {
        if (pp->cond_depth == 0) {
            LOG_ERROR("%s:%d: #endif without #if", 
                     pp->current_file, pp->current_line);
            return;
        }
        
        LOG_DEBUG("#endif: leaving depth %d", pp->cond_depth);
        pp->cond_depth--;
    } else {
        // Unknown directive, pass through
        LOG_WARN("Unknown preprocessor directive: %s", line);
        fprintf(pp->output, "%s\n", line);
    }
}

// Expand macros in a line
static char *expand_macros(Preprocessor *pp, const char *line) {
    static char expanded[MAX_LINE_LENGTH * 2];  // Allow for expansion
    char temp[MAX_LINE_LENGTH];
    const char *src = line;
    char *dst = expanded;
    
    while (*src && (dst - expanded) < MAX_LINE_LENGTH * 2 - 1) {
        // Skip whitespace
        if (isspace(*src)) {
            *dst++ = *src++;
            continue;
        }
        
        // Check if we're starting an identifier
        if (isalpha(*src) || *src == '_') {
            const char *id_start = src;
            char *temp_dst = temp;
            
            // Collect the identifier
            while ((isalnum(*src) || *src == '_') && (temp_dst - temp) < MAX_LINE_LENGTH - 1) {
                *temp_dst++ = *src++;
            }
            *temp_dst = '\0';
            
            // Check if it's a macro
            MacroDefinition *macro = pp->macros;
            bool found = false;
            while (macro) {
                if (strcmp(macro->name, temp) == 0) {
                    // Found a macro
                    if (macro->params == NULL) {
                        // Object-like macro - simple expansion
                        const char *val = macro->value;
                        while (*val && (dst - expanded) < MAX_LINE_LENGTH * 2 - 1) {
                            *dst++ = *val++;
                        }
                        found = true;
                        break;
                    } else {
                        // Function-like macro - check for parentheses
                        const char *saved_src = src;
                        while (isspace(*src)) src++;
                        
                        if (*src == '(') {
                            // Parse arguments
                            src++; // Skip '('
                            char *args[32];
                            int arg_count = 0;
                            
                            // Parse each argument
                            while (*src && *src != ')' && arg_count < 32) {
                                while (isspace(*src)) src++;
                                
                                // Collect argument until comma or closing paren
                                char arg_buf[MAX_LINE_LENGTH];
                                char *arg_dst = arg_buf;
                                int paren_depth = 0;
                                
                                while (*src && (paren_depth > 0 || (*src != ',' && *src != ')'))) {
                                    if (*src == '(') paren_depth++;
                                    else if (*src == ')') paren_depth--;
                                    
                                    if ((arg_dst - arg_buf) < MAX_LINE_LENGTH - 1) {
                                        *arg_dst++ = *src;
                                    }
                                    src++;
                                }
                                *arg_dst = '\0';
                                
                                // Trim trailing whitespace
                                while (arg_dst > arg_buf && isspace(*(arg_dst - 1))) {
                                    *(--arg_dst) = '\0';
                                }
                                
                                if (strlen(arg_buf) > 0 || arg_count > 0 || macro->param_count > 0) {
                                    args[arg_count] = strdup(arg_buf);
                                    arg_count++;
                                }
                                
                                if (*src == ',') {
                                    src++;
                                }
                            }
                            
                            if (*src == ')') {
                                src++; // Skip ')'
                                
                                // Check argument count
                                if (arg_count != macro->param_count) {
                                    LOG_WARN("Macro %s expects %d arguments, got %d", 
                                             macro->name, macro->param_count, arg_count);
                                    // Copy unexpanded
                                    const char *copy = id_start;
                                    while (copy < src && (dst - expanded) < MAX_LINE_LENGTH * 2 - 1) {
                                        *dst++ = *copy++;
                                    }
                                } else {
                                    // Expand the macro with argument substitution
                                    const char *body = macro->value;
                                    while (*body && (dst - expanded) < MAX_LINE_LENGTH * 2 - 1) {
                                        // Check if we're at a parameter name
                                        if (isalpha(*body) || *body == '_') {
                                            const char *param_start = body;
                                            char param_name[MAX_LINE_LENGTH];
                                            char *param_dst = param_name;
                                            
                                            while ((isalnum(*body) || *body == '_') && 
                                                   (param_dst - param_name) < MAX_LINE_LENGTH - 1) {
                                                *param_dst++ = *body++;
                                            }
                                            *param_dst = '\0';
                                            
                                            // Check if it's a parameter
                                            bool param_found = false;
                                            for (int i = 0; i < macro->param_count; i++) {
                                                if (strcmp(param_name, macro->params[i]) == 0) {
                                                    // Substitute with argument
                                                    const char *arg = args[i];
                                                    while (*arg && (dst - expanded) < MAX_LINE_LENGTH * 2 - 1) {
                                                        *dst++ = *arg++;
                                                    }
                                                    param_found = true;
                                                    break;
                                                }
                                            }
                                            
                                            if (!param_found) {
                                                // Not a parameter, copy as-is
                                                const char *copy = param_start;
                                                while (copy < body && (dst - expanded) < MAX_LINE_LENGTH * 2 - 1) {
                                                    *dst++ = *copy++;
                                                }
                                            }
                                        } else {
                                            // Not an identifier, copy as-is
                                            *dst++ = *body++;
                                        }
                                    }
                                }
                                
                                // Clean up arguments
                                for (int i = 0; i < arg_count; i++) {
                                    free(args[i]);
                                }
                                
                                found = true;
                                break;
                            }
                        } else {
                            // Function-like macro but no parentheses - don't expand
                            src = saved_src;
                        }
                    }
                }
                macro = macro->next;
            }
            
            // If not a macro, copy the identifier as-is
            if (!found) {
                const char *copy = id_start;
                while (copy < src && (dst - expanded) < MAX_LINE_LENGTH * 2 - 1) {
                    *dst++ = *copy++;
                }
            }
        } else {
            // Not an identifier, copy as-is
            *dst++ = *src++;
        }
    }
    
    *dst = '\0';
    return expanded;
}

// Check if we're in an active conditional branch
static bool is_active_conditional(Preprocessor *pp) {
    return pp->cond_stack[pp->cond_depth].active;
}

// Process a file (internal helper)
static void process_file_internal(Preprocessor *pp, FILE *input) {
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), input)) {
        pp->current_line++;
        
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        // Check for preprocessor directive
        const char *p = line;
        skip_whitespace(&p);
        
        if (*p == '#') {
            process_directive(pp, p, input);
        } else if (is_active_conditional(pp)) {
            // Only output lines in active conditional branches
            // Perform macro expansion
            char *expanded = expand_macros(pp, line);
            fprintf(pp->output, "%s\n", expanded);
        }
        // else: skip line in inactive conditional branch
    }
}

// Process a file
int preprocessor_process_file(Preprocessor *pp, const char *input_file, const char *output_file) {
    FILE *input = fopen(input_file, "r");
    if (!input) {
        LOG_ERROR("Cannot open input file: %s", input_file);
        return -1;
    }
    
    if (output_file) {
        pp->output = fopen(output_file, "w");
        if (!pp->output) {
            LOG_ERROR("Cannot open output file: %s", output_file);
            fclose(input);
            return -1;
        }
    } else {
        pp->output = stdout;
    }
    
    pp->current_file = strdup(input_file);
    pp->current_line = 0;
    
    // Emit initial line directive
    fprintf(pp->output, "# 1 \"%s\"\n", input_file);
    
    process_file_internal(pp, input);
    
    // Check for unmatched conditionals
    if (pp->cond_depth > 0) {
        LOG_ERROR("%s: unmatched #if/#ifdef/#ifndef (depth=%d)", 
                  input_file, pp->cond_depth);
        fclose(input);
        if (pp->output != stdout) {
            fclose(pp->output);
            pp->output = NULL;
        }
        return -1;
    }
    
    fclose(input);
    if (pp->output != stdout) {
        fclose(pp->output);
        pp->output = NULL;
    }
    
    LOG_INFO("Preprocessed %s", input_file);
    return 0;
}

// Define a function-like macro
void preprocessor_define_function_macro(Preprocessor *pp, const char *name, 
                                       char **params, int param_count, const char *value) {
    // Check if already defined
    MacroDefinition *existing = pp->macros;
    while (existing) {
        if (strcmp(existing->name, name) == 0) {
            // Redefine - free old params
            if (existing->params) {
                for (int i = 0; i < existing->param_count; i++) {
                    free(existing->params[i]);
                }
                free(existing->params);
            }
            free(existing->value);
            
            // Set new params
            existing->param_count = param_count;
            if (param_count > 0) {
                existing->params = malloc(param_count * sizeof(char*));
                for (int i = 0; i < param_count; i++) {
                    existing->params[i] = strdup(params[i]);
                }
            } else {
                existing->params = NULL;
            }
            existing->value = strdup(value);
            
            LOG_DEBUG("Redefined function macro: %s(%d params) = %s", 
                      name, param_count, value);
            return;
        }
        existing = existing->next;
    }
    
    // Create new macro
    MacroDefinition *macro = calloc(1, sizeof(MacroDefinition));
    if (!macro) {
        LOG_ERROR("Failed to allocate macro");
        return;
    }
    
    macro->name = strdup(name);
    macro->value = strdup(value);
    macro->param_count = param_count;
    
    if (param_count > 0) {
        macro->params = malloc(param_count * sizeof(char*));
        for (int i = 0; i < param_count; i++) {
            macro->params[i] = strdup(params[i]);
        }
    } else {
        macro->params = NULL;
    }
    
    macro->next = pp->macros;
    pp->macros = macro;
    
    LOG_DEBUG("Defined function macro: %s(%d params) = %s", name, param_count, value);
}

// Define a macro
void preprocessor_define_macro(Preprocessor *pp, const char *name, const char *value) {
    // Check if already defined
    MacroDefinition *existing = pp->macros;
    while (existing) {
        if (strcmp(existing->name, name) == 0) {
            // Redefine
            free(existing->value);
            existing->value = strdup(value);
            LOG_DEBUG("Redefined macro: %s = %s", name, value);
            return;
        }
        existing = existing->next;
    }
    
    // Create new macro
    MacroDefinition *macro = calloc(1, sizeof(MacroDefinition));
    if (!macro) {
        LOG_ERROR("Failed to allocate macro");
        return;
    }
    
    macro->name = strdup(name);
    macro->value = strdup(value);
    macro->next = pp->macros;
    pp->macros = macro;
    
    LOG_DEBUG("Defined macro: %s = %s", name, value);
}

// Undefine a macro
void preprocessor_undefine_macro(Preprocessor *pp, const char *name) {
    MacroDefinition **current = &pp->macros;
    while (*current) {
        if (strcmp((*current)->name, name) == 0) {
            MacroDefinition *to_remove = *current;
            *current = (*current)->next;
            
            free(to_remove->name);
            free(to_remove->value);
            if (to_remove->params) {
                for (int i = 0; i < to_remove->param_count; i++) {
                    free(to_remove->params[i]);
                }
                free(to_remove->params);
            }
            free(to_remove);
            
            LOG_DEBUG("Undefined macro: %s", name);
            return;
        }
        current = &(*current)->next;
    }
}

// Check if a macro is defined
bool preprocessor_is_macro_defined(Preprocessor *pp, const char *name) {
    MacroDefinition *macro = pp->macros;
    while (macro) {
        if (strcmp(macro->name, name) == 0) {
            return true;
        }
        macro = macro->next;
    }
    return false;
}
