// Test: Simple Arithmetic Operations
// Expected Output: 42
// Description: Tests basic arithmetic operators

int main() {
    int a = 10;
    int b = 5;
    int c = 2;
    
    // Test addition and subtraction
    int result1 = a + b - c;  // 10 + 5 - 2 = 13
    
    // Test multiplication and division
    int result2 = a * c / b;  // 10 * 2 / 5 = 4
    
    // Test modulo
    int result3 = a % b;      // 10 % 5 = 0
    
    // Test operator precedence
    int result4 = a + b * c;  // 10 + (5 * 2) = 20
    
    // Test parentheses override precedence
    int result5 = (a + b) / c; // (10 + 5) / 2 = 7
    
    // Combine all results: 13 + 4 + 0 + 20 + 7 - 2 = 42
    return result1 + result2 + result3 + result4 + result5 - c;
}