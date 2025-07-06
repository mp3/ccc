int main() {
    int i;
    int sum;
    
    sum = 0;
    
    // Basic for loop
    for (i = 0; i < 5; i = i + 1) {
        sum = sum + i;
    }
    
    // Should return 0 + 1 + 2 + 3 + 4 = 10
    return sum;
}