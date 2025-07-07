int main() {
    int a;
    int b;
    int c;
    
    // Basic comma operator
    a = 5;
    b = 10;
    c = (a, b);  // Should return b (10)
    
    if (c != 10) {
        return 1;
    }
    
    // Comma with assignment
    c = (a = 20, b = 30, a + b);
    if (c != 50) {
        return 2;
    }
    
    return 0;
}