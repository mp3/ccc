int main() {
    int a = 15;
    int b = 240;
    int f = ~a;
    
    // ~15 should be -16 in 32-bit two's complement
    // But for exit code purposes, we need to mask it to 8 bits
    // -16 & 0xFF = 240
    
    return f;  // This will return -16, which becomes 240 as exit code
}
