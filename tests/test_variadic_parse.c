// Test variadic function parsing

int sum(int count, ...) {
    // For now, just return count
    return count;
}

int main() {
    int result = sum(3, 10, 20, 30);
    return result;  // Should return 3
}