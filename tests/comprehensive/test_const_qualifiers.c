/*
 * Test: Const Qualifiers
 * Expected Output: 50
 * Description: Tests const variable declarations and usage
 */

int main() {
    // Test basic const variables
    const int max_size = 100;
    const int min_size = 10;
    const char grade = 'A';
    
    // Test const with initialization
    const int half_max = max_size / 2;  // 50
    
    // Test const in expressions
    int range = max_size - min_size;    // 90
    int average = range / 2 + min_size; // 45 + 10 = 55
    
    // Test const with pointers (the value pointed to is const)
    int value = 25;
    const int *ptr_to_const = &value;
    int const_value = *ptr_to_const;    // 25
    
    // Test const pointer (the pointer itself is const)
    int another_value = 75;
    int * const const_ptr = &value;
    int pointed_value = *const_ptr;     // 25
    
    // Test const in array context
    const int arr_size = 5;
    int arr[5];  // Note: Using literal 5 since const vars can't be used for array size in C89
    arr[0] = max_size;          // 100
    arr[1] = half_max;          // 50
    arr[2] = min_size;          // 10
    arr[3] = (int)grade;        // 65 ('A')
    arr[4] = const_value;       // 25
    
    // Test const in calculations
    const int multiplier = 2;
    int doubled = half_max * multiplier; // 50 * 2 = 100
    
    // Test const character
    const char letter = 'B';
    int letter_code = (int)letter;       // 66
    
    // Use const values in final calculation
    // We want to return 50, which is half_max
    return half_max;
}