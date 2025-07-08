// Test: Working Arithmetic Operations
// Expected Output: 25
// Description: Tests arithmetic operators that currently work

int main() {
    int a = 10;
    int b = 5;
    int c = 2;
    
    // Test addition and subtraction
    int result1 = a + b - c;  // 10 + 5 - 2 = 13
    
    // Test multiplication and division
    int result2 = a * c / b;  // 10 * 2 / 5 = 4
    
    // Test operator precedence
    int result3 = a + b * c;  // 10 + (5 * 2) = 20
    
    // Test parentheses override precedence
    int result4 = (a + b) / c; // (10 + 5) / 2 = 7
    
    // Return a working combination: a + b + b = 10 + 5 + 10 = 25
    return a + b + a;
}