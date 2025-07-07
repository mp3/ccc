# CCC Compiler Documentation

Welcome to the comprehensive documentation for the CCC compiler. This documentation covers everything from basic usage to advanced development topics.

## Documentation Overview

### For Users

- **[Quick Reference](QUICK_REFERENCE.md)** - One-page reference card
  - Command-line syntax
  - Operator precedence table
  - Common patterns
  - Quick debugging tips

- **[User Guide](USER_GUIDE.md)** - Getting started, language features, and examples
  - Installation and setup
  - Command-line usage
  - Language feature tutorials
  - Common patterns and examples
  - Debugging tips

- **[Features](FEATURES.md)** - Detailed reference of all implemented features
  - Complete feature list with examples
  - Language constructs and syntax
  - Built-in functions and operators
  - Implementation limits and restrictions

### For Developers

- **[Architecture](ARCHITECTURE.md)** - Internal design and structure
  - Component overview
  - Data flow and compilation phases
  - Memory management
  - Extension points

- **[Development Guide](DEVELOPMENT.md)** - Contributing and extending the compiler
  - Development setup
  - Adding new features
  - Testing guidelines
  - Debugging techniques
  - Code style and best practices

## Quick Links

### Getting Started
1. [Installation](USER_GUIDE.md#installation)
2. [Your First Program](USER_GUIDE.md#your-first-program)
3. [Basic Language Features](USER_GUIDE.md#language-features-guide)

### Feature Reference
1. [Data Types](FEATURES.md#data-types)
2. [Operators](FEATURES.md#operators)
3. [Control Flow](FEATURES.md#control-flow)
4. [Functions](FEATURES.md#functions)

### Development
1. [Adding a New Feature](DEVELOPMENT.md#adding-new-features)
2. [Testing](DEVELOPMENT.md#testing-guidelines)
3. [Debugging](DEVELOPMENT.md#debugging-tips)

## Documentation Map

```
docs/
├── README.md           # This file - Documentation index
├── QUICK_REFERENCE.md  # One-page quick reference
├── USER_GUIDE.md       # User-focused guide
├── FEATURES.md         # Complete feature reference
├── ARCHITECTURE.md     # Compiler architecture
└── DEVELOPMENT.md      # Developer guide
```

## Example Programs

The `samples/` directory contains example programs demonstrating various features:

- `hello.c` - Basic hello world
- `factorial.c` - Recursive functions
- `pointer_test.c` - Pointer operations
- `struct_test.c` - Structures and unions
- `optimize_test.c` - Optimization examples

## Test Suite

The `tests/` directory contains comprehensive test cases:

- `test_lexer.py` - Lexer unit tests
- `test_arrays.py` - Array functionality tests
- `test_pointers.py` - Pointer operation tests
- `test_strings.py` - String literal tests
- `test_function_pointers.py` - Function pointer tests

## Getting Help

1. **Check the documentation** - Most questions are answered in the guides
2. **Look at examples** - The samples/ directory has working examples
3. **Read the tests** - Test files show expected behavior
4. **Debug with -v** - Use verbose mode to see what's happening
5. **Check the logs** - `ccc.log` contains detailed execution traces

## Contributing

See the [Development Guide](DEVELOPMENT.md#contributing-guidelines) for information on:
- Setting up a development environment
- Code style guidelines
- Testing requirements
- Pull request process

## Version History

The compiler implements a substantial subset of C89 with some modern conveniences:

### Currently Implemented
- Basic types (int, char)
- All arithmetic, logical, and bitwise operators
- Complete control flow (if/else, loops, switch)
- Functions with parameters and return values
- Arrays and pointers with full arithmetic
- Structures and unions
- Enumerations
- Function pointers
- Variadic functions (basic support)
- Static variables
- Type definitions (typedef)
- Const qualifier
- Sizeof operator
- Type casting

### Not Yet Implemented
- Floating-point types
- Preprocessor
- Standard library (except putchar/getchar)
- Multiple translation units
- Advanced type qualifiers (volatile, restrict)
- Variable-length arrays
- Inline functions

## License and Credits

This compiler was developed as an educational project for learning compiler construction techniques. See the main README.md for license information.