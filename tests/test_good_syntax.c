// Test file to demonstrate successful compilation with error handling enabled

int main() {
    int x = 5;
    int y = 10;
    
    // This should compile successfully
    int z = x + y;
    
    if (x > 5) {
        y = 20;
    }
    
    while (x < 10) {
        x++;
    }
    
    return z;
}