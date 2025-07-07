enum Color {
    RED,
    GREEN = 5,
    BLUE
};

enum {
    ANONYMOUS_A = 10,
    ANONYMOUS_B,
    ANONYMOUS_C = 20
};

int main() {
    int color = RED;
    int value = GREEN;
    
    if (color != 0) {
        return 1;
    }
    
    if (value != 5) {
        return 2;
    }
    
    if (BLUE != 6) {
        return 3;
    }
    
    if (ANONYMOUS_A != 10) {
        return 4;
    }
    
    if (ANONYMOUS_B != 11) {
        return 5;
    }
    
    if (ANONYMOUS_C != 20) {
        return 6;
    }
    
    return 0;
}