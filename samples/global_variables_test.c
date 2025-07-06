int global_counter = 42;
char global_flag = 65;  // 'A'
int* global_ptr;

int main() {
    int local_var;
    
    local_var = global_counter;
    global_counter = global_counter + 1;
    global_flag = global_flag + 1;  // Should become 'B' (66)
    
    return global_counter - local_var;  // Should return 1
}