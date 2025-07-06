#!/usr/bin/env python3
"""Test pointer support."""

import subprocess
import os
import tempfile

def test_basic_pointer():
    """Test basic pointer operations."""
    code = """
    int main() {
        int x;
        int* p;
        
        x = 42;
        p = &x;
        
        return *p;  // Should return 42
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_pointer.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_pointer.ll', 'r') as out:
            content = out.read()
            assert 'alloca i32*' in content
            assert 'store i32* %x, i32** %p' in content
            assert 'load i32*, i32**' in content
            
        os.unlink(f.name)
        os.unlink('test_pointer.ll')

def test_pointer_to_array():
    """Test pointer to array element."""
    code = """
    int main() {
        int arr[3];
        int* p;
        
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        
        p = &arr[1];
        
        return *p;  // Should return 20
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_array_ptr.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_array_ptr.ll', 'r') as out:
            content = out.read()
            assert 'getelementptr [3 x i32]' in content
            assert 'store i32* %' in content
            
        os.unlink(f.name)
        os.unlink('test_array_ptr.ll')

def test_char_pointer():
    """Test character pointer."""
    code = """
    int main() {
        char c;
        char* p;
        
        c = 'A';
        p = &c;
        
        return *p;  // Should return 65
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_char_ptr.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_char_ptr.ll', 'r') as out:
            content = out.read()
            assert 'alloca i8*' in content
            assert 'store i8* %c, i8** %p' in content
            
        os.unlink(f.name)
        os.unlink('test_char_ptr.ll')

def test_pointer_types():
    """Test pointer type declarations."""
    code = """
    int main() {
        int* p1;
        char* p2;
        int** pp;
        
        return 0;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_ptr_types.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_ptr_types.ll', 'r') as out:
            content = out.read()
            assert 'alloca i32*' in content
            assert 'alloca i8*' in content
            assert 'alloca i32**' in content
            
        os.unlink(f.name)
        os.unlink('test_ptr_types.ll')

def test_pointer_assignment():
    """Test pointer assignment through dereference."""
    code = """
    int main() {
        int x;
        int y;
        int* p;
        
        x = 10;
        p = &x;
        *p = 20;  // x should now be 20
        
        y = x;
        return y;  // Should return 20
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_ptr_assign.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output contains proper stores through pointer
        with open('test_ptr_assign.ll', 'r') as out:
            content = out.read()
            assert 'store i32' in content
            
        os.unlink(f.name)
        os.unlink('test_ptr_assign.ll')

if __name__ == '__main__':
    test_basic_pointer()
    print("✓ Basic pointer test passed")
    
    test_pointer_to_array()
    print("✓ Pointer to array test passed")
    
    test_char_pointer()
    print("✓ Character pointer test passed")
    
    test_pointer_types()
    print("✓ Pointer types test passed")
    
    test_pointer_assignment()
    print("✓ Pointer assignment test passed")
    
    print("\nAll pointer tests passed!")