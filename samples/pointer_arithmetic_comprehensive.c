int main() {
    char str[10];
    char* cp1;
    char* cp2;
    int arr[5];
    int* ip1;
    int* ip2;
    
    // Test char pointer arithmetic (1-byte elements)
    cp1 = &str[0];
    cp2 = cp1 + 3;  // Should advance by 3 bytes
    
    // Test int pointer arithmetic (4-byte elements)  
    ip1 = &arr[0];
    ip2 = ip1 + 2;  // Should advance by 8 bytes (2 * 4)
    
    // Test pointer differences
    // For char pointers: should be 3
    // For int pointers: should be 2
    return (cp2 - cp1) + (ip2 - ip1);  // Should return 5
}