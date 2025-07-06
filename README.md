# ccc - A Small C Compiler

A minimal C89 subset compiler that generates LLVM IR.

## Features Implemented

- **Lexer**: Tokenizes C source code
- **Parser**: Recursive descent parser for a subset of C
- **Code Generator**: Produces LLVM IR
- **Logger**: Comprehensive logging system for debugging

### Currently Supported

- Integer literals
- Arithmetic operators: `+`, `-`, `*`, `/`
- Comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Functions (only `int main()` for now)
- Return statements
- Proper operator precedence
- Variable declarations with optional initialization
- Variable assignments
- Expression statements
- If statements with optional else clause
- While loops

## Building

```bash
make
```

## Usage

```bash
./ccc input.c -o output.ll
```

Then compile the LLVM IR to an executable:
```bash
llc output.ll -filetype=obj -o output.o
clang output.o -o output
./output
```

## Example

```c
int main() {
    int n = 5;
    int factorial = 1;
    int i = 1;
    
    while (i <= n) {
        factorial = factorial * i;
        i = i + 1;
    }
    
    if (factorial == 120) {
        return 1;  // Success
    } else {
        return 0;  // Failure
    }
}
```

## Project Structure

```
ccc/
├── src/
│   ├── main.c      # Entry point
│   ├── lexer.[ch]  # Tokenization
│   ├── parser.[ch] # AST construction
│   ├── codegen.[ch]# LLVM IR generation
│   └── logger.[ch] # Logging utilities
├── tests/          # Test suite
├── samples/        # Example C programs
└── Makefile
```

## Next Steps

- [x] Variables and assignments
- [x] Control flow (if/while)
- [x] Comparison operators
- [ ] Function parameters and local variables
- [ ] Simple optimizations
- [ ] Self-hosting capability

## Extensions (Future)

- Structs and pointers
- Arrays
- More data types (char, float, etc.)
- Standard library functions
- Preprocessor directives