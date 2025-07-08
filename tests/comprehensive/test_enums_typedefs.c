/*
 * Test: Enums and Typedefs
 * Expected Output: 8
 * Description: Tests enumeration types and type definitions
 */

// Test enum declaration
enum Color {
    RED,     // 0
    GREEN,   // 1
    BLUE,    // 2
    YELLOW   // 3
};

// Test enum with explicit values
enum Status {
    PENDING = 10,
    PROCESSING = 20,
    COMPLETE = 30,
    ERROR = 40
};

// Test typedef with basic type
typedef int Integer;
typedef char Character;

// Test typedef with pointer
typedef int* IntPtr;

// Test typedef with struct
typedef struct {
    int x;
    int y;
} Point;

// Test typedef with enum
typedef enum Color ColorType;

// Test typedef with function pointer
typedef int (*BinaryOp)(int, int);

int add_func(int a, int b) {
    return a + b;
}

int main() {
    // Test enum usage
    enum Color primary = RED;         // 0
    enum Color secondary = BLUE;      // 2
    enum Status current = PROCESSING; // 20
    
    // Test typedef usage
    Integer num = 42;
    Character ch = 'A';  // ASCII 65
    
    // Test typedef with pointer
    Integer value = 15;
    IntPtr ptr = &value;
    Integer deref_val = *ptr;  // 15
    
    // Test typedef with struct
    Point p;
    p.x = 3;
    p.y = 4;
    
    // Test typedef with enum
    ColorType color = GREEN;  // 1
    
    // Test typedef with function pointer
    BinaryOp operation = add_func;
    Integer result = operation(5, 3);  // 8
    
    // Test enum arithmetic
    int color_sum = RED + GREEN + BLUE;  // 0 + 1 + 2 = 3
    
    // Test enum in expressions
    int status_diff = current - PENDING;  // 20 - 10 = 10
    
    // Calculate final result
    // We want to return 8
    // result from function pointer call is 8
    return result;
}