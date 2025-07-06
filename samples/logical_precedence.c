int main() {
    // Test operator precedence
    // NOT has highest precedence, then AND, then OR
    
    int a = 1;
    int b = 0;
    int c = 1;
    
    // Test NOT precedence over AND
    // !b && a should be (!b) && a = 1 && 1 = 1
    if (!b && a) {
        // Should execute
    } else {
        return 1;
    }
    
    // Test AND precedence over OR
    // a || b && c should be a || (b && c) = 1 || 0 = 1
    if (a || b && c) {
        // Should execute
    } else {
        return 2;
    }
    
    // More complex test
    // !b || a && c should be (!b) || (a && c) = 1 || 1 = 1
    if (!b || a && c) {
        // Should execute
    } else {
        return 3;
    }
    
    // Test that parentheses override precedence
    // (a || b) && c should be 1 && 1 = 1
    if ((a || b) && c) {
        // Should execute
    } else {
        return 4;
    }
    
    // Without parentheses: a || (b && c) = 1 || 0 = 1
    // With parentheses: (a || b) && (!c) = 1 && 0 = 0
    if ((a || b) && (!c)) {
        // Should NOT execute
        return 5;
    }
    
    return 0;  // All tests passed
}