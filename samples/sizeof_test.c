int main() {
    // Test sizeof with basic types
    int size_int = sizeof(int);        // Should be 4
    int size_char = sizeof(char);      // Should be 1
    int size_ptr = sizeof(int*);       // Should be 8 (64-bit)
    int size_char_ptr = sizeof(char*); // Should be 8 (64-bit)
    
    // Verify sizes
    if (size_int != 4) return 1;
    if (size_char != 1) return 2;
    if (size_ptr != 8) return 3;
    if (size_char_ptr != 8) return 4;
    
    // Test sizeof with variables
    int x = 42;
    char c = 'A';
    int *p = &x;
    
    int size_x = sizeof(x);     // Should be 4
    int size_c = sizeof(c);     // Should be 1
    int size_p = sizeof(p);     // Should be 8
    
    if (size_x != 4) return 5;
    if (size_c != 1) return 6;
    if (size_p != 8) return 7;
    
    return 0;  // All tests passed
}