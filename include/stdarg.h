#ifndef STDARG_H
#define STDARG_H

// Variadic argument support for CCC compiler
// This is a minimal implementation that generates appropriate LLVM IR

// va_list is typically a pointer to the variadic arguments
typedef char* va_list;

// va_start initializes the va_list to point after the last named parameter
#define va_start(ap, last) __builtin_va_start(ap, last)

// va_arg retrieves the next argument of the specified type
#define va_arg(ap, type) __builtin_va_arg(ap, type)

// va_end cleans up (no-op in our simple implementation)
#define va_end(ap) __builtin_va_end(ap)

// va_copy duplicates a va_list
#define va_copy(dest, src) __builtin_va_copy(dest, src)

#endif // STDARG_H