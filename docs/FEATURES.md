# CCC Compiler - Comprehensive Feature Documentation

This document provides detailed information about all features implemented in the CCC compiler.

## Table of Contents

1. [Data Types](#data-types)
2. [Operators](#operators)
3. [Control Flow](#control-flow)
4. [Functions](#functions)
5. [Storage Classes](#storage-classes)
6. [Type System](#type-system)
7. [I/O Operations](#io-operations)
8. [Compiler Optimizations](#compiler-optimizations)
9. [Implementation Details](#implementation-details)

## Data Types

### Integer Types

The compiler supports 32-bit signed integers (`int`).

```c
int x = 42;
int y = -17;
int z = 0;
```

**LLVM IR Generation:**
- Integers are represented as `i32` in LLVM IR
- Integer literals are directly embedded in the IR

### Character Types

The compiler supports 8-bit characters (`char`) with full ASCII support.

```c
char c = 'A';
char newline = '\n';
char tab = '\t';
char quote = '\"';
```

**Features:**
- Single character literals with single quotes
- Escape sequences: `\n`, `\t`, `\r`, `\\`, `\'`, `\"`
- Characters are represented as `i8` in LLVM IR

### String Literals

String literals are implemented as null-terminated character arrays.

```c
char *str = "Hello, World!";
char *escaped = "Line 1\nLine 2\tTabbed";
```

**Implementation:**
- Strings are stored as global constants in LLVM IR
- Automatic null-termination
- Support for escape sequences within strings

### Arrays

Both single and multi-dimensional arrays are supported.

```c
int arr[10];
int matrix[3][4];
char buffer[256];
```

**Features:**
- Fixed-size arrays only (no VLAs)
- Array indexing with bounds determined at compile time
- Arrays decay to pointers when passed to functions

### Pointers

Full pointer support including arithmetic and multiple levels of indirection.

```c
int *p;
int **pp;
char *str;
void (*func_ptr)(int);
```

**Features:**
- Address-of operator (`&`)
- Dereference operator (`*`)
- Pointer arithmetic with proper type scaling
- Null pointer constant

### Structures

User-defined structures with member access.

```c
struct Point {
    int x;
    int y;
};

struct Point p;
p.x = 10;
p.y = 20;
```

**Features:**
- Nested structures
- Structure pointers with `->` operator
- Proper memory layout and alignment

### Unions

Unions with shared memory layout for members.

```c
union Data {
    int i;
    char c;
    float f;  // Note: float not fully supported yet
};
```

**Features:**
- All members share the same memory location
- Size is determined by the largest member

### Enumerations

Enumeration types with automatic value assignment.

```c
enum Color {
    RED,      // 0
    GREEN,    // 1
    BLUE = 5, // 5
    YELLOW    // 6
};
```

**Features:**
- Auto-incrementing values
- Explicit value assignment
- Enums are treated as integers in expressions

### Function Pointers

Pointers to functions with full type checking.

```c
int (*operation)(int, int);
operation = &add;
int result = operation(5, 3);
```

**Features:**
- Function pointer declarations
- Assignment from function names
- Calling through function pointers
- Type checking for parameters and return types

## Operators

### Arithmetic Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `+` | Addition | `a + b` |
| `-` | Subtraction | `a - b` |
| `*` | Multiplication | `a * b` |
| `/` | Division | `a / b` |
| `%` | Modulo | `a % b` |

**Implementation Notes:**
- Integer division truncates towards zero
- Division by zero is undefined behavior
- Modulo operator works only with integers

### Comparison Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `==` | Equal to | `a == b` |
| `!=` | Not equal to | `a != b` |
| `<` | Less than | `a < b` |
| `>` | Greater than | `a > b` |
| `<=` | Less than or equal | `a <= b` |
| `>=` | Greater than or equal | `a >= b` |

**Implementation Notes:**
- Comparison results are 0 (false) or 1 (true)
- Pointer comparisons are supported

### Logical Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `&&` | Logical AND | `a && b` |
| `||` | Logical OR | `a || b` |
| `!` | Logical NOT | `!a` |

**Features:**
- Short-circuit evaluation
- Any non-zero value is considered true
- Results are always 0 or 1

### Bitwise Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `&` | Bitwise AND | `a & b` |
| `|` | Bitwise OR | `a | b` |
| `^` | Bitwise XOR | `a ^ b` |
| `~` | Bitwise NOT | `~a` |
| `<<` | Left shift | `a << b` |
| `>>` | Right shift | `a >> b` |

**Implementation Notes:**
- Shifts by negative amounts are undefined
- Right shift is arithmetic (sign-extending) for signed integers

### Assignment Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `=` | Simple assignment | `a = b` |
| `+=` | Add and assign | `a += b` |
| `-=` | Subtract and assign | `a -= b` |
| `*=` | Multiply and assign | `a *= b` |
| `/=` | Divide and assign | `a /= b` |

**Implementation Notes:**
- Assignment expressions return the assigned value
- Compound assignments are expanded to full expressions

### Increment/Decrement Operators

| Operator | Description | Example |
|----------|-------------|---------|
| `++` | Pre-increment | `++a` |
| `++` | Post-increment | `a++` |
| `--` | Pre-decrement | `--a` |
| `--` | Post-decrement | `a--` |

**Implementation Notes:**
- Pre-increment/decrement returns the new value
- Post-increment/decrement returns the old value
- Works with pointers (scaled by pointed-to type size)

### Other Operators

#### Address and Dereference
- `&` - Address-of operator: gets the address of a variable
- `*` - Dereference operator: accesses value at a pointer

#### Member Access
- `.` - Direct member access for structures/unions
- `->` - Indirect member access through pointers

#### Ternary Conditional
```c
int max = (a > b) ? a : b;
```

#### Comma Operator
```c
int x = (a = 5, b = 10, a + b);  // x = 15
```

#### Cast Operator
```c
int i = (int)3.14;  // Note: float literals not fully supported
char c = (char)65;  // c = 'A'
```

#### Sizeof Operator
```c
int size = sizeof(int);        // 4
int arr_size = sizeof(arr);    // array size in bytes
```

## Control Flow

### If/Else Statements

```c
if (condition) {
    // then block
} else if (another_condition) {
    // else if block
} else {
    // else block
}
```

**Features:**
- Nested if statements
- Dangling else handled correctly
- Conditions converted to boolean (0 = false, non-zero = true)

### While Loops

```c
while (condition) {
    // loop body
}
```

**Features:**
- Condition checked before each iteration
- Support for break and continue
- Empty body allowed

### Do-While Loops

```c
do {
    // loop body
} while (condition);
```

**Features:**
- Body executes at least once
- Condition checked after each iteration
- Support for break and continue

### For Loops

```c
for (int i = 0; i < 10; i++) {
    // loop body
}
```

**Features:**
- All three parts (init, condition, update) are optional
- Variable declarations in init section
- Support for break and continue

### Switch Statements

```c
switch (expression) {
    case 1:
        // code
        break;
    case 2:
    case 3:
        // code for 2 or 3
        break;
    default:
        // default code
}
```

**Features:**
- Integer and character expressions
- Fall-through behavior
- Multiple case labels
- Optional default case
- Break statement to exit switch

### Jump Statements

#### Break
- Exits the innermost loop or switch statement

#### Continue
- Skips to the next iteration of the innermost loop

#### Return
- Returns from a function with optional value
```c
return;        // void functions
return expr;   // non-void functions
```

## Functions

### Function Definitions

```c
int add(int a, int b) {
    return a + b;
}

void print_message(char *msg) {
    // implementation
}
```

**Features:**
- Parameter passing by value
- Local variables with function scope
- Automatic return type checking

### Function Calls

```c
int result = add(5, 3);
print_message("Hello");
```

**Features:**
- Argument evaluation order (right-to-left)
- Type checking for arguments
- Implicit function declarations not supported

### Recursive Functions

```c
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
```

**Features:**
- Full recursion support
- Stack-based calling convention

### Variadic Functions

```c
int printf_like(const char *format, ...) {
    // Note: stdarg.h macros not implemented
    // Manual argument handling required
}
```

**Features:**
- Ellipsis (`...`) syntax
- Minimum of one named parameter required
- No built-in va_list support

### Static Variables

```c
void counter() {
    static int count = 0;
    count++;
}
```

**Features:**
- Persistent storage across function calls
- Initialized only once
- Function-scoped static variables

## Storage Classes

### Global Variables

```c
int global_var = 100;
char *global_str = "Global";
```

**Features:**
- File scope visibility
- Zero-initialized if no initializer
- Accessible from all functions

### Local Variables

```c
void function() {
    int local_var = 42;
    {
        int block_var = 10;  // Block scope
    }
}
```

**Features:**
- Function or block scope
- Automatic storage duration
- Uninitialized values are undefined

### Static Variables

```c
static int file_static = 0;  // File scope

void func() {
    static int func_static = 0;  // Function scope
}
```

**Features:**
- Static storage duration
- File-scope statics have internal linkage
- Function-scope statics persist across calls

## Type System

### Type Qualifiers

#### Const
```c
const int x = 42;        // Cannot modify x
const int *p;            // Pointer to const int
int * const q;           // Const pointer to int
```

**Features:**
- Read-only variables
- Compile-time error on modification attempts
- Const-correctness checking

### Type Definitions

```c
typedef int Integer;
typedef struct Point {
    int x, y;
} Point;
typedef int (*BinaryOp)(int, int);
```

**Features:**
- Creates type aliases
- Simplifies complex type declarations
- Scoped like variables

### Type Conversions

```c
int i = 65;
char c = (char)i;        // Explicit cast
int x = c;               // Implicit conversion
```

**Features:**
- Explicit casting with cast operator
- Implicit conversions in assignments
- Pointer casting supported

## I/O Operations

### Character I/O

```c
int c = getchar();       // Read one character
putchar('A');            // Write one character
```

**Features:**
- Built-in functions (no headers needed)
- EOF handling with getchar()
- Direct LLVM intrinsic mapping

## Compiler Optimizations

### Constant Folding

The compiler evaluates constant expressions at compile time.

```c
int x = 2 + 3 * 4;       // Computed as 14 at compile time
int y = 100 / 5 - 10;    // Computed as 10 at compile time
```

**Supported Operations:**
- Arithmetic: `+`, `-`, `*`, `/`
- Comparison: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical: `&&`, `||`, `!`

### Dead Code Elimination

Removes unreachable code based on constant conditions.

```c
if (0) {
    // This code is removed
}

while (0) {
    // This loop is removed
}
```

**Features:**
- If statements with constant conditions
- While loops with constant false conditions
- Preserves side effects in condition evaluation

### Optimization Levels

```bash
ccc input.c -O0  # No optimization
ccc input.c -O1  # Default optimizations
ccc input.c -O2  # More aggressive optimizations
```

## Implementation Details

### Lexical Analysis

The lexer (`lexer.c`) tokenizes the source code into:
- Keywords: `int`, `char`, `if`, `while`, etc.
- Identifiers: Variable and function names
- Literals: Integer, character, and string constants
- Operators: All arithmetic, logical, and bitwise operators
- Delimiters: Parentheses, braces, semicolons, etc.

### Parsing

The parser (`parser.c`) uses recursive descent parsing:
- Operator precedence parsing for expressions
- Proper associativity handling
- Syntax error reporting with line/column information

### Symbol Table

The symbol table (`symtab.c`) manages:
- Hierarchical scope management
- Type information storage
- Function signatures
- Static variable tracking

### Code Generation

The code generator (`codegen.c`) produces LLVM IR:
- SSA form with automatic temporary generation
- Type-correct LLVM instructions
- Proper function calling conventions
- String literal management

### AST Optimization

The optimizer (`optimizer.c`) performs:
- AST-level constant folding
- Dead code elimination
- Optimization statistics tracking

## Limitations

### Not Implemented
- Floating-point types (`float`, `double`)
- Other integer types (`short`, `long`, `unsigned`)
- Preprocessor directives (`#include`, `#define`, etc.)
- Standard library functions (except `putchar`/`getchar`)
- Multiple translation units
- Inline assembly
- Variable-length arrays
- Complex initializers for arrays/structs
- Bit fields in structures
- Function prototypes without definitions
- External linkage specifications

### Known Restrictions
- Single source file compilation only
- Limited to 32-bit integers and 8-bit characters
- No standard headers available
- Manual memory management required (no malloc/free)
- Variadic functions require manual argument extraction

## Error Handling

The compiler provides error messages for:
- Syntax errors with line/column information
- Type mismatches
- Undefined variables/functions
- Invalid operations
- Constant expression violations

Example:
```
Error: Expected SEMICOLON but got RBRACE at 10:5
Error: Undefined variable 'x' at 15:12
Error: Type mismatch in assignment at 20:8
```

## Testing

The compiler includes comprehensive test suites:
- `test_lexer.py`: Lexical analysis tests
- `test_arrays.py`: Array functionality tests
- `test_pointers.py`: Pointer operation tests
- `test_strings.py`: String literal tests
- `test_char.py`: Character type tests
- `test_function_pointers.py`: Function pointer tests

Each test verifies both parsing and code generation correctness.