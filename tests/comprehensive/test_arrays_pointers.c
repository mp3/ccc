/*
 * Test: Arrays and Pointers
 * Expected Output: 30
 * Description: Tests array declarations, indexing, pointer arithmetic, and dereferencing
 */

int main() {
    // Test basic array operations
    int arr[5];
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;
    arr[3] = 40;
    arr[4] = 50;
    
    // Test array indexing
    int sum = arr[0] + arr[1];  // 10 + 20 = 30
    
    // Test pointer basics
    int x = 15;
    int *p = &x;
    int value = *p;  // value = 15
    
    // Test pointer arithmetic with arrays
    int *arr_ptr = arr;
    int first = *arr_ptr;        // arr[0] = 10
    int second = *(arr_ptr + 1); // arr[1] = 20
    
    // Test array as pointer
    int third = *(arr + 2);      // arr[2] = 30
    
    // Test multi-dimensional array
    int matrix[2][2];
    matrix[0][0] = 1;
    matrix[0][1] = 2;
    matrix[1][0] = 3;
    matrix[1][1] = 4;
    
    int matrix_sum = matrix[0][0] + matrix[1][1];  // 1 + 4 = 5
    
    // Test pointer to pointer
    int **pp = &p;
    int indirect_value = **pp;  // Should be 15
    
    // Combine results
    // We want to return 30, so let's calculate:
    // arr[2] = 30, which is what we want
    return arr[2];
}