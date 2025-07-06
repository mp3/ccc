int main() {
    int i;
    int sum;
    
    sum = 0;
    
    // Test break in while loop
    i = 0;
    while (1) {  // Infinite loop
        if (i == 5) {
            break;  // Exit when i reaches 5
        }
        sum = sum + i;
        i = i + 1;
    }
    
    // Test continue in for loop
    for (i = 0; i < 10; i = i + 1) {
        if (i == 5) {
            continue;  // Skip when i is 5
        }
        sum = sum + i;
    }
    
    // sum should be:
    // From while: 0+1+2+3+4 = 10
    // From for: 0+1+2+3+4+6+7+8+9 = 40
    // Total: 50
    return sum;
}