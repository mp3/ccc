int main() {
    // Test sizeof with arrays
    int arr[10];
    char str[20];
    
    int size_arr = sizeof(arr);    // Should be 4 * 10 = 40
    int size_str = sizeof(str);    // Should be 1 * 20 = 20
    
    if (size_arr != 40) return 1;
    if (size_str != 20) return 2;
    
    // Test sizeof with string literals
    int size_literal = sizeof("Hello");  // Should be 6 (includes null terminator)
    if (size_literal != 6) return 3;
    
    // Test sizeof with expressions
    int size_deref = sizeof(*(&arr[0]));  // Should be 4 (int)
    int size_addr = sizeof(&arr[0]);      // Should be 8 (pointer)
    
    if (size_deref != 4) return 4;
    if (size_addr != 8) return 5;
    
    return 0;  // All tests passed
}