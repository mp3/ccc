int main() {
    // Test bitwise AND
    int a = 12;   // 1100 in binary
    int b = 10;   // 1010 in binary
    int c = a & b; // should be 8 (1000 in binary)
    if (c != 8) return 1;
    
    // Test bitwise OR
    int d = a | b; // should be 14 (1110 in binary)
    if (d != 14) return 2;
    
    // Test bitwise XOR
    int e = a ^ b; // should be 6 (0110 in binary)
    if (e != 6) return 3;
    
    // Test bitwise NOT
    int f = ~0;    // should be -1 (all bits set)
    if (f != -1) return 4;
    
    int g = ~12;   // ~1100 = ...11110011 = -13 in two's complement
    if (g != -13) return 5;
    
    // Test left shift
    int h = 3 << 2; // 3 * 4 = 12
    if (h != 12) return 6;
    
    // Test right shift (arithmetic)
    int i = 12 >> 2; // 12 / 4 = 3
    if (i != 3) return 7;
    
    // Test right shift with negative number
    int j = -8 >> 2; // -8 / 4 = -2 (arithmetic shift)
    if (j != -2) return 8;
    
    // Test complex expression
    int k = (a & b) | (c ^ d);
    if (k != 14) return 9; // 8 | 6 = 14
    
    return 0;
}