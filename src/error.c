#include "error.h"
#include "logger.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// ANSI color codes for error messages
#define COLOR_RED     "\033[91m"
#define COLOR_YELLOW  "\033[93m"
#define COLOR_BLUE    "\033[94m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_RESET   "\033[0m"

ErrorManager *error_manager_create(void) {
    ErrorManager *manager = calloc(1, sizeof(ErrorManager));
    manager->error_recovery = true;
    manager->max_errors = 50;  // Stop after 50 errors
    return manager;
}

void error_manager_destroy(ErrorManager *manager) {
    if (!manager) return;
    
    Error *current = manager->errors;
    while (current) {
        Error *next = current->next;
        free(current->message);
        free(current->hint);
        free(current);
        current = next;
    }
    
    free(manager);
}

static Error *create_error(ErrorType type, ErrorSeverity severity,
                          const ErrorContext *context, const char *message) {
    Error *error = calloc(1, sizeof(Error));
    error->type = type;
    error->severity = severity;
    error->message = strdup(message);
    if (context) {
        error->context = *context;
    }
    return error;
}

static void add_error(ErrorManager *manager, Error *error) {
    if (!manager->errors) {
        manager->errors = error;
        manager->last_error = error;
    } else {
        manager->last_error->next = error;
        manager->last_error = error;
    }
    
    if (error->severity == SEVERITY_ERROR) {
        manager->error_count++;
    } else if (error->severity == SEVERITY_WARNING) {
        manager->warning_count++;
    }
}

void error_report(ErrorManager *manager, ErrorType type, ErrorSeverity severity,
                 const ErrorContext *context, const char *format, ...) {
    char buffer[1024];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Error *error = create_error(type, severity, context, buffer);
    add_error(manager, error);
}

void error_report_with_hint(ErrorManager *manager, ErrorType type, ErrorSeverity severity,
                           const ErrorContext *context, const char *hint,
                           const char *format, ...) {
    char buffer[1024];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    Error *error = create_error(type, severity, context, buffer);
    if (hint) {
        error->hint = strdup(hint);
    }
    add_error(manager, error);
}

void error_syntax(ErrorManager *manager, const ErrorContext *context,
                 const char *expected, const char *found) {
    error_report_with_hint(manager, ERROR_SYNTAX, SEVERITY_ERROR, context,
                          "Check for missing semicolons or typos",
                          "expected %s, but found %s", expected, found);
}

void error_undefined_variable(ErrorManager *manager, const ErrorContext *context,
                             const char *var_name) {
    error_report_with_hint(manager, ERROR_UNDEFINED, SEVERITY_ERROR, context,
                          "Did you forget to declare the variable?",
                          "undefined variable '%s'", var_name);
}

void error_undefined_function(ErrorManager *manager, const ErrorContext *context,
                             const char *func_name) {
    error_report_with_hint(manager, ERROR_UNDEFINED, SEVERITY_ERROR, context,
                          "Check if the function is defined before use",
                          "undefined function '%s'", func_name);
}

void error_type_mismatch(ErrorManager *manager, const ErrorContext *context,
                        const char *expected_type, const char *actual_type) {
    error_report_with_hint(manager, ERROR_TYPE, SEVERITY_ERROR, context,
                          "Consider using a type cast or check variable types",
                          "type mismatch: expected '%s', got '%s'", 
                          expected_type, actual_type);
}

void error_redefinition(ErrorManager *manager, const ErrorContext *context,
                       const char *name, const char *previous_location) {
    error_report_with_hint(manager, ERROR_REDEFINITION, SEVERITY_ERROR, context,
                          "Use a different name or remove the duplicate",
                          "redefinition of '%s' (previously defined at %s)", 
                          name, previous_location);
}

void error_argument_count(ErrorManager *manager, const ErrorContext *context,
                         const char *func_name, int expected, int actual) {
    char hint[256];
    if (expected < actual) {
        snprintf(hint, sizeof(hint), "Remove %d argument(s)", actual - expected);
    } else {
        snprintf(hint, sizeof(hint), "Add %d more argument(s)", expected - actual);
    }
    
    error_report_with_hint(manager, ERROR_SEMANTIC, SEVERITY_ERROR, context, hint,
                          "function '%s' expects %d argument(s), but %d provided",
                          func_name, expected, actual);
}

static void print_error_location(const ErrorContext *context) {
    if (!context->filename) return;
    
    // Print file location
    fprintf(stderr, "%s%s:%d:%d:%s ", 
            COLOR_BOLD, context->filename, context->line, context->column, COLOR_RESET);
}

static void print_error_line(const ErrorContext *context) {
    if (!context->source_line) return;
    
    // Print the source line
    fprintf(stderr, "  %s\n", context->source_line);
    
    // Print error marker
    fprintf(stderr, "  ");
    for (int i = 0; i < context->column - 1; i++) {
        fprintf(stderr, " ");
    }
    
    fprintf(stderr, COLOR_RED "^");
    for (int i = 1; i < context->length; i++) {
        fprintf(stderr, "~");
    }
    fprintf(stderr, COLOR_RESET "\n");
}

static const char *severity_string(ErrorSeverity severity) {
    switch (severity) {
        case SEVERITY_ERROR:   return COLOR_RED "error" COLOR_RESET;
        case SEVERITY_WARNING: return COLOR_YELLOW "warning" COLOR_RESET;
        case SEVERITY_NOTE:    return COLOR_BLUE "note" COLOR_RESET;
        default:               return "unknown";
    }
}

void error_print_all(ErrorManager *manager) {
    if (!manager || !manager->errors) return;
    
    Error *current = manager->errors;
    while (current) {
        // Print error header
        print_error_location(&current->context);
        fprintf(stderr, "%s: %s%s%s\n", 
                severity_string(current->severity),
                COLOR_BOLD, current->message, COLOR_RESET);
        
        // Print source line and error marker
        print_error_line(&current->context);
        
        // Print hint if available
        if (current->hint) {
            fprintf(stderr, "  %shint:%s %s\n", COLOR_BLUE, COLOR_RESET, current->hint);
        }
        
        fprintf(stderr, "\n");
        current = current->next;
    }
    
    // Print summary
    if (manager->error_count > 0 || manager->warning_count > 0) {
        fprintf(stderr, "%s: ", COLOR_BOLD);
        
        if (manager->error_count > 0) {
            fprintf(stderr, "%d error%s", 
                    manager->error_count, 
                    manager->error_count == 1 ? "" : "s");
        }
        
        if (manager->error_count > 0 && manager->warning_count > 0) {
            fprintf(stderr, " and ");
        }
        
        if (manager->warning_count > 0) {
            fprintf(stderr, "%d warning%s", 
                    manager->warning_count,
                    manager->warning_count == 1 ? "" : "s");
        }
        
        fprintf(stderr, " generated%s\n", COLOR_RESET);
    }
}

bool error_should_continue(ErrorManager *manager) {
    if (!manager->error_recovery) {
        return manager->error_count == 0;
    }
    
    return manager->error_count < manager->max_errors;
}

ErrorContext error_context_from_token(const char *filename, Token *token) {
    ErrorContext context = {0};
    
    if (filename) {
        context.filename = filename;
    }
    
    if (token) {
        context.line = token->line;
        context.column = token->column;
        context.length = token->text ? strlen(token->text) : 1;
    }
    
    return context;
}
