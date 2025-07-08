# Error Handling Demo

The CCC compiler now includes improved error handling with:

## Features Implemented

1. **Categorized Error Types**
   - Syntax errors
   - Semantic errors  
   - Type errors
   - Undefined variable/function errors
   - Redefinition errors

2. **Error Severity Levels**
   - Errors (prevent compilation)
   - Warnings (allow compilation to continue)
   - Notes (additional information)

3. **Formatted Error Messages**
   - ANSI color codes for better readability
   - File location with line and column numbers
   - Helpful hints for common errors
   - Clear error descriptions

4. **Basic Error Recovery**
   - Missing semicolon recovery (detects when next token suggests new statement)
   - Graceful error reporting without immediate exit
   - Error count tracking

## Example Error Output

When compiling test_error_messages.c with missing semicolons:

```
tests/test_error_messages.c:6:4: error: expected SEMICOLON, but found INT
  hint: Check for missing semicolons or typos

tests/test_error_messages.c:18:14: error: expected RPAREN, but found LBRACE
  hint: Check for missing semicolons or typos

: 2 errors generated
```

## Implementation Details

- **error.h/error.c**: Core error handling infrastructure
- **parser.c**: Integrated error manager and basic recovery for missing semicolons
- **main.c**: Handles parsing failures gracefully

The error handling system provides a foundation for future enhancements like:
- More sophisticated error recovery strategies
- Source line display with error highlighting
- Error limits and recovery synchronization
- Warnings for common mistakes