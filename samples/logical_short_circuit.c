int main() {
    int x = 0;
    int y = 0;
    
    // Test short-circuit evaluation for AND
    // Second operand should not be evaluated if first is false
    if (0 && (x = 1)) {
        return 1;
    }
    
    // x should still be 0 due to short-circuit
    if (x != 0) return 2;
    
    // Test short-circuit evaluation for OR
    // Second operand should not be evaluated if first is true
    if (1 || (y = 1)) {
        // Should enter here
    } else {
        return 3;
    }
    
    // y should still be 0 due to short-circuit
    if (y != 0) return 4;
    
    // Test that second operand IS evaluated when needed
    if (1 && (x = 5)) {
        // Should enter and x should be set to 5
    } else {
        return 5;
    }
    
    if (x != 5) return 6;
    
    // Test OR evaluation when first is false
    if (0 || (y = 7)) {
        // Should enter and y should be set to 7
    } else {
        return 7;
    }
    
    if (y != 7) return 8;
    
    return 0;  // All tests passed
}