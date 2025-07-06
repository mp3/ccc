int main() {
    int n = 5;
    int result = 1;
    int i = 1;
    
    while (i <= n) {
        result = result * i;
        i = i + 1;
    }
    
    if (result == 120) {
        return 1;  // Success
    } else {
        return 0;  // Failure
    }
}