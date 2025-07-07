# CCC Compiler Quick Reference

## Command Line

```bash
./ccc [options] input.c [-o output.ll]

Options:
  -o <file>    Output file (default: stdout)
  -O0          No optimization
  -O1          Basic optimizations (default)
  -O2          More optimizations
  -v           Verbose output
```

## Types

| Type | Size | Range | Example |
|------|------|-------|---------|
| `int` | 4 bytes | -2³¹ to 2³¹-1 | `int x = 42;` |
| `char` | 1 byte | -128 to 127 | `char c = 'A';` |
| `type*` | 8 bytes | Pointer | `int *p = &x;` |

## Operators (Precedence High to Low)

| Level | Operators | Associativity |
|-------|-----------|---------------|
| 1 | `()` `[]` `.` `->` `++` `--` (postfix) | Left to right |
| 2 | `++` `--` (prefix) `+` `-` (unary) `!` `~` `*` `&` `sizeof` `(type)` | Right to left |
| 3 | `*` `/` `%` | Left to right |
| 4 | `+` `-` | Left to right |
| 5 | `<<` `>>` | Left to right |
| 6 | `<` `>` `<=` `>=` | Left to right |
| 7 | `==` `!=` | Left to right |
| 8 | `&` | Left to right |
| 9 | `^` | Left to right |
| 10 | `|` | Left to right |
| 11 | `&&` | Left to right |
| 12 | `||` | Left to right |
| 13 | `?:` | Right to left |
| 14 | `=` `+=` `-=` `*=` `/=` | Right to left |
| 15 | `,` | Left to right |

## Keywords

```c
// Types
int char void struct union enum typedef const

// Control Flow
if else while do for switch case default break continue return

// Storage
static

// Operators
sizeof
```

## Control Structures

### Conditionals
```c
if (condition) {
    // statements
} else if (condition) {
    // statements
} else {
    // statements
}

// Ternary
result = condition ? true_value : false_value;
```

### Loops
```c
// While
while (condition) {
    // statements
}

// Do-While
do {
    // statements
} while (condition);

// For
for (init; condition; update) {
    // statements
}
```

### Switch
```c
switch (expression) {
    case constant1:
        // statements
        break;
    case constant2:
    case constant3:
        // statements
        break;
    default:
        // statements
}
```

## Functions

```c
// Declaration
return_type function_name(param_type param);

// Definition
return_type function_name(param_type param) {
    // statements
    return value;
}

// Function Pointer
return_type (*ptr_name)(param_types);
ptr_name = &function_name;
result = ptr_name(args);

// Variadic
int func(int required, ...) {
    // manual arg handling
}
```

## Arrays and Pointers

```c
// Arrays
int arr[10];                    // Array of 10 ints
int matrix[3][4];              // 2D array
char str[] = "Hello";          // String array

// Pointers
int *p = &variable;            // Address of
int value = *p;                // Dereference
p++;                           // Pointer arithmetic
arr[i] == *(arr + i)          // Array-pointer equivalence
```

## Structures and Unions

```c
// Structure
struct Point {
    int x;
    int y;
};
struct Point p = {10, 20};
p.x = 30;

// Union
union Data {
    int i;
    char c;
};
union Data d;
d.i = 65;

// Typedef
typedef struct Point Point;
Point p2;
```

## Enumerations

```c
enum Color {
    RED,        // 0
    GREEN,      // 1
    BLUE = 10,  // 10
    YELLOW      // 11
};
enum Color c = RED;
```

## Type Casting

```c
int i = 65;
char c = (char)i;              // Explicit cast
void *p = (void *)&i;          // Pointer cast
int x = c;                     // Implicit cast
```

## I/O Functions

```c
int c = getchar();             // Read character (EOF on end)
putchar('A');                  // Write character
putchar(c);                    // Echo character
```

## Common Patterns

### String Output
```c
char *s = "Hello";
while (*s) {
    putchar(*s++);
}
```

### Integer Output
```c
void print_int(int n) {
    if (n < 0) {
        putchar('-');
        n = -n;
    }
    if (n >= 10) {
        print_int(n / 10);
    }
    putchar('0' + (n % 10));
}
```

### Array Length
```c
int arr[10];
int len = sizeof(arr) / sizeof(arr[0]);  // 10
```

### Swap Values
```c
int temp = a;
a = b;
b = temp;
```

## Escape Sequences

| Sequence | Meaning |
|----------|---------|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\'` | Single quote |
| `\"` | Double quote |

## Compilation Pipeline

```bash
# Compile to LLVM IR
./ccc program.c -o program.ll

# Assemble to object file
llc program.ll -filetype=obj -o program.o

# Link to executable
clang program.o -o program

# Run
./program
```

## Quick Debugging

```bash
# Verbose compilation
./ccc -v program.c -o program.ll

# Check generated IR
cat program.ll | less

# View optimization effects
./ccc -O0 program.c -o unopt.ll
./ccc -O2 program.c -o opt.ll
diff unopt.ll opt.ll
```

## Limitations

- No floating-point (`float`, `double`)
- No other integer types (`short`, `long`, `unsigned`)
- No preprocessor (`#include`, `#define`)
- No standard library (except `putchar`/`getchar`)
- Single file compilation only
- No `malloc`/`free`
- No variable-length arrays

## Error Messages

| Error | Example | Fix |
|-------|---------|-----|
| `Undefined variable` | `x = 5;` | Declare: `int x;` |
| `Expected SEMICOLON` | `int x = 5` | Add: `;` |
| `Type mismatch` | `int *p = 5;` | Use: `&variable` or cast |
| `Function expects N arguments` | `add(5);` | Provide all arguments |
| `Unexpected token` | `int 123;` | Valid identifier needed |