int main() {
    // Test const with different types
    const int a = 10;
    int const b = 20;  // Same as const int
    const char c = 'A';
    char const d = 'B';
    
    // Test const in expressions
    int result = a + b;
    result = result + (int)c + (int)d;
    
    // Test const pointers
    const int *p = &a;
    int x = *p;
    result = result + x;
    
    return result;  // Should be 10+20+65+66+10 = 171
}