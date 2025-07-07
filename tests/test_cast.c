int main() {
    // Basic int to char cast
    int a = 65;
    char b = (char)a;
    if (b != 65) return 1;
    
    // Char to int cast (implicit promotion happens anyway)
    char c = 'A';
    int d = (int)c;
    if (d != 65) return 2;
    
    // Cast in expressions
    int e = (int)'B' + 1;
    if (e != 67) return 3;
    
    // Cast with arithmetic
    int f = 300;
    char g = (char)f;  // Should truncate to 44 (300 & 0xFF)
    if (g != 44) return 4;
    
    // Cast negative numbers
    int h = -1;
    char i = (char)h;  // Should be 255 when zero-extended back
    if (i != 255) return 5;
    
    // Pointer casts
    int j = 42;
    int *p = &j;
    char *q = (char*)p;
    int *r = (int*)q;
    if (*r != 42) return 6;
    
    // Integer to pointer cast
    int addr = 0;
    int *s = (int*)addr;  // NULL pointer
    if (s != 0) return 7;
    
    // Cast in function calls
    putchar((int)'H');
    putchar((int)'i');
    putchar((int)'\n');
    
    return 0;
}