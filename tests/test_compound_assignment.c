int main() {
    // Test +=
    int a = 10;
    a += 5;
    if (a != 15) return 1;
    
    // Test -=
    int b = 20;
    b -= 8;
    if (b != 12) return 2;
    
    // Test *=
    int c = 6;
    c *= 7;
    if (c != 42) return 3;
    
    // Test /=
    int d = 100;
    d /= 4;
    if (d != 25) return 4;
    
    // Test with array elements
    int arr[5];
    arr[0] = 10;
    arr[0] += 10;
    if (arr[0] != 20) return 5;
    
    // Test with pointers
    int x = 50;
    int *p = &x;
    *p += 10;
    if (x != 60) return 6;
    
    // Test chaining
    int e = 1;
    int f = 2;
    e += f += 3;  // f becomes 5, then e becomes 6
    if (e != 6 || f != 5) return 7;
    
    return 0;
}