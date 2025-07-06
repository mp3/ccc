int main() {
    int x = 99;  // Not in any case
    int result = 0;
    
    switch (x) {
        case 1:
            result = 10;
            break;
        case 2:
            result = 20;
            break;
        case 3:
            result = 30;
            break;
        default:
            result = 42;  // Should hit this
            break;
    }
    
    return result;
}