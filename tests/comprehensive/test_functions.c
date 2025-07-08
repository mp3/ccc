/*
 * Test: Function Definitions and Calls
 * Expected Output: 21
 * Description: Tests function definitions, parameters, return values, and recursion
 */

// Test basic function with parameters
int add(int a, int b) {
    return a + b;
}

// Test function with multiple parameters
int multiply(int x, int y, int z) {
    return x * y * z;
}

// Test recursive function
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// Test function with no parameters
int get_constant() {
    return 7;
}

// Test function that calls other functions
int complex_calculation(int base) {
    int step1 = add(base, 3);        // 2 + 3 = 5
    int step2 = multiply(step1, 2, 1); // 5 * 2 * 1 = 10
    int step3 = factorial(3);         // 3! = 6
    int step4 = get_constant();       // 7
    
    return step2 + step3 - step4 + step1;  // 10 + 6 - 7 + 5 = 14
}

int main() {
    int base = 2;
    int result = complex_calculation(base);  // Should be 14
    
    // Add some more to get to 21
    result = result + get_constant();  // 14 + 7 = 21
    
    return result;
}