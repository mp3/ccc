/*
 * Test: Type Casting
 * Expected Output: 65
 * Description: Tests explicit type casting between different types
 */

int main() {
    // Test int to char casting
    int ascii_value = 65;
    char letter = (char)ascii_value;     // 'A'
    
    // Test char to int casting  
    char ch = 'B';
    int ch_value = (int)ch;              // 66
    
    // Test pointer casting
    int number = 42;
    int *int_ptr = &number;
    char *char_ptr = (char*)int_ptr;
    void *void_ptr = (void*)int_ptr;
    int *back_to_int = (int*)void_ptr;
    int restored_value = *back_to_int;   // 42
    
    // Test casting in expressions
    int a = 10;
    int b = 3;
    char result_char = (char)(a + b);    // 13 as char
    int result_int = (int)result_char;   // 13 as int
    
    // Test casting with arithmetic
    char small1 = 5;
    char small2 = 10;
    int sum = (int)small1 + (int)small2; // 15
    
    // Test casting function return values
    char get_char_val = (char)65;        // 'A'
    int char_to_int = (int)get_char_val; // 65
    
    // Test casting in array context
    int values[3];
    values[0] = (int)'A';                // 65
    values[1] = (int)'B';                // 66  
    values[2] = (int)'C';                // 67
    
    char chars[3];
    chars[0] = (char)values[0];          // 'A'
    chars[1] = (char)values[1];          // 'B'
    chars[2] = (char)values[2];          // 'C'
    
    // Test casting with pointers and arrays
    int *int_array_ptr = values;
    char *byte_view = (char*)int_array_ptr;
    
    // Test nested casting
    int nested = (int)(char)(int)'Z';    // 'Z' -> int -> char -> int = 90
    
    // Test casting zero
    char null_char = (char)0;            // '\0'
    int null_int = (int)null_char;       // 0
    
    // Return ASCII value of 'A' which is 65
    return (int)letter;
}