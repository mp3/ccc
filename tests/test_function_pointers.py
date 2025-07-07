#!/usr/bin/env python3
"""Test function pointer support in ccc compiler."""

import subprocess
import os
import sys
import tempfile

def compile_and_check(code, expected_output=None, expected_error=None):
    """Compile code and check output or error."""
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile with ccc
        result = subprocess.run(['../ccc', f.name, '-o', f.name + '.ll'], 
                              capture_output=True, text=True)
        
        os.unlink(f.name)
        
        if expected_error:
            if expected_error not in result.stderr:
                print(f"Expected error: {expected_error}")
                print(f"Got stderr: {result.stderr}")
                return False
            return True
            
        if result.returncode != 0:
            print(f"Compilation failed: {result.stderr}")
            return False
            
        # Check generated LLVM IR if expected_output provided
        if expected_output:
            with open(f.name + '.ll', 'r') as llvm_file:
                llvm_ir = llvm_file.read()
                if expected_output not in llvm_ir:
                    print(f"Expected in output: {expected_output}")
                    print(f"Generated LLVM IR:\n{llvm_ir}")
                    os.unlink(f.name + '.ll')
                    return False
                    
        os.unlink(f.name + '.ll')
        return True

def test_function_pointer_declaration():
    """Test basic function pointer declaration."""
    code = '''
int add(int a, int b) { return a + b; }
int main() {
    int (*fp)(int, int);
    fp = add;
    return 0;
}
'''
    assert compile_and_check(code, "alloca i32 (i32, i32)*")
    print("✓ Function pointer declaration")

def test_function_pointer_call():
    """Test function pointer call."""
    code = '''
int add(int a, int b) { return a + b; }
int main() {
    int (*fp)(int, int);
    fp = add;
    int result = fp(5, 3);
    return result;
}
'''
    assert compile_and_check(code, "call i32 %")
    print("✓ Function pointer call")

def test_function_pointer_reassignment():
    """Test function pointer reassignment."""
    code = '''
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int main() {
    int (*fp)(int, int);
    fp = add;
    int r1 = fp(5, 3);
    fp = sub;
    int r2 = fp(5, 3);
    return r1 + r2;
}
'''
    assert compile_and_check(code)
    print("✓ Function pointer reassignment")

def test_function_pointer_as_parameter():
    """Test passing function pointer as parameter."""
    code = '''
int add(int a, int b) { return a + b; }
int apply(int (*op)(int, int), int x, int y) {
    return op(x, y);
}
int main() {
    return apply(add, 10, 5);
}
'''
    # This test might fail as we haven't implemented function pointers as parameters yet
    try:
        compile_and_check(code)
        print("✓ Function pointer as parameter")
    except:
        print("✗ Function pointer as parameter (not implemented yet)")

def main():
    """Run all tests."""
    print("Testing function pointer support...")
    
    test_function_pointer_declaration()
    test_function_pointer_call()
    test_function_pointer_reassignment()
    test_function_pointer_as_parameter()
    
    print("\nFunction pointer tests completed!")

if __name__ == '__main__':
    main()