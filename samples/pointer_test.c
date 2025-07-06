int main() {
    int x;
    int* p;
    
    // Basic pointer assignment
    x = 42;
    p = &x;
    
    // Dereference the pointer
    return *p;  // Should return 42
}