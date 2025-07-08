int main() {
    int x;      // uninitialized
    int y = 5;  // initialized
    int z;      // uninitialized and unused
    
    x = 10;     // x gets initialized here
    
    return x + y;  // x and y are used, z is not
}