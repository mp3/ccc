int main() {
    // Test prefix increment
    int a = 5;
    int b = ++a;  // a becomes 6, b gets 6
    if (a != 6) return 1;
    if (b != 6) return 2;
    
    // Test postfix increment
    int c = 10;
    int d = c++;  // c becomes 11, d gets 10
    if (c != 11) return 3;
    if (d != 10) return 4;
    
    // Test prefix decrement
    int e = 8;
    int f = --e;  // e becomes 7, f gets 7
    if (e != 7) return 5;
    if (f != 7) return 6;
    
    // Test postfix decrement
    int g = 15;
    int h = g--;  // g becomes 14, h gets 15
    if (g != 14) return 7;
    if (h != 15) return 8;
    
    // Test with array elements
    int arr[3];
    arr[0] = 20;
    arr[1] = arr[0]++;  // arr[0] becomes 21, arr[1] gets 20
    if (arr[0] != 21) return 9;
    if (arr[1] != 20) return 10;
    
    // Test with pointers
    int x = 30;
    int *p = &x;
    (*p)++;  // x becomes 31
    if (x != 31) return 11;
    
    ++(*p);  // x becomes 32
    if (x != 32) return 12;
    
    // Test in expressions
    int i = 5;
    int j = 10;
    int k = i++ + ++j;  // i becomes 6, j becomes 11, k = 5 + 11 = 16
    if (i != 6) return 13;
    if (j != 11) return 14;
    if (k != 16) return 15;
    
    // Test multiple increments
    int m = 1;
    m++;
    m++;
    ++m;
    if (m != 4) return 16;
    
    return 0;
}