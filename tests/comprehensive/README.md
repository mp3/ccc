# Comprehensive Test Suite

This directory contains comprehensive test cases for the CCC compiler that verify both compilation and execution with expected outputs.

## Test Organization

Each test file follows this format:
- Header comment with test description and expected output
- Comprehensive test of a specific language feature area
- Expected exit code that matches the test description

### Test Categories

| Test File | Feature Area | Expected Output | Description |
|-----------|--------------|-----------------|-------------|
| `test_basic_arithmetic.c` | Arithmetic Operations | 42 | Tests +, -, *, /, % with precedence |
| `test_control_flow.c` | Control Statements | 15 | Tests if/else, loops, switch |
| `test_arrays_pointers.c` | Arrays & Pointers | 30 | Tests array indexing, pointer arithmetic |
| `test_functions.c` | Function Features | 21 | Tests function calls, parameters, recursion |
| `test_structures.c` | Struct & Union | 25 | Tests struct/union declarations and access |
| `test_operators.c` | All Operators | 7 | Tests logical, bitwise, comparison operators |
| `test_function_pointers.c` | Function Pointers | 18 | Tests function pointer syntax and calls |
| `test_static_variables.c` | Static Storage | 10 | Tests static variable persistence |
| `test_enums_typedefs.c` | Type System | 8 | Tests enums and typedef declarations |
| `test_strings_chars.c` | Character Data | 72 | Tests char literals and string handling |
| `test_variadic_functions.c` | Variadic Functions | 3 | Tests ... parameter syntax |
| `test_const_qualifiers.c` | Const Qualifier | 50 | Tests const variable declarations |
| `test_type_casting.c` | Type Casting | 65 | Tests explicit type conversions |
| `test_sizeof_operator.c` | Sizeof Operator | 4 | Tests sizeof with types and expressions |

## Running Tests

### Run All Comprehensive Tests
```bash
cd tests/comprehensive
python3 run_comprehensive_tests.py
```

### Run Individual Test
```bash
cd tests/comprehensive
../../ccc test_basic_arithmetic.c -o test_basic_arithmetic.ll
llc test_basic_arithmetic.ll -filetype=obj -o test_basic_arithmetic.o
clang test_basic_arithmetic.o -o test_basic_arithmetic
./test_basic_arithmetic
echo $?  # Should print the expected output
```

### Integration with Main Test Runner
```bash
# From project root
python3 tests/run_tests.py --category comprehensive
```

## Test Design Principles

### Expected Output Verification
Each test is designed to return a specific exit code that can be verified:
- Tests exercise multiple aspects of a feature
- Return value is deterministic and meaningful
- Expected output is documented in the header comment

### Comprehensive Coverage
Tests are designed to cover:
- **Basic functionality** - Core feature usage
- **Edge cases** - Boundary conditions and special cases
- **Integration** - How features work together
- **Error conditions** - Invalid usage (in separate error tests)

### Self-Documenting
Each test includes:
- Clear description of what is being tested
- Expected output value
- Comments explaining the calculation
- Logical flow that's easy to follow

## Test Results

### Compilation Testing
- Verifies that code compiles without errors
- Checks that LLVM IR is generated correctly
- Ensures all language features parse properly

### Execution Testing
- Compiles LLVM IR to native code
- Executes the program and captures exit code
- Verifies exit code matches expected output

### Integration Testing
- Tests how different features work together
- Verifies complex expressions and statements
- Ensures proper operator precedence and evaluation

## Expected Test Results

When all tests pass, you should see:
```
CCC Compiler Comprehensive Test Suite
=====================================
Found 14 test files

Running test_basic_arithmetic.c... ✅ Expected 42, got 42
Running test_control_flow.c... ✅ Expected 15, got 15
Running test_arrays_pointers.c... ✅ Expected 30, got 30
Running test_functions.c... ✅ Expected 21, got 21
Running test_structures.c... ✅ Expected 25, got 25
Running test_operators.c... ✅ Expected 7, got 7
Running test_function_pointers.c... ✅ Expected 18, got 18
Running test_static_variables.c... ✅ Expected 10, got 10
Running test_enums_typedefs.c... ✅ Expected 8, got 8
Running test_strings_chars.c... ✅ Expected 72, got 72
Running test_variadic_functions.c... ✅ Expected 3, got 3
Running test_const_qualifiers.c... ✅ Expected 50, got 50
Running test_type_casting.c... ✅ Expected 65, got 65
Running test_sizeof_operator.c... ✅ Expected 4, got 4

==================================================
TEST SUMMARY
==================================================
Total tests: 14
Passed:      14 (100.0%)
Failed:      0 (0.0%)
Errors:      0 (0.0%)

✅ ALL TESTS PASSED!
```

## Adding New Tests

### Template for New Test
```c
/*
 * Test: [Feature Name]
 * Expected Output: [Exit Code]
 * Description: [What this test verifies]
 */

int main() {
    // Test implementation
    // ...
    
    // Return expected exit code
    return [expected_value];
}
```

### Guidelines
1. **Focus on one feature area** per test file
2. **Use meaningful expected outputs** that relate to the test
3. **Include comprehensive coverage** of the feature
4. **Document calculations** that lead to the expected output
5. **Test edge cases** and boundary conditions
6. **Keep tests deterministic** - same input always produces same output

### Naming Convention
- Use `test_[feature_area].c` format
- Use descriptive feature area names
- Keep names concise but clear

## Debugging Failed Tests

### Compilation Failures
```bash
../../ccc test_failing.c -o test_failing.ll
# Check error messages
```

### Wrong Exit Code
```bash
# Compile and run manually
../../ccc test_file.c -o test_file.ll
llc test_file.ll -filetype=obj -o test_file.o
clang test_file.o -o test_file
./test_file
echo $?  # Check actual exit code

# Debug with LLVM IR
cat test_file.ll  # Examine generated code
```

### Execution Errors
```bash
# Run with debugging
gdb ./test_file
# or
valgrind ./test_file
```

## Contributing

When adding new comprehensive tests:
1. Follow the existing format and style
2. Ensure expected output is documented
3. Test both compilation and execution
4. Add entry to the table in this README
5. Verify test works with the test runner