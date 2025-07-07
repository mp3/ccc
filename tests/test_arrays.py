#!/usr/bin/env python3
"""Test array support."""

import subprocess
import os
import tempfile

def test_int_array():
    """Test integer array declaration and access."""
    code = """
    int main() {
        int arr[5];
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        arr[3] = 40;
        arr[4] = 50;
        
        return arr[2];  // Should return 30
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['./ccc', f.name, '-o', 'test_int_array.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output contains array allocation
        with open('test_int_array.ll', 'r') as out:
            content = out.read()
            assert 'alloca [5 x i32]' in content
            assert 'getelementptr [5 x i32]' in content
            
        os.unlink(f.name)
        os.unlink('test_int_array.ll')

def test_char_array():
    """Test character array declaration and access."""
    code = """
    int main() {
        char str[10];
        str[0] = 'H';
        str[1] = 'e';
        str[2] = 'l';
        str[3] = 'l';
        str[4] = 'o';
        
        return str[0];  // Should return 72 ('H')
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['./ccc', f.name, '-o', 'test_char_array.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output contains array allocation
        with open('test_char_array.ll', 'r') as out:
            content = out.read()
            assert 'alloca [10 x i8]' in content
            assert 'getelementptr [10 x i8]' in content
            assert 'add i8 0, 72' in content  # 'H'
            
        os.unlink(f.name)
        os.unlink('test_char_array.ll')

def test_array_with_variables():
    """Test array access with variable indices."""
    code = """
    int main() {
        int arr[3];
        int i;
        
        arr[0] = 100;
        arr[1] = 200;
        arr[2] = 300;
        
        i = 1;
        return arr[i];  // Should return 200
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['./ccc', f.name, '-o', 'test_array_var.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_array_var.ll', 'r') as out:
            content = out.read()
            assert 'alloca [3 x i32]' in content
            assert 'load i32, i32* %i' in content  # Load variable i for index
            
        os.unlink(f.name)
        os.unlink('test_array_var.ll')

def test_array_assignment():
    """Test array element assignment in expressions."""
    code = """
    int main() {
        int arr[2];
        int x;
        
        x = 42;
        arr[0] = x;
        arr[1] = arr[0] + 8;
        
        return arr[1];  // Should return 50
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['./ccc', f.name, '-o', 'test_array_assign.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_array_assign.ll', 'r') as out:
            content = out.read()
            assert 'alloca [2 x i32]' in content
            
        os.unlink(f.name)
        os.unlink('test_array_assign.ll')

if __name__ == '__main__':
    test_int_array()
    print("✓ Integer array test passed")
    
    test_char_array()
    print("✓ Character array test passed")
    
    test_array_with_variables()
    print("✓ Array with variable index test passed")
    
    test_array_assignment()
    print("✓ Array assignment test passed")
    
    print("\nAll array tests passed!")