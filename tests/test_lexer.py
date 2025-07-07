#!/usr/bin/env python3
"""Test lexer functionality."""

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

def test_integer_literal():
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

def test_arithmetic_operators():
    """Test that arithmetic operators are properly lexed."""
    # Use variables to prevent constant folding optimization
    c_file = create_temp_c_file("int main() { int a = 1; int b = 2; int c = 3; return a + b * c; }")
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

def test_parentheses():
    """Test that parentheses are properly handled."""
    c_file = create_temp_c_file("int main() { return (1 + 2) * 3; }")
    output_file = tempfile.mktemp(suffix='.ll')
    
    result = compile_file(c_file, output_file)
    
    assert result.returncode == 0, f"Compilation failed: {result.stderr}"
    assert os.path.exists(output_file), "Output file was not created"
    
    os.unlink(c_file)
    os.unlink(output_file)

def test_variables():
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

def test_assignments():
    """Test variable assignments."""
    c_file = create_temp_c_file("int main() { int x; x = 42; return x; }")
    output_file = tempfile.mktemp(suffix='.ll')
    
    result = compile_file(c_file, output_file)
    
    assert result.returncode == 0, f"Compilation failed: {result.stderr}"
    assert os.path.exists(output_file), "Output file was not created"
    
    os.unlink(c_file)
    os.unlink(output_file)

def test_if_statement():
    """Test if statements."""
    c_file = create_temp_c_file("int main() { int x = 5; if (x > 0) { return 1; } else { return 0; } }")
    output_file = tempfile.mktemp(suffix='.ll')
    
    result = compile_file(c_file, output_file)
    
    assert result.returncode == 0, f"Compilation failed: {result.stderr}"
    assert os.path.exists(output_file), "Output file was not created"
    
    with open(output_file, 'r') as f:
        content = f.read()
        assert 'icmp' in content
        assert 'br i1' in content
    
    os.unlink(c_file)
    os.unlink(output_file)

def test_while_loop():
    """Test while loops."""
    c_file = create_temp_c_file("int main() { int i = 0; int sum = 0; while (i < 10) { sum = sum + i; i = i + 1; } return sum; }")
    output_file = tempfile.mktemp(suffix='.ll')
    
    result = compile_file(c_file, output_file)
    
    assert result.returncode == 0, f"Compilation failed: {result.stderr}"
    assert os.path.exists(output_file), "Output file was not created"
    
    os.unlink(c_file)
    os.unlink(output_file)

def test_function_definition():
    """Test function definitions with parameters."""
    c_file = create_temp_c_file("int add(int a, int b) { return a + b; } int main() { return 0; }")
    output_file = tempfile.mktemp(suffix='.ll')
    
    result = compile_file(c_file, output_file)
    
    assert result.returncode == 0, f"Compilation failed: {result.stderr}"
    assert os.path.exists(output_file), "Output file was not created"
    
    with open(output_file, 'r') as f:
        content = f.read()
        assert 'define i32 @add(i32' in content
        assert 'define i32 @main()' in content
    
    os.unlink(c_file)
    os.unlink(output_file)

def test_function_calls():
    """Test function calls."""
    c_file = create_temp_c_file("int square(int x) { return x * x; } int main() { return square(5); }")
    output_file = tempfile.mktemp(suffix='.ll')
    
    result = compile_file(c_file, output_file)
    
    assert result.returncode == 0, f"Compilation failed: {result.stderr}"
    assert os.path.exists(output_file), "Output file was not created"
    
    with open(output_file, 'r') as f:
        content = f.read()
        assert 'call i32 @square' in content
    
    os.unlink(c_file)
    os.unlink(output_file)

def main():
    """Run all lexer tests."""
    tests = [
        test_integer_literal,
        test_arithmetic_operators,
        test_parentheses,
        test_variables,
        test_assignments,
        test_if_statement,
        test_while_loop,
        test_function_definition,
        test_function_calls,
    ]
    
    passed = 0
    failed = 0
    
    for test_func in tests:
        try:
            test_func()
            print(f"✓ {test_func.__name__}")
            passed += 1
        except Exception as e:
            print(f"✗ {test_func.__name__}: {e}")
            failed += 1
    
    print(f"\nLexer tests: {passed} passed, {failed} failed")
    return failed == 0

if __name__ == '__main__':
    success = main()
    exit(0 if success else 1)