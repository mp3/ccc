#!/usr/bin/env python3
"""Test string literal support."""

import subprocess
import os
import tempfile

def test_basic_string():
    """Test basic string literal assignment."""
    code = """
    int main() {
        char* str;
        str = "Hello, World!";
        return 0;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_string.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_string.ll', 'r') as out:
            content = out.read()
            assert '@.str.' in content
            assert 'Hello, World!' in content
            assert 'getelementptr' in content
            
        os.unlink(f.name)
        os.unlink('test_string.ll')

def test_multiple_strings():
    """Test multiple string literals."""
    code = """
    int main() {
        char* s1;
        char* s2;
        char* s3;
        
        s1 = "First";
        s2 = "Second";
        s3 = "Third";
        
        return 0;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_multi_string.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_multi_string.ll', 'r') as out:
            content = out.read()
            assert 'First' in content
            assert 'Second' in content
            assert 'Third' in content
            assert '@.str.0' in content
            assert '@.str.1' in content
            assert '@.str.2' in content
            
        os.unlink(f.name)
        os.unlink('test_multi_string.ll')

def test_string_with_escapes():
    """Test string literals with escape sequences."""
    code = """
    int main() {
        char* s1;
        char* s2;
        
        s1 = "Hello\\nWorld";
        s2 = "Tab\\tHere";
        
        return 0;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_escape_string.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_escape_string.ll', 'r') as out:
            content = out.read()
            assert '\\0A' in content  # \n
            assert '\\09' in content  # \t
            
        os.unlink(f.name)
        os.unlink('test_escape_string.ll')

def test_string_dereference():
    """Test dereferencing string literals."""
    code = """
    int main() {
        char* str;
        char ch;
        
        str = "ABC";
        ch = *str;  // Should be 'A' = 65
        
        return ch;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_string_deref.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_string_deref.ll', 'r') as out:
            content = out.read()
            assert 'load i8, i8*' in content
            assert 'sext i8' in content  # char to int conversion
            
        os.unlink(f.name)
        os.unlink('test_string_deref.ll')

def test_empty_string():
    """Test empty string literal."""
    code = """
    int main() {
        char* empty;
        empty = "";
        return 0;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['../ccc', f.name, '-o', 'test_empty_string.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_empty_string.ll', 'r') as out:
            content = out.read()
            assert '[1 x i8]' in content  # empty string + null terminator
            
        os.unlink(f.name)
        os.unlink('test_empty_string.ll')

if __name__ == '__main__':
    test_basic_string()
    print("✓ Basic string test passed")
    
    test_multiple_strings()
    print("✓ Multiple strings test passed")
    
    test_string_with_escapes()
    print("✓ String escape sequences test passed")
    
    test_string_dereference()
    print("✓ String dereference test passed")
    
    test_empty_string()
    print("✓ Empty string test passed")
    
    print("\nAll string literal tests passed!")