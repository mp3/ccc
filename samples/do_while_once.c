int main() {
    int x = 0;
    int count = 0;
    
    // This do-while executes exactly once since condition is false
    do {
        count = count + 1;
        x = 42;
    } while (0);  // Always false
    
    // count should be 1 (body executed once)
    // x should be 42
    if (count != 1) return 1;
    return x;
}