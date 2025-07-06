import pytest
import subprocess
import os
import tempfile

def create_temp_c_file(content):
    """Create a temporary C file with the given content."""
    fd, path = tempfile.mkstemp(suffix='.c')
    with os.fdopen(fd, 'w') as f:
        f.write(content)
    return path

def compile_file(input_file, output_file):
    """Run the ccc compiler on the input file."""
    result = subprocess.run(['./ccc', input_file, '-o', output_file], 
                          capture_output=True, text=True)
    return result

class TestLexer:
    def test_integer_literal(self):
        """Test that integer literals are properly lexed."""
        c_file = create_temp_c_file("int main() { return 42; }")
        output_file = tempfile.mktemp(suffix='.ll')
        
        result = compile_file(c_file, output_file)
        
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        assert os.path.exists(output_file), "Output file was not created"
        
        with open(output_file, 'r') as f:
            content = f.read()
            assert 'define i32 @main()' in content
            assert '42' in content
        
        os.unlink(c_file)
        os.unlink(output_file)
    
    def test_arithmetic_operators(self):
        """Test that arithmetic operators are properly lexed."""
        c_file = create_temp_c_file("int main() { return 1 + 2 * 3; }")
        output_file = tempfile.mktemp(suffix='.ll')
        
        result = compile_file(c_file, output_file)
        
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        assert os.path.exists(output_file), "Output file was not created"
        
        with open(output_file, 'r') as f:
            content = f.read()
            assert 'add' in content
            assert 'mul' in content
        
        os.unlink(c_file)
        os.unlink(output_file)
    
    def test_parentheses(self):
        """Test that parentheses are properly handled."""
        c_file = create_temp_c_file("int main() { return (1 + 2) * 3; }")
        output_file = tempfile.mktemp(suffix='.ll')
        
        result = compile_file(c_file, output_file)
        
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        assert os.path.exists(output_file), "Output file was not created"
        
        os.unlink(c_file)
        os.unlink(output_file)
    
    def test_variables(self):
        """Test variable declarations and usage."""
        c_file = create_temp_c_file("int main() { int x = 5; int y = 10; return x + y; }")
        output_file = tempfile.mktemp(suffix='.ll')
        
        result = compile_file(c_file, output_file)
        
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        assert os.path.exists(output_file), "Output file was not created"
        
        with open(output_file, 'r') as f:
            content = f.read()
            assert 'alloca i32' in content
            assert 'store i32' in content
            assert 'load i32' in content
        
        os.unlink(c_file)
        os.unlink(output_file)
    
    def test_assignments(self):
        """Test variable assignments."""
        c_file = create_temp_c_file("int main() { int x; x = 42; return x; }")
        output_file = tempfile.mktemp(suffix='.ll')
        
        result = compile_file(c_file, output_file)
        
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        assert os.path.exists(output_file), "Output file was not created"
        
        os.unlink(c_file)
        os.unlink(output_file)