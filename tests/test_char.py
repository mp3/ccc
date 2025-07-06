#!/usr/bin/env python3
"""Test character type support."""

import subprocess
import os
import tempfile

def test_char_literals():
    """Test that character literals are parsed correctly."""
    code = """
    int main() {
        char c1 = 'A';
        char c2 = 'z';
        char c3 = '0';
        char c4 = ' ';
        return 0;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['./ccc', f.name, '-o', 'test_char.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_char.ll', 'r') as out:
            content = out.read()
            assert 'add i8 0, 65' in content  # 'A' = 65
            assert 'add i8 0, 122' in content # 'z' = 122
            assert 'add i8 0, 48' in content  # '0' = 48
            assert 'add i8 0, 32' in content  # ' ' = 32
            
        os.unlink(f.name)
        os.unlink('test_char.ll')

def test_char_escape_sequences():
    """Test that escape sequences work correctly."""
    code = """
    int main() {
        char c1 = '\\n';
        char c2 = '\\t';
        char c3 = '\\r';
        char c4 = '\\\\';
        char c5 = '\\'';
        return 0;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['./ccc', f.name, '-o', 'test_escape.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_escape.ll', 'r') as out:
            content = out.read()
            assert 'add i8 0, 10' in content  # '\n' = 10
            assert 'add i8 0, 9' in content   # '\t' = 9
            assert 'add i8 0, 13' in content  # '\r' = 13
            assert 'add i8 0, 92' in content  # '\\' = 92
            assert 'add i8 0, 39' in content  # '\'' = 39
            
        os.unlink(f.name)
        os.unlink('test_escape.ll')

def test_char_functions():
    """Test functions with char return types and parameters."""
    code = """
    char get_a() {
        return 'A';
    }
    
    int char_to_ascii(char c) {
        return c;
    }
    
    int main() {
        char letter = get_a();
        return 0;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        # Compile
        result = subprocess.run(['./ccc', f.name, '-o', 'test_char_func.ll'], 
                              capture_output=True, text=True)
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check output
        with open('test_char_func.ll', 'r') as out:
            content = out.read()
            assert 'define i8 @get_a()' in content
            assert 'define i32 @char_to_ascii(i8 %c.param)' in content
            assert 'ret i8' in content
            
        os.unlink(f.name)
        os.unlink('test_char_func.ll')

if __name__ == '__main__':
    test_char_literals()
    print("✓ Character literals test passed")
    
    test_char_escape_sequences()
    print("✓ Escape sequences test passed")
    
    test_char_functions()
    print("✓ Character functions test passed")
    
    print("\nAll character type tests passed!")