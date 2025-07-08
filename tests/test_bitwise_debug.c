int main() {
    int a = 15;   // 0x0F
    int b = 240;  // 0xF0
    
    int c = a & b;  // 0
    int d = a | b;  // 255
    int e = a ^ b;  // 255
    int f = ~a;     // -16
    int g = a << 2; // 60
    int h = b >> 2; // 60
    
    // Let's return each component separately to debug
    // return c;     // Should be 0
    // return d;     // Should be 255
    // return e;     // Should be 255
    // return f;     // Should be -16 (240 in unsigned)
    // return g;     // Should be 60
    // return h;     // Should be 60
    
    // Let's test the sum step by step
    int sum1 = c + d;       // 0 + 255 = 255
    int sum2 = sum1 + e;    // 255 + 255 = 510
    int sum3 = sum2 + f;    // 510 + (-16) = 494
    int sum4 = sum3 + g;    // 494 + 60 = 554
    int sum5 = sum4 + h;    // 554 + 60 = 614
    
    return sum5;
}