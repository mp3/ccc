union Data {
    int i;
    char c;
};

int main() {
    union Data data;
    
    // Set as integer
    data.i = 65;
    
    // Read as character (should be 'A')
    if (data.c != 65) {
        return 1;
    }
    
    // Set as character
    data.c = 66;
    
    // Lower byte should be 66
    if ((data.i & 255) != 66) {
        return 2;
    }
    
    return 0;
}
