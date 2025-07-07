int main() {
    // Test const with different types
    const int a = 10;
    int const b = 20;  // Same as const int
    const char c = 'A';
    char const d = 'B';
    
    // Test const in expressions
    int result = a + b;
    result = result + (int)c + (int)d;
    
    // Test const arrays
    const int arr[3] = {1, 2, 3};
    result = result + arr[0] + arr[1] + arr[2];
    
    return result;  // Should be 10+20+65+66+1+2+3 = 167
}