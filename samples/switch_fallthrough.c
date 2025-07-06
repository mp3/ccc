int main() {
    int x = 2;
    int result = 0;
    
    switch (x) {
        case 1:
            result = result + 1;
        case 2:
            result = result + 2;  // Falls through from case 1 if x=1
        case 3:
            result = result + 3;  // Falls through from case 2
            break;
        default:
            result = 99;
    }
    
    // x = 2, so starts at case 2, adds 2, then falls through to case 3 and adds 3
    // Result should be 0 + 2 + 3 = 5
    return result;
}