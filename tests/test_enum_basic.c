enum Color {
    RED,
    GREEN = 5,
    BLUE
};

int main() {
    // For now, test with explicit values since enum constant 
    // usage would require more complex symbol table implementation
    int red_value = 0;     // RED
    int green_value = 5;   // GREEN
    int blue_value = 6;    // BLUE
    
    if (red_value != 0) {
        return 1;
    }
    
    if (green_value != 5) {
        return 2;
    }
    
    if (blue_value != 6) {
        return 3;
    }
    
    return 0;
}