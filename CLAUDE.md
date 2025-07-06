# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CCC is a C89 subset compiler that generates LLVM IR. It implements a recursive descent parser and supports a growing subset of C features including functions, control flow, arrays, pointers, structs, and various operators.

## Build and Run Commands

```bash
# Build the compiler
make

# Compile a C file to LLVM IR
./ccc input.c -o output.ll

# Compile LLVM IR to executable (macOS/Linux)
clang output.ll -o output

# Run with different optimization levels
./ccc -O0 input.c -o output.ll  # No optimization
./ccc -O1 input.c -o output.ll  # Default optimization
./ccc -O2 input.c -o output.ll  # All optimizations

# Run tests
make test  # Runs pytest on tests directory

# Clean build artifacts
make clean
```

## Architecture Overview

The compiler follows a traditional multi-pass architecture:

1. **Lexer** (`src/lexer.[ch]`): Tokenizes input into a stream of tokens. Each token has a type, text representation, and source location.

2. **Parser** (`src/parser.[ch]`): Recursive descent parser that builds an Abstract Syntax Tree (AST). The parser defines precedence through the call hierarchy:
   - `parse_expression` → `parse_assignment` → `parse_logical_or` → `parse_logical_and` → `parse_comparison` → `parse_additive` → `parse_multiplicative` → `parse_primary`

3. **Symbol Table** (`src/symtab.[ch]`): Hierarchical symbol tables for nested scopes. Tracks variables, functions, arrays, and structs with their types, offsets, and other metadata.

4. **Code Generator** (`src/codegen.[ch]`): Traverses the AST and emits LLVM IR. Key responsibilities:
   - Manages temporary variables and labels
   - Handles type conversions (e.g., char to i32)
   - Implements short-circuit evaluation for logical operators
   - Tracks loop context for break/continue statements

5. **Optimizer** (`src/optimizer.[ch]`): Optional optimization passes:
   - Constant folding: Evaluates constant expressions at compile time
   - Dead code elimination: Removes unreachable code

## Key Implementation Details

### Adding New Features

When implementing new language features:

1. **Tokens**: Add to `TokenType` enum in `lexer.h` and update `token_type_to_string` array
2. **AST Nodes**: Add to `ASTNodeType` enum and union in `parser.h`
3. **Parser**: Add parsing logic, typically in `parse_statement` or `parse_primary`
4. **AST Destruction**: Add cleanup in `ast_destroy` function
5. **Code Generation**: Add case in `codegen_expression` or `codegen_statement`
6. **Optimizer**: Add optimization support if applicable

### Testing Strategy

- Sample programs in `samples/` directory demonstrate each feature
- Python-based test suite in `tests/` directory uses pytest
- Tests compile sample programs and verify exit codes match expected values

### Current Implementation Status

Implemented:
- Basic types (int, char), arrays, pointers, structs
- All arithmetic, comparison, and logical operators
- Control flow: if/else, while, do-while, for, switch/case
- Functions with parameters and return values
- break/continue statements
- sizeof operator
- Basic I/O (putchar/getchar as external functions)

Not yet implemented:
- Global variables (partial implementation exists)
- Bitwise operators
- Compound assignments (+=, -=, etc.)
- Increment/decrement operators (++, --)
- Type casting
- Preprocessor directives