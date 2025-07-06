int main() {
    int i = 0;
    int result = 0;
    
    // Test break and continue in do-while
    do {
        i = i + 1;
        
        if (i == 3) {
            continue;  // Skip when i is 3
        }
        
        result = result + i;
        
        if (i == 5) {
            break;  // Exit when i reaches 5
        }
    } while (i < 10);
    
    // result should be 1 + 2 + 4 + 5 = 12 (skipped 3)
    return result;
}