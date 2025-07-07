int counter() {
    static int count = 0;
    count = count + 1;
    return count;
}

int main() {
    // First call should return 1
    if (counter() != 1) {
        return 1;
    }
    
    // Second call should return 2 (static variable retains value)
    if (counter() != 2) {
        return 2;
    }
    
    // Third call should return 3
    if (counter() != 3) {
        return 3;
    }
    
    return 0;
}