/*
 * Test: Strings and Characters
 * Expected Output: 72
 * Description: Tests character literals, string literals, and character operations
 */

int main() {
    // Test character literals
    char ch1 = 'A';      // ASCII 65
    char ch2 = 'B';      // ASCII 66
    char ch3 = 'H';      // ASCII 72
    
    // Test escape sequences
    char newline = '\n';  // ASCII 10
    char tab = '\t';      // ASCII 9
    char quote = '\'';    // ASCII 39
    char backslash = '\\'; // ASCII 92
    
    // Test string literals
    char *str1 = "Hello";
    char *str2 = "World";
    char *str3 = "Test\n";
    
    // Test character arrays
    char arr[6];
    arr[0] = 'H';   // 72
    arr[1] = 'e';   // 101
    arr[2] = 'l';   // 108
    arr[3] = 'l';   // 108
    arr[4] = 'o';   // 111
    arr[5] = '\0';  // 0 (null terminator)
    
    // Test character arithmetic
    char start = 'A';     // 65
    char next = start + 1; // 66 ('B')
    char offset = next - start; // 1
    
    // Test character comparisons
    int is_upper = (ch1 >= 'A' && ch1 <= 'Z');  // 1 (true)
    int is_digit = (ch1 >= '0' && ch1 <= '9');  // 0 (false)
    
    // Test string indexing
    char first_char = str1[0];   // 'H' = 72
    char second_char = str1[1];  // 'e' = 101
    
    // Test pointer arithmetic with strings
    char *ptr = str1;
    char first_via_ptr = *ptr;     // 'H' = 72
    char second_via_ptr = *(ptr + 1); // 'e' = 101
    
    // Test character casting
    int char_as_int = (int)ch3;    // 72
    char int_as_char = (char)72;   // 'H'
    
    // Test character in array access
    char indexed_char = arr[ch3 - 72]; // arr[0] = 'H' = 72
    
    // Return ASCII value of 'H' which is 72
    return (int)ch3;
}