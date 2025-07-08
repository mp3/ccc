// Demonstration of compiler warnings

// Function with unused variable
int example1() {
    int used = 5;
    int unused = 10;  // Warning: unused variable
    
    return used;
}

// Function with uninitialized variable use
int example2() {
    int x;
    int y = 5;
    
    // Using x before initialization would generate a warning
    // but our current test file has x assigned before use
    x = 10;
    
    return x + y;
}

// Main function with various warnings
int main() {
    int a = 5;
    int b;         // Declared but not initialized
    int c = 10;    // Warning: unused variable
    
    b = a * 2;     // b is now initialized
    
    int result = example1() + b;
    
    return result;
}