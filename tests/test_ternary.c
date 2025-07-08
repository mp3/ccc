// Test ternary operator
int main() {
    int a = 10;
    int b = 5;
    
    // Basic ternary
    int max = a > b ? a : b;  // max = 10
    
    // Nested ternary
    int x = 15;
    int y = 20;
    int z = 18;
    int middle = x > y ? (x > z ? (y > z ? y : z) : x) : (y > z ? (x > z ? x : z) : y);  // middle = 18
    
    // Ternary in expression
    int result = (a < b ? a : b) + (x > y ? x : y);  // 5 + 20 = 25
    
    // Ternary with side effects (but we avoid them in test)
    int flag = 1;
    int value = flag ? 100 : 200;  // value = 100
    
    return max + middle + result + value;  // 10 + 18 + 25 + 100 = 153
}
EOF < /dev/null