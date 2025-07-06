int main() {
    int arr[5];
    int* p;
    int* q;
    int offset;
    int diff;
    
    p = &arr[0];
    q = p + 2;    // Pointer + integer
    p = q - 1;    // Pointer - integer  
    diff = q - p; // Pointer - pointer
    
    return diff;
}