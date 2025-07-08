/*
 * Test: Variadic Functions
 * Expected Output: 3
 * Description: Tests functions with variable number of arguments
 */

// Simple variadic function that just returns the count
int simple_variadic(int count, ...) {
    // Note: Without stdarg.h, we can't actually access the variadic arguments
    // This test just verifies that variadic function syntax works
    return count;
}

// Another variadic function
int min_args_required(int first, int second, ...) {
    // Return sum of required arguments
    return first + second;
}

// Variadic function with different signature
char get_first_char(char base, ...) {
    return base;
}

int main() {
    // Test variadic function calls with different argument counts
    int result1 = simple_variadic(1, 100);                    // Should return 1
    int result2 = simple_variadic(2, 200, 300);               // Should return 2  
    int result3 = simple_variadic(3, 400, 500, 600);          // Should return 3
    int result4 = simple_variadic(0);                         // Should return 0
    
    // Test with required parameters
    int result5 = min_args_required(10, 20);                  // Should return 30
    int result6 = min_args_required(5, 7, 100, 200);          // Should return 12
    
    // Test with character variadic
    char result7 = get_first_char('A', 'B', 'C');             // Should return 'A'
    char result8 = get_first_char('Z');                       // Should return 'Z'
    
    // Test complex variadic call
    int complex_result = simple_variadic(4, result5, result6, (int)result7, (int)result8);
    // Should return 4
    
    // We want to return 3
    return result3;
}