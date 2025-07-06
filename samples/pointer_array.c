int main() {
    int arr[3];
    int* p;
    
    // Initialize array
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    
    // Point to array element
    p = &arr[1];
    
    // Access via pointer
    return *p;  // Should return 20
}