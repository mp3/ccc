int main() {
    char* greeting;
    char first_char;
    int i;
    
    greeting = "Hello, World!\n";
    
    // Get first character
    first_char = *greeting;
    
    // Iterate through string manually
    i = 0;
    while (i < 5) {
        char c;
        char* p;
        
        p = greeting;
        p = p + i;  // Simplified pointer arithmetic
        c = *p;
        
        i = i + 1;
    }
    
    return first_char;  // Returns 'H' = 72
}