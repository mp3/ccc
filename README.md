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
- Multiple function definitions
- Function parameters
- Function calls with arguments
- Return statements
- Proper operator precedence
- Variable declarations with optional initialization
- Variable assignments
- Expression statements
- If statements with optional else clause
- While loops
- Recursive functions

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
int factorial(int n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 5;
    int y = 3;
    int sum = add(x, y);
    int fact = factorial(5);
    
    return sum + fact;  // 8 + 120 = 128
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
- [x] Function parameters and local variables
- [ ] Simple optimizations (constant folding, dead code elimination)
- [ ] Self-hosting capability

## Extensions (Future)

- Structs and pointers
- Arrays
- More data types (char, float, etc.)
- Standard library functions
- Preprocessor directives