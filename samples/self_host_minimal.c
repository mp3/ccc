// Minimal self-hosting test without standard library

// Test struct - anonymous struct in typedef
typedef struct {
    int x;
    int y;
} Point;

// Test named struct
struct Node {
    int value;
    struct Node *next;
};

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

// Test extern declaration
extern int putchar(int c);

int main() {
    // Test basic operations
    int result = add(3, 4);
    if (result != 7) return 1;
    
    // Test anonymous struct
    Point p;
    p.x = 10;
    p.y = 20;
    if (p.x != 10) return 2;
    
    // Test named struct
    struct Node n;
    n.value = 100;
    n.next = 0;
    if (n.value != 100) return 3;
    
    // Test global access
    if (global_var != 42) return 4;
    
    // Test pointer arithmetic
    int arr[5];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    int *ptr = &arr[0];
    ptr = ptr + 2;
    if (*ptr != 3) return 5;
    
    // Test type casting
    char c = 65;
    int i = (int)c;
    if (i != 65) return 6;
    
    // Test bitwise operations
    int x = 5 & 3;
    if (x != 1) return 7;
    
    // Test compound assignment
    int y = 10;
    y += 5;
    if (y != 15) return 8;
    
    // Test ternary operator
    int z = (result > 5) ? 100 : 200;
    if (z != 100) return 9;
    
    // Success
    putchar('O');
    putchar('K');
    putchar('\n');
    
    return 0;
}