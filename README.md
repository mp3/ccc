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
- Functions (only `int main()` for now)
- Return statements
- Proper operator precedence
- Variable declarations with optional initialization
- Variable assignments
- Expression statements

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
    int x = 10;
    int y = 20;
    int sum = x + y;
    return sum;
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
- [ ] Control flow (if/while)
- [ ] Comparison operators
- [ ] Function parameters and local variables
- [ ] Simple optimizations
- [ ] Self-hosting capability

## Extensions (Future)

- Structs and pointers
- Arrays
- More data types (char, float, etc.)
- Standard library functions
- Preprocessor directives