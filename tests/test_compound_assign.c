// Test compound assignment operators
int main() {
    int a = 10;
    int b = 5;
    
    // Test +=
    a += b;  // a = 15
    
    // Test -=
    a -= 3;  // a = 12
    
    // Test *=
    a *= 2;  // a = 24
    
    // Test /=
    a /= 4;  // a = 6
    
    // Test with arrays
    int arr[5];
    arr[0] = 100;
    arr[1] = 50;
    arr[0] += arr[1];  // arr[0] = 150
    
    // Test with complex expressions
    int c = 2;
    c += a * b;  // c = 2 + 6 * 5 = 32
    
    return a + c;  // 6 + 32 = 38
}