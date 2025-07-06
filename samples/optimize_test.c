int main() {
    // Constant folding examples
    int a = 5 + 3 * 2;        // Should fold to 11
    int b = 100 / 5 - 10;     // Should fold to 10
    int c = (2 + 3) * (4 + 1); // Should fold to 25
    
    // Comparison folding
    int d = 5 > 3;            // Should fold to 1
    int e = 10 == 10;         // Should fold to 1
    int f = 7 < 2;            // Should fold to 0
    
    // Dead code elimination
    if (1) {
        a = 100;              // This should remain
    } else {
        a = 200;              // This should be eliminated
    }
    
    if (0) {
        b = 999;              // This should be eliminated
    }
    
    // While loop with constant false condition
    while (0) {
        c = 42;               // This should be eliminated
    }
    
    return a + b + c;         // 100 + 10 + 25 = 135
}