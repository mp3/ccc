int add(int a, int b) {
    return a + b;
}

int main() {
    int (*op)(int, int);
    op = add;
    
    // Test with explicit arguments
    int arg1 = 10;
    int arg2 = 5;
    int result = op(arg1, arg2);
    
    return result;
}