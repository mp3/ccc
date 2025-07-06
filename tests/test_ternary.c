int main() {
    // Basic ternary operator
    int a = 5;
    int b = a > 3 ? 10 : 20;
    if (b != 10) return 1;
    
    // Ternary with false condition
    int c = a < 3 ? 100 : 200;
    if (c != 200) return 2;
    
    // Nested ternary (right associative)
    int d = a > 3 ? a < 10 ? 1 : 2 : 3;
    if (d != 1) return 3;
    
    // Ternary in expressions
    int e = 2 + (a > 0 ? 3 : 4);
    if (e != 5) return 4;
    
    // Ternary with complex expressions
    int f = 10;
    int g = (a > 3 && f < 20) ? a + f : a - f;
    if (g != 15) return 5;
    
    // Ternary with function calls
    int h = putchar('A') > 0 ? 1 : 0;
    putchar('\n');
    if (h != 1) return 6;
    
    // Multiple ternaries
    int x = 5;
    int y = 10;
    int z = 15;
    int max = x > y ? (x > z ? x : z) : (y > z ? y : z);
    if (max != 15) return 7;
    
    // Ternary with assignment
    int result = 0;
    result = a > 0 ? result + 10 : result - 10;
    if (result != 10) return 8;
    
    // Ternary with different types promotion
    int i = 1;
    int j = i ? 42 : 0;
    if (j != 42) return 9;
    
    return 0;
}