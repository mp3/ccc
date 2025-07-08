// Test modulo operator
int main() {
    int a = 17;
    int b = 5;
    int c = a % b;  // 17 % 5 = 2
    
    int d = 100;
    int e = 7;
    int f = d % e;  // 100 % 7 = 2
    
    // Test with negative numbers
    int g = -17;
    int h = 5;
    int i = g % h;  // -17 % 5 = -2
    
    // Test modulo in expressions
    int j = (a + d) % (b + e);  // (17 + 100) % (5 + 7) = 117 % 12 = 9
    
    return c + f + i + j;  // 2 + 2 + (-2) + 9 = 11
}