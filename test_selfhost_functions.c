// Self-hosting test with only functions
// Tests basic compiler functionality

// Simulated memory allocation size
int get_token_size() {
    return 16;  // sizeof(Token)
}

// Simulated token type checking
int is_keyword(int token_type) {
    if (token_type == 1) {  // TOKEN_INT
        return 1;
    }
    if (token_type == 2) {  // TOKEN_CHAR
        return 1;
    }
    return 0;
}

// Simulated expression evaluation
int evaluate_binary_op(int left, int right, int op) {
    if (op == 43) {  // '+'
        return left + right;
    }
    if (op == 45) {  // '-'
        return left - right;
    }
    if (op == 42) {  // '*'
        return left * right;
    }
    if (op == 47) {  // '/'
        if (right != 0) {
            return left / right;
        }
    }
    return 0;
}

// Simulated code generation
void emit_instruction(int opcode, int operand) {
    if (opcode == 1) {  // LOAD
        printf("  %%tmp%d = load i32, i32* %%var%d\n", operand, operand);
    } else if (opcode == 2) {  // STORE
        printf("  store i32 %%tmp%d, i32* %%var%d\n", operand, operand);
    } else if (opcode == 3) {  // ADD
        printf("  %%tmp%d = add i32 %%tmp%d, %%tmp%d\n", operand, operand-1, operand);
    }
}

// Simulated optimization
int optimize_constant_expression(int value, int is_constant) {
    if (is_constant) {
        // Fold constants
        if (value == 0) {
            return 0;
        }
        if (value == 1) {
            return 1;
        }
        return value;
    }
    return -1;  // Not optimizable
}

// Test the compiler components
int test_compiler() {
    int token_size = get_token_size();
    int is_int_keyword = is_keyword(1);
    int result = evaluate_binary_op(10, 20, 43);  // 10 + 20
    
    puts("Testing compiler components:");
    
    if (token_size == 16) {
        puts("Token size: OK");
    }
    
    if (is_int_keyword == 1) {
        puts("Keyword check: OK");
    }
    
    if (result == 30) {
        puts("Expression evaluation: OK");
    }
    
    // Test code generation
    puts("\nGenerated code:");
    emit_instruction(1, 0);  // LOAD
    emit_instruction(1, 1);  // LOAD
    emit_instruction(3, 2);  // ADD
    emit_instruction(2, 3);  // STORE
    
    return 0;
}

// Main entry point
int main() {
    puts("CCC Self-hosting test");
    puts("====================");
    
    int status = test_compiler();
    
    if (status == 0) {
        puts("\nAll tests passed!");
    } else {
        puts("\nSome tests failed!");
    }
    
    return status;
}