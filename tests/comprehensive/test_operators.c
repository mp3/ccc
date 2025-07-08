/*
 * Test: All Operators
 * Expected Output: 7
 * Description: Tests arithmetic, logical, bitwise, comparison, and assignment operators
 */

int main() {
    // Test arithmetic operators
    int a = 10;
    int b = 3;
    int add_result = a + b;    // 13
    int sub_result = a - b;    // 7
    int mul_result = a * b;    // 30
    int div_result = a / b;    // 3
    int mod_result = a % b;    // 1
    
    // Test comparison operators
    int comp1 = (a > b);       // 1 (true)
    int comp2 = (a < b);       // 0 (false)
    int comp3 = (a == 10);     // 1 (true)
    int comp4 = (b != 5);      // 1 (true)
    int comp5 = (a >= 10);     // 1 (true)
    int comp6 = (b <= 3);      // 1 (true)
    
    // Test logical operators
    int log1 = comp1 && comp3; // 1 && 1 = 1
    int log2 = comp2 || comp4; // 0 || 1 = 1
    int log3 = !comp2;         // !0 = 1
    
    // Test bitwise operators
    int bit1 = a & b;          // 10 & 3 = 2 (1010 & 0011 = 0010)
    int bit2 = a | b;          // 10 | 3 = 11 (1010 | 0011 = 1011)
    int bit3 = a ^ b;          // 10 ^ 3 = 9 (1010 ^ 0011 = 1001)
    int bit4 = ~a;             // ~10 = -11 (bitwise NOT)
    int bit5 = a << 1;         // 10 << 1 = 20
    int bit6 = a >> 1;         // 10 >> 1 = 5
    
    // Test increment/decrement
    int c = 5;
    int pre_inc = ++c;         // c = 6, pre_inc = 6
    int post_inc = c++;        // post_inc = 6, c = 7
    int pre_dec = --c;         // c = 6, pre_dec = 6
    int post_dec = c--;        // post_dec = 6, c = 5
    
    // Test assignment operators
    int d = 10;
    d += 5;                    // d = 15
    d -= 3;                    // d = 12
    d *= 2;                    // d = 24
    d /= 4;                    // d = 6
    
    // Test ternary operator
    int ternary = (a > b) ? 100 : 200;  // Should be 100
    
    // Test address and dereference
    int e = 42;
    int *ptr = &e;
    int deref_val = *ptr;      // Should be 42
    
    // Test sizeof operator
    int size_int = sizeof(int);    // Should be 4
    int size_ptr = sizeof(ptr);    // Should be 8 (on 64-bit)
    
    // Return 7 (which is sub_result)
    return sub_result;
}