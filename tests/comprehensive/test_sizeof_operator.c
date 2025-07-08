/*
 * Test: Sizeof Operator
 * Expected Output: 4
 * Description: Tests sizeof operator with various types and expressions
 */

struct TestStruct {
    int a;
    char b;
    int c;
};

union TestUnion {
    int i;
    char c[4];
};

int main() {
    // Test sizeof with basic types
    int size_int = sizeof(int);          // Should be 4
    int size_char = sizeof(char);        // Should be 1
    int size_ptr = sizeof(void*);        // Should be 8 (on 64-bit)
    
    // Test sizeof with variables
    int var = 42;
    char ch = 'A';
    int var_size = sizeof(var);          // Should be 4
    int ch_size = sizeof(ch);            // Should be 1
    
    // Test sizeof with arrays
    int arr[10];
    char str[20];
    int arr_size = sizeof(arr);          // Should be 40 (10 * 4)
    int str_size = sizeof(str);          // Should be 20
    
    // Test sizeof with pointers
    int *ptr = &var;
    char *str_ptr = str;
    int ptr_size = sizeof(ptr);          // Should be 8
    int str_ptr_size = sizeof(str_ptr);  // Should be 8
    
    // Test sizeof with structures
    struct TestStruct s;
    int struct_size = sizeof(struct TestStruct);  // Should be 12 (4+1+3+4 with padding)
    int s_size = sizeof(s);              // Should be 12
    
    // Test sizeof with unions  
    union TestUnion u;
    int union_size = sizeof(union TestUnion);     // Should be 4
    int u_size = sizeof(u);              // Should be 4
    
    // Test sizeof with expressions
    int expr_size = sizeof(var + ch);    // Should be 4 (result is int)
    int literal_size = sizeof(42);       // Should be 4
    int char_literal_size = sizeof('A'); // Should be 4 (char literals are int in C)
    
    // Test sizeof with array elements
    int element_size = sizeof(arr[0]);   // Should be 4
    int str_element_size = sizeof(str[0]); // Should be 1
    
    // Test sizeof with pointer dereferencing
    int deref_size = sizeof(*ptr);       // Should be 4
    int str_deref_size = sizeof(*str_ptr); // Should be 1
    
    // Test sizeof with struct members
    int member_size = sizeof(s.a);       // Should be 4
    int char_member_size = sizeof(s.b);  // Should be 1
    
    // Calculate array length using sizeof
    int arr_length = sizeof(arr) / sizeof(arr[0]); // 40 / 4 = 10
    
    // Test sizeof in expressions
    int total_bytes = sizeof(int) + sizeof(char); // 4 + 1 = 5
    
    // Return sizeof(int) which should be 4
    return sizeof(int);
}