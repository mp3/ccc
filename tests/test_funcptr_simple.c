int add(int a, int b) {
    return a + b;
}

int subtract(int a, int b) {
    return a - b;
}

int main() {
    // Function pointer declaration
    int (*op)(int, int);
    
    // Assign function to pointer
    op = add;
    int result1 = op(10, 5);  // Should be 15
    
    // Reassign to different function
    op = subtract;
    int result2 = op(10, 5);  // Should be 5
    
    return result1 + result2;  // Should be 20
}