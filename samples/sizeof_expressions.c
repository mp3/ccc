int main() {
    // Test sizeof with various expressions
    
    // Literals
    int size1 = sizeof(42);          // int literal - should be 4
    int size2 = sizeof('A');         // char literal - should be 1
    
    if (size1 != 4) return 1;
    if (size2 != 1) return 2;
    
    // Complex expressions
    int x = 10;
    int y = 20;
    
    // Binary expressions are evaluated at compile time
    int size3 = sizeof(x + y);       // Result is int - should be 4
    int size4 = sizeof(x * y);       // Result is int - should be 4
    
    if (size3 != 4) return 3;
    if (size4 != 4) return 4;
    
    // Sizeof doesn't evaluate the expression, just determines its type
    int counter = 0;
    int size5 = sizeof(counter = counter + 1);  // counter should not be incremented
    
    if (counter != 0) return 5;  // Verify counter wasn't modified
    if (size5 != 4) return 6;    // But sizeof still returns 4
    
    return 0;  // All tests passed
}