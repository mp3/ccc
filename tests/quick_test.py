#!/usr/bin/env python3
"""
Quick test runner for development.
Runs a subset of tests to quickly verify basic functionality.
"""

import os
import sys
import tempfile
import subprocess
from pathlib import Path

def run_compiler(input_file, output_file, compiler_path="./ccc"):
    """Run the compiler."""
    result = subprocess.run([compiler_path, input_file, "-o", output_file], 
                          capture_output=True, text=True, timeout=10)
    return result

def test_basic_compilation():
    """Test basic compilation works."""
    code = """
    int main() {
        return 42;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        output_file = f.name.replace('.c', '.ll')
        result = run_compiler(f.name, output_file)
        
        success = result.returncode == 0 and os.path.exists(output_file)
        
        # Cleanup
        os.unlink(f.name)
        if os.path.exists(output_file):
            os.unlink(output_file)
        
        return success, result.stderr if not success else ""

def test_function_calls():
    """Test function calls work."""
    code = """
    int add(int a, int b) {
        return a + b;
    }
    
    int main() {
        return add(5, 3);
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        output_file = f.name.replace('.c', '.ll')
        result = run_compiler(f.name, output_file)
        
        success = result.returncode == 0 and os.path.exists(output_file)
        
        # Cleanup
        os.unlink(f.name)
        if os.path.exists(output_file):
            os.unlink(output_file)
        
        return success, result.stderr if not success else ""

def test_arrays():
    """Test array functionality."""
    code = """
    int main() {
        int arr[3];
        arr[0] = 10;
        arr[1] = 20;
        arr[2] = 30;
        return arr[1];
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        output_file = f.name.replace('.c', '.ll')
        result = run_compiler(f.name, output_file)
        
        success = result.returncode == 0 and os.path.exists(output_file)
        
        # Cleanup
        os.unlink(f.name)
        if os.path.exists(output_file):
            os.unlink(output_file)
        
        return success, result.stderr if not success else ""

def test_pointers():
    """Test pointer functionality."""
    code = """
    int main() {
        int x = 42;
        int *p = &x;
        return *p;
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        output_file = f.name.replace('.c', '.ll')
        result = run_compiler(f.name, output_file)
        
        success = result.returncode == 0 and os.path.exists(output_file)
        
        # Cleanup
        os.unlink(f.name)
        if os.path.exists(output_file):
            os.unlink(output_file)
        
        return success, result.stderr if not success else ""

def test_control_flow():
    """Test control flow."""
    code = """
    int main() {
        int x = 5;
        if (x > 0) {
            return 1;
        } else {
            return 0;
        }
    }
    """
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        output_file = f.name.replace('.c', '.ll')
        result = run_compiler(f.name, output_file)
        
        success = result.returncode == 0 and os.path.exists(output_file)
        
        # Cleanup
        os.unlink(f.name)
        if os.path.exists(output_file):
            os.unlink(output_file)
        
        return success, result.stderr if not success else ""

def main():
    """Run quick tests."""
    # Change to project root if running from tests directory
    if os.path.basename(os.getcwd()) == "tests":
        os.chdir("..")
    
    print("CCC Compiler Quick Test Suite")
    print("=" * 40)
    
    tests = [
        ("Basic compilation", test_basic_compilation),
        ("Function calls", test_function_calls),
        ("Arrays", test_arrays),
        ("Pointers", test_pointers),
        ("Control flow", test_control_flow),
    ]
    
    passed = 0
    failed = 0
    
    for test_name, test_func in tests:
        try:
            success, error = test_func()
            if success:
                print(f"✓ {test_name}")
                passed += 1
            else:
                print(f"✗ {test_name}")
                if error:
                    print(f"  Error: {error}")
                failed += 1
        except Exception as e:
            print(f"E {test_name}: {e}")
            failed += 1
    
    print("-" * 40)
    print(f"Passed: {passed}, Failed: {failed}")
    
    if failed == 0:
        print("✅ All quick tests passed!")
        return 0
    else:
        print("❌ Some tests failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())