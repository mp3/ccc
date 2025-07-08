// Test file to demonstrate compiler warnings

int calculate(int x, int y) {
    int unused = 42;  // Should warn: unused variable
    int result;
    
    result = x + y;  // result was used uninitialized before this
    
    return result;
}

int missing_return(int x) {
    if (x > 0) {
        return x * 2;
    }
    // Should warn: missing return statement
}

int main() {
    int a = 5;
    int b = 10;
    int c;  // Should warn: unused variable
    int d;  // Should warn: uninitialized and unused
    
    int sum = calculate(a, b);
    
    // Unreachable code after return
    return sum;
    
    int unreachable = 123;  // Should warn: unreachable code
}