# 1 "test_function_macros.c"

int main(void) {
    int x = 10;
    int y = 20;
    
    int max_val = ((x) > (y) ? (x) : (y));
    int min_val = ((x) < (y) ? (x) : (y));
    int square = ((5) * (5));
    int sum = ((3) + (4));
    
    // Test nested macro calls
    int nested = ((SQUARE(3)) > (ADD(4, 5)) ? (SQUARE(3)) : (ADD(4, 5)));
    
    return max_val + min_val + square + sum + nested;
}
