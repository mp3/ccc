int main() {
    int a = 5;
    int b = 10;
    int c;
    
    // Basic comma operator
    c = (a = 20, b = 30, a + b);
    if (c != 50) {
        return 1;
    }
    
    // Comma operator in for loop
    int i;
    int j;
    for (i = 0, j = 10; i < 5; i = i + 1, j = j - 1) {
        // i goes 0, 1, 2, 3, 4
        // j goes 10, 9, 8, 7, 6
    }
    if (i != 5) {
        return 2;
    }
    if (j != 5) {
        return 3;
    }
    
    // Comma operator with side effects
    int x = 0;
    int y = (x = x + 1, x = x + 1, x = x + 1, x);
    if (y != 3) {
        return 4;
    }
    
    // Comma operator in function call
    putchar((a = 72, a));   // 'H'
    putchar((b = 105, b));  // 'i'
    putchar(10);            // '\n'
    
    return 0;
}