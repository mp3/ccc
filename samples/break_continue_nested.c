int main() {
    int i;
    int j;
    int count;
    
    count = 0;
    
    // Test break in nested loops - break only exits inner loop
    for (i = 0; i < 3; i = i + 1) {
        for (j = 0; j < 5; j = j + 1) {
            if (j == 2) {
                break;  // Exit inner loop when j == 2
            }
            count = count + 1;
        }
    }
    // Should count: 3 * 2 = 6 (3 outer iterations, each with 2 inner)
    
    // Test continue in nested loops
    for (i = 0; i < 2; i = i + 1) {
        for (j = 0; j < 3; j = j + 1) {
            if (j == 1) {
                continue;  // Skip when j == 1
            }
            count = count + 1;
        }
    }
    // Should count: 2 * 2 = 4 (2 outer iterations, each with 2 inner (0 and 2))
    
    // Total: 6 + 4 = 10
    return count;
}