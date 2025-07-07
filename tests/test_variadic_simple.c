#include <stdarg.h>

// Simple variadic function to sum integers
int sum(int count, ...) {
    va_list args;
    va_start(args, count);
    
    int total = 0;
    for (int i = 0; i < count; i++) {
        total += va_arg(args, int);
    }
    
    va_end(args);
    return total;
}

int main() {
    int result1 = sum(3, 10, 20, 30);  // Should return 60
    int result2 = sum(2, 5, 15);       // Should return 20
    return result1 - 40;  // Should return 20
}