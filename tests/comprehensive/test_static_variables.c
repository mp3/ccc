/*
 * Test: Static Variables
 * Expected Output: 10
 * Description: Tests static variables in functions and global scope
 */

// Global static variable
static int global_counter = 0;

// Function with static variable
int increment_counter() {
    static int local_counter = 0;
    local_counter++;
    global_counter++;
    return local_counter;
}

// Another function with static variable
int get_value() {
    static int value = 5;
    return value;
}

// Function that modifies static variable
void modify_static() {
    static int internal = 100;
    internal = internal / 10;  // internal becomes 10
}

// Function that returns static variable value
int get_modified_static() {
    static int internal = 100;
    internal = internal / 10;  // internal becomes 10 on first call
    return internal;
}

int main() {
    // Test static variable persistence
    int first_call = increment_counter();   // local_counter = 1, global_counter = 1
    int second_call = increment_counter();  // local_counter = 2, global_counter = 2
    int third_call = increment_counter();   // local_counter = 3, global_counter = 3
    
    // Test static variable initialization
    int static_value = get_value();         // Should be 5
    
    // Test static variable in different function instances
    modify_static();  // Modifies internal static variable
    int modified_value = get_modified_static();  // Should be 10
    
    // Test global static
    int global_val = global_counter;        // Should be 3
    
    // Calculate result
    // We want to return 10
    // modified_value should be 10
    return modified_value;
}