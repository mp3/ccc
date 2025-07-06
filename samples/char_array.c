int main() {
    char str[10];
    
    // Store some characters
    str[0] = 'H';
    str[1] = 'e';
    str[2] = 'l';
    str[3] = 'l';
    str[4] = 'o';
    str[5] = ' ';
    str[6] = 'C';
    str[7] = 'C';
    str[8] = 'C';
    str[9] = '\0';
    
    // Return the first character's ASCII value
    return str[0];  // Should return 72 ('H')
}