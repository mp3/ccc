# CCC Compiler User Guide

This guide helps you get started with the CCC compiler and make the most of its features.

## Quick Start

### Installation

1. Clone the repository:
```bash
git clone <repository-url>
cd ccc
```

2. Build the compiler:
```bash
make
```

3. Verify installation:
```bash
./ccc --version
```

### Your First Program

Create a file `hello.c`:
```c
int main() {
    putchar('H');
    putchar('e');
    putchar('l');
    putchar('l');
    putchar('o');
    putchar('\n');
    return 0;
}
```

Compile and run:
```bash
./ccc hello.c -o hello.ll
llc hello.ll -filetype=obj -o hello.o
clang hello.o -o hello
./hello
```

## Command Line Usage

### Basic Syntax
```bash
./ccc [options] input.c [-o output.ll]
```

### Options

| Option | Description | Default |
|--------|-------------|---------|
| `-o <file>` | Output file name | stdout |
| `-O0` | No optimization | |
| `-O1` | Basic optimizations | ✓ |
| `-O2` | More optimizations | |
| `-v` | Verbose output | |

### Examples

```bash
# Compile with default settings
./ccc program.c -o program.ll

# Compile with no optimization
./ccc -O0 program.c -o program.ll

# Compile with verbose output
./ccc -v program.c -o program.ll

# Output to stdout (pipe to other tools)
./ccc program.c | llc -filetype=obj -o program.o
```

## Language Features Guide

### Variables and Types

#### Basic Types
```c
int x = 42;         // 32-bit integer
char c = 'A';       // 8-bit character
char *s = "Hello";  // String (char pointer)
```

#### Arrays
```c
int numbers[10];           // Array of 10 integers
char buffer[256];          // Character buffer
int matrix[3][3];          // 2D array

// Initialization
int values[] = {1, 2, 3};  // Size inferred
```

#### Pointers
```c
int x = 42;
int *p = &x;        // Pointer to x
int **pp = &p;      // Pointer to pointer

*p = 100;           // Dereference: x is now 100
int y = *p;         // y = 100

// Pointer arithmetic
int arr[5];
int *ptr = arr;
ptr++;              // Points to arr[1]
```

#### Structures
```c
struct Point {
    int x;
    int y;
};

struct Point p1;
p1.x = 10;
p1.y = 20;

struct Point *pp = &p1;
pp->x = 30;         // Same as (*pp).x = 30
```

#### Unions
```c
union Value {
    int i;
    char c;
};

union Value v;
v.i = 65;
char ch = v.c;      // ch = 'A' (65 in ASCII)
```

#### Enumerations
```c
enum Status {
    SUCCESS,        // 0
    ERROR,          // 1
    PENDING = 10,   // 10
    COMPLETE        // 11
};

enum Status s = SUCCESS;
```

#### Type Definitions
```c
typedef int Integer;
typedef struct Point Point;
typedef int (*BinaryOp)(int, int);

Integer x = 42;
Point p;
BinaryOp op = &add;
```

### Functions

#### Basic Functions
```c
int add(int a, int b) {
    return a + b;
}

void print_char(char c) {
    putchar(c);
}

int main() {
    int sum = add(5, 3);
    print_char('A');
    return 0;
}
```

#### Function Pointers
```c
int multiply(int a, int b) {
    return a * b;
}

int main() {
    int (*op)(int, int);
    op = &add;        // or just: op = add;
    int result = op(10, 5);
    
    op = multiply;
    result = op(10, 5);
    
    return 0;
}
```

#### Variadic Functions
```c
int sum(int count, ...) {
    // Note: Manual argument handling needed
    // No va_list support yet
    return count;
}

int main() {
    int total = sum(3, 10, 20, 30);
    return 0;
}
```

#### Static Variables
```c
void counter() {
    static int count = 0;  // Initialized once
    count++;
    putchar('0' + count);
}

int main() {
    counter();  // Prints '1'
    counter();  // Prints '2'
    counter();  // Prints '3'
    return 0;
}
```

### Control Flow

#### Conditionals
```c
if (x > 0) {
    // positive
} else if (x < 0) {
    // negative
} else {
    // zero
}

// Ternary operator
int max = (a > b) ? a : b;
```

#### Loops
```c
// While loop
int i = 0;
while (i < 10) {
    putchar('0' + i);
    i++;
}

// Do-while loop
do {
    putchar('A');
} while (0);  // Executes once

// For loop
for (int j = 0; j < 10; j++) {
    if (j == 5) continue;  // Skip 5
    if (j == 8) break;     // Stop at 8
    putchar('0' + j);
}
```

#### Switch Statements
```c
switch (ch) {
    case 'a':
    case 'A':
        putchar('1');
        break;
    
    case 'b':
    case 'B':
        putchar('2');
        // Fall through
    
    default:
        putchar('?');
}
```

### Operators

#### Arithmetic
```c
int a = 10 + 5;    // Addition
int b = 10 - 5;    // Subtraction
int c = 10 * 5;    // Multiplication
int d = 10 / 5;    // Division
int e = 10 % 3;    // Modulo (remainder)
```

#### Compound Assignment
```c
int x = 10;
x += 5;    // x = x + 5
x -= 3;    // x = x - 3
x *= 2;    // x = x * 2
x /= 4;    // x = x / 4
```

#### Increment/Decrement
```c
int i = 5;
int a = i++;   // a = 5, i = 6 (post-increment)
int b = ++i;   // b = 7, i = 7 (pre-increment)
int c = i--;   // c = 7, i = 6 (post-decrement)
int d = --i;   // d = 5, i = 5 (pre-decrement)
```

#### Bitwise Operations
```c
int a = 5 & 3;     // AND: 0101 & 0011 = 0001 (1)
int b = 5 | 3;     // OR:  0101 | 0011 = 0111 (7)
int c = 5 ^ 3;     // XOR: 0101 ^ 0011 = 0110 (6)
int d = ~5;        // NOT: ~0101 = ...11111010 (-6)
int e = 5 << 1;    // Left shift: 0101 << 1 = 1010 (10)
int f = 5 >> 1;    // Right shift: 0101 >> 1 = 0010 (2)
```

#### Logical Operations
```c
int a = (5 > 3) && (2 < 4);   // AND: 1 (true)
int b = (5 < 3) || (2 < 4);   // OR: 1 (true)
int c = !(5 > 3);             // NOT: 0 (false)

// Short-circuit evaluation
int x = 0;
if (x != 0 && 10/x > 1) {    // Second part not evaluated
    // Never executed
}
```

### Memory and Pointers

#### Address Operations
```c
int x = 42;
int *p = &x;       // Get address of x
int y = *p;        // Dereference p (y = 42)

// Array-pointer relationship
int arr[5] = {1, 2, 3, 4, 5};
int *ptr = arr;    // arr decays to pointer
int first = *ptr;  // first = 1
int third = *(ptr + 2);  // third = 3
```

#### Sizeof Operator
```c
int size1 = sizeof(int);        // 4
int size2 = sizeof(char);       // 1
int arr[10];
int size3 = sizeof(arr);        // 40 (10 * 4)
int size4 = sizeof(arr[0]);     // 4

// Array length calculation
int length = sizeof(arr) / sizeof(arr[0]);  // 10
```

#### Type Casting
```c
int i = 65;
char c = (char)i;              // c = 'A'
int *p = (int *)0;             // Null pointer
char *s = (char *)&i;          // Treat int as char array
```

### Constants and Qualifiers

#### Const Qualifier
```c
const int MAX = 100;           // Cannot modify MAX
const char *msg = "Hello";     // Cannot modify string
int * const p = &x;            // Cannot change pointer
const int * const q = &y;      // Cannot change pointer or value
```

## Common Patterns

### String Manipulation
```c
// Print a string
void print_string(char *s) {
    while (*s) {
        putchar(*s);
        s++;
    }
}

// String length
int strlen(char *s) {
    int len = 0;
    while (*s++) {
        len++;
    }
    return len;
}
```

### Array Processing
```c
// Sum array elements
int sum_array(int *arr, int size) {
    int sum = 0;
    for (int i = 0; i < size; i++) {
        sum += arr[i];
    }
    return sum;
}

// Find maximum
int find_max(int *arr, int size) {
    int max = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}
```

### Recursive Functions
```c
// Factorial
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// Fibonacci
int fibonacci(int n) {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}
```

### Input/Output
```c
// Read integer from input
int read_int() {
    int num = 0;
    int ch;
    
    // Skip whitespace
    do {
        ch = getchar();
    } while (ch == ' ' || ch == '\n' || ch == '\t');
    
    // Read digits
    while (ch >= '0' && ch <= '9') {
        num = num * 10 + (ch - '0');
        ch = getchar();
    }
    
    return num;
}

// Print integer
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

## Optimization Tips

### Enable Optimizations
```bash
# Basic optimizations (default)
./ccc -O1 program.c -o program.ll

# More aggressive optimizations
./ccc -O2 program.c -o program.ll
```

### Write Optimization-Friendly Code

1. **Use constants where possible**
```c
// Good: Compiler can fold this
int x = 2 * 10 + 5;

// Less optimal: Runtime calculation
int a = get_value();
int x = 2 * a + 5;
```

2. **Avoid unnecessary variables**
```c
// Can be optimized away
if (1) {
    do_something();
}

// Runtime check
if (condition) {
    do_something();
}
```

3. **Use appropriate types**
```c
// Use char for small values
char counter = 0;
while (counter < 10) {
    counter++;
}
```

## Debugging Tips

### Use Verbose Mode
```bash
./ccc -v program.c -o program.ll
```

### Check Generated LLVM IR
```bash
cat program.ll
```

### Common Error Messages

| Error | Cause | Solution |
|-------|-------|----------|
| `Undefined variable 'x'` | Variable used before declaration | Declare variable before use |
| `Expected SEMICOLON` | Missing semicolon | Add semicolon at end of statement |
| `Type mismatch` | Incompatible types in operation | Use correct types or cast |
| `Function expects N arguments` | Wrong number of arguments | Check function signature |
| `Invalid escape sequence` | Unknown escape in string/char | Use valid escapes: \n, \t, \\, etc. |

### Testing Your Code

1. **Start simple**: Test individual functions first
2. **Use putchar for debugging**: Print values to trace execution
3. **Check edge cases**: Test with 0, negative numbers, empty arrays
4. **Verify loops**: Ensure proper termination conditions

## Limitations to Remember

1. **No standard library**: Only `putchar` and `getchar` available
2. **Integer arithmetic only**: No floating-point support
3. **Single file programs**: Cannot link multiple source files
4. **Manual memory management**: No malloc/free
5. **Fixed-size arrays**: No variable-length arrays
6. **No preprocessor**: No #include, #define, etc.

## Example Programs

### Program 1: Prime Number Checker
```c
int is_prime(int n) {
    if (n < 2) return 0;
    
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0;
    }
    
    return 1;
}

int main() {
    for (int i = 2; i <= 20; i++) {
        if (is_prime(i)) {
            print_int(i);
            putchar(' ');
        }
    }
    putchar('\n');
    return 0;
}
```

### Program 2: Bubble Sort
```c
void bubble_sort(int *arr, int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (arr[j] > arr[j + 1]) {
                // Swap
                int temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;
            }
        }
    }
}

int main() {
    int numbers[5] = {5, 2, 8, 1, 9};
    
    bubble_sort(numbers, 5);
    
    for (int i = 0; i < 5; i++) {
        print_int(numbers[i]);
        putchar(' ');
    }
    putchar('\n');
    
    return 0;
}
```

### Program 3: Simple Calculator
```c
int calculate(int a, int b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return (b != 0) ? a / b : 0;
        default: return 0;
    }
}

int main() {
    int result = calculate(10, 5, '+');
    print_int(result);
    putchar('\n');
    
    return 0;
}
```

## Next Steps

1. **Explore the test suite**: Learn from example programs in `tests/`
2. **Read the architecture guide**: Understand compiler internals
3. **Contribute**: Add new features or fix bugs
4. **Build larger programs**: Test the compiler's limits