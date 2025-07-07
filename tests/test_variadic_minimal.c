// Minimal variadic function test
// Just parse the syntax without full stdarg.h support

int printf(const char *format, ...);

int sum(int count, ...) {
    // For now, just return count
    return count;
}

int main() {
    int result = sum(3, 10, 20, 30);
    return result;  // Should return 3
}