// Test file to demonstrate various optimizations

int test_constant_propagation() {
    int x = 10;      // x is a constant 10
    int y = x + 5;   // Should be optimized to y = 15
    int z = x * 2;   // Should be optimized to z = 20
    return y + z;    // Should be optimized to return 35
}

int test_algebraic_simplification() {
    int a = 5;
    int b = a + 0;    // Should be optimized to b = a
    int c = a - 0;    // Should be optimized to c = a
    int d = a * 1;    // Should be optimized to d = a
    int e = a * 0;    // Should be optimized to e = 0
    int f = 0 * a;    // Should be optimized to f = 0
    int g = a / 1;    // Should be optimized to g = a
    return b + c + d + e + f + g;
}

int test_strength_reduction() {
    int x = 5;
    int a = x * 2;    // Power of 2 multiplication
    int b = x * 4;    // Power of 2 multiplication
    int c = x * 8;    // Power of 2 multiplication
    int d = x / 2;    // Power of 2 division
    int e = x / 4;    // Power of 2 division
    return a + b + c + d + e;
}

int test_constant_folding() {
    int a = 2 + 3;         // Should fold to 5
    int b = 10 - 4;        // Should fold to 6
    int c = 3 * 4;         // Should fold to 12
    int d = 20 / 5;        // Should fold to 4
    int e = (2 + 3) * 4;   // Should fold to 20
    return a + b + c + d + e;
}

int test_dead_code() {
    if (1) {
        return 42;     // Always executed
    } else {
        return 0;      // Dead code - should be eliminated
    }
}

int main() {
    int result = 0;
    result = test_constant_propagation();
    result = test_algebraic_simplification();
    result = test_strength_reduction();
    result = test_constant_folding();
    result = test_dead_code();
    return result;
}