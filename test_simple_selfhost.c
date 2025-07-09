// Very simple self-hosting test
// Tests basic features needed for CCC to compile itself

// Basic function
int add(int a, int b) {
    return a + b;
}

// Main function with basic features
int main() {
    // Variable declarations
    int x = 10;
    int y = 20;
    
    // Function call
    int sum = add(x, y);
    
    // Conditionals
    if (sum > 25) {
        puts("Sum is large");
    } else {
        puts("Sum is small");
    }
    
    // Loops
    int i = 0;
    while (i < 5) {
        i = i + 1;
    }
    
    // For loop
    for (int j = 0; j < 3; j++) {
        sum = sum + j;
    }
    
    return 0;
}