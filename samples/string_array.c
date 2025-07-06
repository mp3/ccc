int main() {
    char* msg;
    char first;
    
    msg = "Hello";
    
    // Access first character of string
    first = *msg;
    
    return first;  // Should return 72 ('H')
}