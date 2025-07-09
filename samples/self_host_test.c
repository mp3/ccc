// Minimal self-hosting test
// This tests basic features needed for self-hosting

#include <stdio.h>
#include <stdlib.h>

// Test struct with function pointer
typedef struct {
    int x;
    int (*func)(int);
} TestStruct;

// Test function declaration
int add(int a, int b);

// Test global variable
int global_var = 42;

// Test static function
static int multiply(int a, int b) {
    return a * b;
}

// Test function implementation
int add(int a, int b) {
    return a + b;
}

// Test variadic function
int my_printf(const char *fmt, ...) {
    // Just return 0 for now
    return 0;
}

int main() {
    // Test basic operations
    int result = add(3, 4);
    if (result != 7) return 1;
    
    // Test struct
    TestStruct ts;
    ts.x = 10;
    ts.func = multiply;
    
    // Test function pointer
    if (ts.func(5, 6) != 30) return 2;
    
    // Test global access
    if (global_var != 42) return 3;
    
    // Test pointer arithmetic
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = arr;
    p += 2;
    if (*p != 3) return 4;
    
    // Test type casting
    char c = 'A';
    int i = (int)c;
    if (i != 65) return 5;
    
    // Test bitwise operations
    int x = 5 & 3;
    if (x != 1) return 6;
    
    // Test compound assignment
    int y = 10;
    y += 5;
    if (y != 15) return 7;
    
    // Test ternary operator
    int z = (result > 5) ? 100 : 200;
    if (z != 100) return 8;
    
    printf("All tests passed!\n");
    return 0;
}