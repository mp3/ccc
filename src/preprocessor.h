#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <stdio.h>
#include <stdbool.h>

// Forward declarations
typedef struct Preprocessor Preprocessor;
typedef struct IncludePath IncludePath;
typedef struct MacroDefinition MacroDefinition;

// Include path node for linked list
struct IncludePath {
    char *path;
    IncludePath *next;
};

// Macro definition structure
struct MacroDefinition {
    char *name;
    char *value;
    char **params;  // NULL for object-like macros
    int param_count;
    MacroDefinition *next;
};

// Main preprocessor structure
struct Preprocessor {
    FILE *output;               // Output stream for preprocessed code
    char *current_file;         // Current file being processed
    int current_line;           // Current line number
    IncludePath *include_paths; // List of include directories
    MacroDefinition *macros;    // List of defined macros
    bool in_include;           // Currently processing an include
    int include_depth;         // Nesting level of includes
};

// Preprocessor API
Preprocessor *preprocessor_create(void);
void preprocessor_destroy(Preprocessor *pp);

// Add include search paths
void preprocessor_add_include_path(Preprocessor *pp, const char *path);
void preprocessor_add_system_includes(Preprocessor *pp);

// Process a file
int preprocessor_process_file(Preprocessor *pp, const char *input_file, const char *output_file);

// Macro handling
void preprocessor_define_macro(Preprocessor *pp, const char *name, const char *value);
void preprocessor_undefine_macro(Preprocessor *pp, const char *name);
bool preprocessor_is_macro_defined(Preprocessor *pp, const char *name);

#endif // PREPROCESSOR_H
