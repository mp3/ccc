int main() {
    int i;
    int j;
    int count;
    
    count = 0;
    
    // Normal for loop
    for (i = 0; i < 3; i = i + 1) {
        count = count + 1;
    }
    
    // For loop with missing init
    i = 0;
    for (; i < 3; i = i + 1) {
        count = count + 1;
    }
    
    // For loop with variable declaration in init
    for (int k = 0; k < 2; k = k + 1) {
        count = count + 1;
    }
    
    // Nested for loops
    for (i = 0; i < 2; i = i + 1) {
        for (j = 0; j < 2; j = j + 1) {
            count = count + 1;
        }
    }
    
    // Should return 3 + 3 + 2 + 4 = 12
    return count;
}