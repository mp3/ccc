int main() {
    int x;
    int y;
    int* p;
    
    x = 10;
    p = &x;
    *p = 20;  // x should now be 20
    
    y = x;
    return y;  // Should return 20
}