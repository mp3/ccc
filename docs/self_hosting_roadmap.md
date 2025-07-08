# Self-Hosting Roadmap for CCC Compiler

## Overview

Self-hosting means the CCC compiler can compile its own source code. This is a significant milestone that demonstrates the compiler's maturity and completeness.

## Current Status

The CCC compiler has made substantial progress with:
- ✅ Basic C89 syntax support
- ✅ Functions, control flow, arrays, pointers, structs
- ✅ Error handling and warnings
- ✅ Optimization framework
- ✅ LLVM IR code generation

However, many features required for self-hosting are missing.

## Missing Features Analysis

### 1. Critical Path Features (Required)

#### Preprocessor (High Priority)
- `#include` directive support
- `#define` macros (object-like and function-like)
- `#ifndef/#define/#endif` header guards
- `#ifdef/#else/#endif` conditional compilation
- Macro expansion
- Built-in macros (`__FILE__`, `__LINE__`)

#### Standard Library Integration
- Header file declarations
- External function declarations
- Linking with libc functions:
  - Memory: `malloc`, `free`, `calloc`, `realloc`
  - String: `strcmp`, `strcpy`, `strlen`, `strdup`
  - I/O: `printf`, `fprintf`, `fopen`, `fclose`
  - Character: `isdigit`, `isalpha`, `isspace`

#### Global Variables (Partially Implemented)
- Complete global variable support
- Static global variables
- Initialized globals
- String literal globals

#### Variadic Functions
- Function declarations with `...`
- `va_list`, `va_start`, `va_end` support
- Calling variadic functions

### 2. Important Features (Highly Desirable)

#### Missing Operators
- Bitwise operators: `&`, `|`, `^`, `~`, `<<`, `>>`
- Compound assignments: `+=`, `-=`, `*=`, `/=`
- Increment/decrement: `++`, `--`
- Ternary operator: `? :`
- Modulo operator: `%`

#### Type System Enhancements
- `typedef` declarations
- `enum` support
- `union` support (partially implemented)
- Type qualifiers: `const`, `volatile`
- Storage classes: `static`, `extern`, `register`, `auto`

### 3. Nice-to-Have Features

- Function pointers
- Complex initializers
- Bit fields
- `inline` functions (C99)
- Variable-length arrays (C99)

## Implementation Strategy

### Phase 1: Minimal Preprocessor
1. Implement `#include` with simple file inclusion
2. Add `#define` for simple object-like macros
3. Support header guards (`#ifndef/#define/#endif`)
4. Handle system vs local includes (`<>` vs `""`)

### Phase 2: Standard Library Bridge
1. Create minimal header files for required functions
2. Implement external function declarations
3. Ensure proper linking with system libc

### Phase 3: Complete Global Variables
1. Finish global variable implementation
2. Add static storage class support
3. Support initialized globals with all types

### Phase 4: Essential Operators
1. Implement bitwise operators
2. Add compound assignment operators
3. Implement increment/decrement operators

### Phase 5: Variadic Functions
1. Parse variadic function declarations
2. Implement va_list mechanism
3. Support variadic function calls

## Minimal Self-Hosting Path

A minimal path to self-hosting could involve:

1. **Preprocessor Stub**: Manually preprocess the source files
2. **Library Stubs**: Create declarations for required libc functions
3. **Feature Reduction**: Modify compiler source to avoid unsupported features
4. **Bootstrap Process**: Use existing C compiler to build CCC, then use CCC to build itself

## Estimated Effort

Based on the analysis:
- Preprocessor: 2-4 weeks
- Standard library integration: 1-2 weeks
- Global variables completion: 1 week
- Missing operators: 2-3 weeks
- Variadic functions: 2-3 weeks
- Testing and debugging: 2-4 weeks

**Total estimated effort**: 10-18 weeks for full self-hosting capability

## Alternative Approach: Limited Self-Hosting

Create a simplified version of CCC that:
1. Uses a subset of C features
2. Manually preprocessed source files
3. Links against minimal runtime
4. Demonstrates self-compilation capability

This could be achieved in 4-6 weeks and would serve as a proof of concept.

## Conclusion

While full self-hosting is a significant undertaking, the CCC compiler has a solid foundation. The most practical approach would be to implement features incrementally, testing self-compilation capability at each stage. Starting with a limited self-hosting demonstration would provide valuable insights and motivation for completing the full implementation.