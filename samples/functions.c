int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    int result = x * y;
    return result;
}

int factorial(int n) {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

int main() {
    int sum = add(5, 3);
    int product = multiply(4, 7);
    int fact = factorial(5);
    
    return sum + product + fact;  // 8 + 28 + 120 = 156
}