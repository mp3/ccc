int main() {
    // Test logical AND
    int a = 1;
    int b = 0;
    int c = 1;
    
    // Basic AND tests
    if (a && c) {
        // Both true, should execute
    } else {
        return 1;
    }
    
    if (a && b) {
        // One false, should not execute
        return 2;
    }
    
    // Test logical OR
    if (a || b) {
        // One true, should execute
    } else {
        return 3;
    }
    
    if (b || b) {
        // Both false, should not execute
        return 4;
    }
    
    // Test logical NOT
    if (!b) {
        // NOT false = true, should execute
    } else {
        return 5;
    }
    
    if (!a) {
        // NOT true = false, should not execute
        return 6;
    }
    
    // Complex expression with all operators
    if ((a && c) || (!b && a)) {
        // Should be true
    } else {
        return 7;
    }
    
    return 0;  // All tests passed
}