/*
 * Test: Function Pointers
 * Expected Output: 18
 * Description: Tests function pointer declarations, assignments, and calls
 */

// Test functions to point to
int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int multiply(int a, int b) {
    return a * b;
}

// Function that takes a function pointer as parameter
int apply_operation(int x, int y, int (*op)(int, int)) {
    return op(x, y);
}

// Function that returns a function pointer
int (*get_operation(int choice))(int, int) {
    if (choice == 1) {
        return add;
    } else if (choice == 2) {
        return subtract;
    } else {
        return multiply;
    }
}

int main() {
    // Test basic function pointer declaration and assignment
    int (*operation)(int, int);
    
    // Test assignment and call
    operation = add;
    int result1 = operation(10, 5);  // 15
    
    // Test reassignment
    operation = multiply;
    int result2 = operation(3, 2);   // 6
    
    // Test direct assignment from function name
    operation = subtract;
    int result3 = operation(20, 7);  // 13
    
    // Test function pointer as parameter
    int result4 = apply_operation(4, 3, add);       // 7
    int result5 = apply_operation(8, 2, subtract);  // 6
    int result6 = apply_operation(2, 4, multiply);  // 8
    
    // Test function returning function pointer
    int (*func_ptr)(int, int) = get_operation(1);  // Should return add
    int result7 = func_ptr(5, 3);  // 8
    
    func_ptr = get_operation(3);   // Should return multiply
    int result8 = func_ptr(2, 3);  // 6
    
    // Test array of function pointers
    int (*ops[3])(int, int);
    ops[0] = add;
    ops[1] = subtract;
    ops[2] = multiply;
    
    int result9 = ops[0](4, 2);    // 6
    int result10 = ops[2](3, 1);   // 3
    
    // Calculate final result
    // We want 18, so let's use result1 + result8 = 15 + 6 = 21, but we want 18
    // Let's use result1 + result8 - result10 = 15 + 6 - 3 = 18
    return result1 + result8 - result10;
}