#ifndef ERROR_H
#define ERROR_H

#include <stdbool.h>
#include <stdarg.h>
#include "lexer.h"

// Error types
typedef enum {
    ERROR_NONE = 0,
    ERROR_SYNTAX,
    ERROR_SEMANTIC,
    ERROR_TYPE,
    ERROR_UNDEFINED,
    ERROR_REDEFINITION,
    ERROR_INTERNAL
} ErrorType;

// Error severity levels
typedef enum {
    SEVERITY_ERROR,
    SEVERITY_WARNING,
    SEVERITY_NOTE
} ErrorSeverity;

// Error context for better error messages
typedef struct ErrorContext {
    const char *filename;
    const char *source_line;
    int line;
    int column;
    int length;  // Length of the error span
} ErrorContext;

// Error information
typedef struct Error {
    ErrorType type;
    ErrorSeverity severity;
    char *message;
    char *hint;  // Optional hint for fixing the error
    ErrorContext context;
    struct Error *next;
} Error;

// Error manager
typedef struct ErrorManager {
    Error *errors;
    Error *last_error;
    int error_count;
    int warning_count;
    bool error_recovery;  // Whether to attempt error recovery
    int max_errors;       // Maximum errors before stopping
} ErrorManager;

// Create and destroy error manager
ErrorManager *error_manager_create(void);
void error_manager_destroy(ErrorManager *manager);

// Report errors with context
void error_report(ErrorManager *manager, ErrorType type, ErrorSeverity severity,
                 const ErrorContext *context, const char *format, ...);

// Report errors with hints
void error_report_with_hint(ErrorManager *manager, ErrorType type, ErrorSeverity severity,
                           const ErrorContext *context, const char *hint,
                           const char *format, ...);

// Helper functions for common errors
void error_syntax(ErrorManager *manager, const ErrorContext *context,
                 const char *expected, const char *found);

void error_undefined_variable(ErrorManager *manager, const ErrorContext *context,
                             const char *var_name);

void error_undefined_function(ErrorManager *manager, const ErrorContext *context,
                             const char *func_name);

void error_type_mismatch(ErrorManager *manager, const ErrorContext *context,
                        const char *expected_type, const char *actual_type);

void error_redefinition(ErrorManager *manager, const ErrorContext *context,
                       const char *name, const char *previous_location);

void error_argument_count(ErrorManager *manager, const ErrorContext *context,
                         const char *func_name, int expected, int actual);

// Print all errors
void error_print_all(ErrorManager *manager);

// Check if compilation should continue
bool error_should_continue(ErrorManager *manager);

// Create error context from current position
ErrorContext error_context_from_token(const char *filename, Token *token);

#endif // ERROR_H
