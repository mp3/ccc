/*
 * Test: Control Flow Statements
 * Expected Output: 15
 * Description: Tests if/else, while, for, do-while, and switch statements
 */

int main() {
    int result = 0;
    
    // Test if-else
    int x = 5;
    if (x > 3) {
        result += 1;  // Should execute: result = 1
    } else {
        result += 2;
    }
    
    // Test while loop
    int i = 0;
    while (i < 3) {
        result += 1;  // Executes 3 times: result = 4
        i++;
    }
    
    // Test for loop
    for (int j = 0; j < 2; j++) {
        result += 2;  // Executes 2 times: result = 8
    }
    
    // Test do-while loop
    int k = 0;
    do {
        result += 1;  // Executes 2 times: result = 10
        k++;
    } while (k < 2);
    
    // Test switch statement
    int choice = 2;
    switch (choice) {
        case 1:
            result += 10;
            break;
        case 2:
            result += 3;  // Should execute: result = 13
            break;
        case 3:
            result += 20;
            break;
        default:
            result += 100;
    }
    
    // Test nested if
    if (result > 10) {
        if (result < 20) {
            result += 2;  // Should execute: result = 15
        }
    }
    
    return result;  // Expected: 15
}