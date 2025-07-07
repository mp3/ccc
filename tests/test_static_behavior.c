int increment() {
    static int count = 0;
    count = count + 1;
    return count;
}

int main() {
    int a = increment();  // Should be 1
    int b = increment();  // Should be 2
    int c = increment();  // Should be 3
    return c;  // Return 3
}