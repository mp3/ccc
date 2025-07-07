# CCC Compiler Test Suite

This directory contains the comprehensive test suite for the CCC compiler.

## Overview

The test suite includes:
- **Unified test runner** (`run_tests.py`) - Comprehensive test execution
- **Quick test runner** (`quick_test.py`) - Fast subset of tests for development
- **Individual test files** - Specific functionality tests
- **Test configuration** - Settings and expectations

## Test Categories

### Lexer Tests
- **Purpose**: Verify tokenization works correctly
- **Files**: `test_lexer.py`, `test_lexer_tokens.c`
- **Coverage**: Keywords, identifiers, literals, operators, delimiters

### Parser Tests
- **Purpose**: Verify parsing and AST construction
- **Files**: `test_*_parse.c`, `test_*_parse_only.c`
- **Coverage**: Declarations, statements, expressions, syntax errors

### Code Generation Tests
- **Purpose**: Verify LLVM IR generation
- **Files**: Most `test_*.c` files
- **Coverage**: All language constructs, proper IR output

### Feature Tests
- **Purpose**: Test specific language features
- **Files**: `test_arrays.py`, `test_pointers.py`, `test_strings.py`, etc.
- **Coverage**: Arrays, pointers, strings, functions, structures

### Integration Tests
- **Purpose**: Test complete programs
- **Files**: Complex test cases in `test_*.c`
- **Coverage**: Multi-feature programs, realistic code

### Optimization Tests
- **Purpose**: Verify optimization passes work
- **Files**: `test_*_optimize.c`, `test_const_*.c`
- **Coverage**: Constant folding, dead code elimination

### Error Tests
- **Purpose**: Verify error handling and reporting
- **Files**: `test_*_error.c`
- **Coverage**: Syntax errors, semantic errors, type errors

## Running Tests

### Full Test Suite
```bash
# Run all tests
make test

# Run with verbose output
make test-verbose

# Run specific category
make test-category CATEGORY=lexer

# Run and save results to JSON
python3 tests/run_tests.py --output results.json
```

### Quick Tests for Development
```bash
# Run quick subset of tests
make test-quick

# Or directly
python3 tests/quick_test.py
```

### Individual Test Categories
```bash
# Run only lexer tests
python3 tests/run_tests.py --category lexer

# Run only feature tests
python3 tests/run_tests.py --category features

# Run only error tests
python3 tests/run_tests.py --category errors
```

### Legacy pytest Support
```bash
# Run with pytest (legacy)
make test-pytest
```

## Test Structure

### C Test Files
C test files are automatically categorized and executed:

```c
// test_example.c
int main() {
    // Test code
    return expected_exit_code;
}
```

**Naming Conventions**:
- `test_feature_*.c` - Feature-specific tests
- `test_*_error.c` - Error handling tests  
- `test_*_parse.c` - Parsing tests
- `test_*_optimize.c` - Optimization tests

### Python Test Files
Python test files provide more complex test logic:

```python
# test_example.py
def test_functionality():
    # Test implementation
    assert condition
    
if __name__ == '__main__':
    test_functionality()
    print("✓ Test passed")
```

## Test Configuration

### Expected Behaviors
The test runner automatically determines expected behavior:

- **Compilation Success**: Most tests expect successful compilation
- **Error Tests**: Files with "error" in name expect compilation failure
- **Exit Codes**: Extracted from `return` statements in `main()`
- **Optimization**: Tests with "optimize" run with different optimization levels

### Custom Expectations
Override defaults in `test_config.json`:

```json
{
  "test_expectations": {
    "test_custom.c": {
      "should_fail": true,
      "expected_error": "type mismatch",
      "expected_exit_code": 1
    }
  }
}
```

## Test Output

### Console Output
Tests show real-time progress:
```
LEXER: Lexical analysis tests
----------------------------------------
✓ test_lexer.py                    0.045s
✓ test_lexer_tokens.c              0.012s

FEATURES: Language feature tests
----------------------------------------
✓ test_arrays.py                   0.123s
✗ test_pointers.py                 0.089s
    Error: Segmentation fault
```

### JSON Results
Save detailed results for analysis:
```bash
python3 tests/run_tests.py --output results.json
```

Results include:
- Test execution times
- Error messages and output
- Compilation and execution results
- Summary statistics

## Writing New Tests

### Simple C Test
```c
// test_new_feature.c
int main() {
    // Test your feature
    int result = test_function();
    return result;  // Expected exit code
}
```

### Complex Python Test
```python
#!/usr/bin/env python3
"""Test description."""

import subprocess
import tempfile
import os

def test_my_feature():
    """Test specific functionality."""
    code = '''
    int main() {
        // Test code
        return 0;
    }
    '''
    
    with tempfile.NamedTemporaryFile(mode='w', suffix='.c', delete=False) as f:
        f.write(code)
        f.flush()
        
        result = subprocess.run(['../ccc', f.name, '-o', 'test.ll'], 
                               capture_output=True, text=True)
        
        assert result.returncode == 0, f"Compilation failed: {result.stderr}"
        
        # Check LLVM IR
        with open('test.ll', 'r') as llvm_file:
            ir = llvm_file.read()
            assert 'expected_ir_pattern' in ir
        
        os.unlink(f.name)
        os.unlink('test.ll')

if __name__ == '__main__':
    test_my_feature()
    print("✓ Test passed")
```

### Error Test
```c
// test_error_case.c
int main() {
    // Code that should cause compilation error
    undefined_variable = 5;  // Should fail
    return 0;
}
```

## Test Coverage

### Current Coverage
- ✅ Basic compilation
- ✅ Arithmetic expressions
- ✅ Variables and assignments
- ✅ Control flow (if/else, loops)
- ✅ Functions and calls
- ✅ Arrays and pointers
- ✅ Strings and characters
- ✅ Structures and unions
- ✅ Enumerations
- ✅ Function pointers
- ✅ Static variables
- ✅ Type definitions
- ✅ Constant expressions
- ✅ Optimization passes
- ✅ Error handling

### Missing Coverage
- ⚠️ Variadic function runtime behavior
- ⚠️ Complex pointer arithmetic edge cases
- ⚠️ Nested structure/union combinations
- ⚠️ Large program stress tests
- ⚠️ Memory usage profiling
- ⚠️ Cross-platform compatibility

## Debugging Failed Tests

### Verbose Output
```bash
python3 tests/run_tests.py --verbose
```

### Individual Test Debugging
```bash
# Run single test manually
./ccc tests/test_failing.c -v -o debug.ll

# Check generated IR
cat debug.ll

# Try to execute
llc debug.ll -filetype=obj -o debug.o
clang debug.o -o debug
./debug
echo $?  # Check exit code
```

### Common Issues
1. **Compilation failures**: Check syntax and semantics
2. **Wrong exit codes**: Verify return statements
3. **LLVM IR issues**: Check type consistency
4. **Execution failures**: Debug with gdb or valgrind
5. **Timeout issues**: Reduce test complexity

## Continuous Integration

The test suite is designed for CI/CD integration:

```bash
# CI script example
#!/bin/bash
set -e

# Build compiler
make clean
make

# Run tests
python3 tests/run_tests.py --output ci_results.json

# Check results
if [ $? -eq 0 ]; then
    echo "✅ All tests passed"
else
    echo "❌ Tests failed"
    exit 1
fi
```

## Performance Benchmarks

### Test Execution Times
- **Quick tests**: ~1-2 seconds
- **Full test suite**: ~10-30 seconds
- **Individual categories**: ~2-5 seconds

### Optimization Impact
Run optimization comparison:
```bash
# Test different optimization levels
python3 tests/run_tests.py --category optimization
```

## Contributing Tests

### Guidelines
1. **Test one feature per file**
2. **Use descriptive names**
3. **Include edge cases**
4. **Add both positive and negative tests**
5. **Document expected behavior**
6. **Keep tests simple and focused**

### Review Checklist
- [ ] Test compiles and runs
- [ ] Expected behavior is clear
- [ ] Edge cases are covered
- [ ] Documentation is included
- [ ] Test is properly categorized
- [ ] Error cases are handled