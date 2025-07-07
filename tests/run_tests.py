#!/usr/bin/env python3
"""
Unified test runner for the CCC compiler.

This script runs all tests and provides comprehensive reporting.
"""

import os
import sys
import subprocess
import tempfile
import glob
import time
import argparse
from pathlib import Path
from dataclasses import dataclass
from typing import List, Optional, Dict, Any
from enum import Enum
import json

class TestStatus(Enum):
    PASSED = "PASSED"
    FAILED = "FAILED"
    SKIPPED = "SKIPPED"
    ERROR = "ERROR"

@dataclass
class TestResult:
    name: str
    status: TestStatus
    duration: float
    message: str = ""
    output: str = ""
    error: str = ""

class TestCategory:
    def __init__(self, name: str, description: str):
        self.name = name
        self.description = description
        self.tests = []
        self.results = []

class TestRunner:
    def __init__(self, compiler_path: str = "./ccc", verbose: bool = False):
        self.compiler_path = compiler_path
        self.verbose = verbose
        self.categories = {}
        self.total_tests = 0
        self.passed_tests = 0
        self.failed_tests = 0
        self.skipped_tests = 0
        self.error_tests = 0
        self.start_time = None
        self.end_time = None
        
        # Verify compiler exists
        if not os.path.exists(self.compiler_path):
            raise FileNotFoundError(f"Compiler not found at {self.compiler_path}")
    
    def add_category(self, name: str, description: str):
        """Add a test category."""
        self.categories[name] = TestCategory(name, description)
    
    def run_compiler(self, input_file: str, output_file: str, 
                    optimization_level: str = "-O1") -> subprocess.CompletedProcess:
        """Run the compiler on an input file."""
        cmd = [self.compiler_path, input_file, optimization_level, "-o", output_file]
        # Note: verbose flag not implemented in compiler yet
        
        return subprocess.run(cmd, capture_output=True, text=True, timeout=30)
    
    def execute_llvm_ir(self, ll_file: str, expected_exit_code: int = 0) -> tuple:
        """Execute LLVM IR and return (exit_code, stdout, stderr)."""
        obj_file = ll_file.replace('.ll', '.o')
        exe_file = ll_file.replace('.ll', '')
        
        try:
            # Compile to object file
            result = subprocess.run(['llc', ll_file, '-filetype=obj', '-o', obj_file],
                                   capture_output=True, text=True, timeout=10)
            if result.returncode != 0:
                return None, "", f"llc failed: {result.stderr}"
            
            # Link to executable
            result = subprocess.run(['clang', obj_file, '-o', exe_file],
                                   capture_output=True, text=True, timeout=10)
            if result.returncode != 0:
                return None, "", f"clang failed: {result.stderr}"
            
            # Execute
            result = subprocess.run([exe_file], capture_output=True, text=True, timeout=5)
            
            # Cleanup
            for f in [obj_file, exe_file]:
                if os.path.exists(f):
                    os.unlink(f)
            
            return result.returncode, result.stdout, result.stderr
            
        except subprocess.TimeoutExpired:
            return None, "", "Execution timeout"
        except Exception as e:
            return None, "", f"Execution error: {str(e)}"
    
    def run_compilation_test(self, test_file: str, category: str,
                           expected_success: bool = True,
                           expected_exit_code: int = 0,
                           check_ir: bool = True) -> TestResult:
        """Run a compilation test."""
        test_name = os.path.basename(test_file)
        start_time = time.time()
        
        try:
            # Create temporary output file
            with tempfile.NamedTemporaryFile(suffix='.ll', delete=False) as tmp:
                output_file = tmp.name
            
            # Compile
            result = self.run_compiler(test_file, output_file)
            
            if expected_success and result.returncode != 0:
                duration = time.time() - start_time
                return TestResult(
                    name=test_name,
                    status=TestStatus.FAILED,
                    duration=duration,
                    message=f"Compilation failed unexpectedly",
                    error=result.stderr
                )
            
            if not expected_success and result.returncode == 0:
                duration = time.time() - start_time
                return TestResult(
                    name=test_name,
                    status=TestStatus.FAILED,
                    duration=duration,
                    message=f"Compilation succeeded but was expected to fail",
                    output=result.stdout
                )
            
            # If compilation was expected to fail and did fail, that's success
            if not expected_success and result.returncode != 0:
                duration = time.time() - start_time
                os.unlink(output_file)
                return TestResult(
                    name=test_name,
                    status=TestStatus.PASSED,
                    duration=duration,
                    message="Compilation failed as expected"
                )
            
            # Check IR if requested
            if check_ir and os.path.exists(output_file):
                with open(output_file, 'r') as f:
                    ir_content = f.read()
                    if not ir_content.strip():
                        duration = time.time() - start_time
                        os.unlink(output_file)
                        return TestResult(
                            name=test_name,
                            status=TestStatus.FAILED,
                            duration=duration,
                            message="Generated IR is empty"
                        )
            
            # Execute if possible
            if expected_exit_code is not None and os.path.exists(output_file):
                exit_code, stdout, stderr = self.execute_llvm_ir(output_file, expected_exit_code)
                if exit_code != expected_exit_code:
                    duration = time.time() - start_time
                    os.unlink(output_file)
                    return TestResult(
                        name=test_name,
                        status=TestStatus.FAILED,
                        duration=duration,
                        message=f"Expected exit code {expected_exit_code}, got {exit_code}",
                        output=stdout,
                        error=stderr
                    )
            
            # Clean up
            if os.path.exists(output_file):
                os.unlink(output_file)
            
            duration = time.time() - start_time
            return TestResult(
                name=test_name,
                status=TestStatus.PASSED,
                duration=duration,
                message="Test passed"
            )
            
        except Exception as e:
            duration = time.time() - start_time
            return TestResult(
                name=test_name,
                status=TestStatus.ERROR,
                duration=duration,
                message=f"Test error: {str(e)}"
            )
    
    def run_python_test(self, test_file: str, category: str) -> TestResult:
        """Run a Python test file."""
        test_name = os.path.basename(test_file)
        start_time = time.time()
        
        try:
            # Run from project root to ensure relative paths work
            old_cwd = os.getcwd()
            
            # If we're in tests directory, go up to project root
            if os.path.basename(old_cwd) == "tests":
                os.chdir("..")
            
            result = subprocess.run([sys.executable, test_file],
                                   capture_output=True, text=True, timeout=60)
            
            os.chdir(old_cwd)
            
            duration = time.time() - start_time
            
            if result.returncode == 0:
                return TestResult(
                    name=test_name,
                    status=TestStatus.PASSED,
                    duration=duration,
                    message="Python test passed",
                    output=result.stdout
                )
            else:
                return TestResult(
                    name=test_name,
                    status=TestStatus.FAILED,
                    duration=duration,
                    message="Python test failed",
                    error=result.stderr,
                    output=result.stdout
                )
                
        except subprocess.TimeoutExpired:
            duration = time.time() - start_time
            return TestResult(
                name=test_name,
                status=TestStatus.ERROR,
                duration=duration,
                message="Test timeout"
            )
        except Exception as e:
            duration = time.time() - start_time
            return TestResult(
                name=test_name,
                status=TestStatus.ERROR,
                duration=duration,
                message=f"Test error: {str(e)}"
            )
    
    def discover_tests(self, test_dir: str = "tests"):
        """Discover all test files."""
        test_path = Path(test_dir)
        
        # Add categories
        self.add_category("lexer", "Lexical analysis tests")
        self.add_category("parser", "Parsing tests")
        self.add_category("codegen", "Code generation tests")
        self.add_category("features", "Language feature tests")
        self.add_category("integration", "Integration tests")
        self.add_category("optimization", "Optimization tests")
        self.add_category("errors", "Error handling tests")
        
        # Discover Python test files
        python_tests = list(test_path.glob("test_*.py"))
        for test_file in python_tests:
            name = test_file.stem
            if "lexer" in name:
                category = "lexer"
            elif "array" in name or "pointer" in name or "string" in name or "char" in name:
                category = "features"
            elif "function" in name:
                category = "features"
            else:
                category = "integration"
            
            self.categories[category].tests.append(("python", str(test_file)))
        
        # Discover C test files
        c_tests = list(test_path.glob("test_*.c"))
        for test_file in c_tests:
            name = test_file.stem
            
            # Determine category and expected behavior
            if "error" in name:
                category = "errors"
                expected_success = False
            elif "parse" in name or "lex" in name:
                category = "parser"
                expected_success = True
            elif "optimize" in name or "const" in name:
                category = "optimization"
                expected_success = True
            else:
                category = "codegen"
                expected_success = True
            
            # Try to infer expected exit code from file
            expected_exit_code = self.get_expected_exit_code(str(test_file))
            
            self.categories[category].tests.append(
                ("c", str(test_file), expected_success, expected_exit_code)
            )
    
    def get_expected_exit_code(self, test_file: str) -> Optional[int]:
        """Try to determine expected exit code from test file."""
        try:
            with open(test_file, 'r') as f:
                content = f.read()
                # Look for return statements in main
                if "return 0;" in content:
                    return 0
                elif "return " in content:
                    # Try to extract simple return values
                    import re
                    matches = re.findall(r'return\s+(\d+);', content)
                    if matches:
                        return int(matches[-1])  # Use last return value
                return 0  # Default
        except:
            return 0
    
    def run_all_tests(self) -> Dict[str, List[TestResult]]:
        """Run all discovered tests."""
        self.start_time = time.time()
        results = {}
        
        print(f"Running CCC Compiler Test Suite")
        print(f"Compiler: {self.compiler_path}")
        print(f"Verbose: {self.verbose}")
        print("=" * 60)
        
        for category_name, category in self.categories.items():
            if not category.tests:
                continue
                
            print(f"\n{category_name.upper()}: {category.description}")
            print("-" * 40)
            
            category_results = []
            
            for test_info in category.tests:
                test_type = test_info[0]
                test_file = test_info[1]
                
                if test_type == "python":
                    result = self.run_python_test(test_file, category_name)
                elif test_type == "c":
                    expected_success = test_info[2] if len(test_info) > 2 else True
                    expected_exit_code = test_info[3] if len(test_info) > 3 else 0
                    result = self.run_compilation_test(
                        test_file, category_name, expected_success, expected_exit_code
                    )
                
                category_results.append(result)
                
                # Update counters
                self.total_tests += 1
                if result.status == TestStatus.PASSED:
                    self.passed_tests += 1
                    status_char = "✓"
                    color = "\033[92m"  # Green
                elif result.status == TestStatus.FAILED:
                    self.failed_tests += 1
                    status_char = "✗"
                    color = "\033[91m"  # Red
                elif result.status == TestStatus.SKIPPED:
                    self.skipped_tests += 1
                    status_char = "⚠"
                    color = "\033[93m"  # Yellow
                else:  # ERROR
                    self.error_tests += 1
                    status_char = "E"
                    color = "\033[91m"  # Red
                
                reset_color = "\033[0m"
                duration_str = f"{result.duration:.3f}s"
                
                print(f"{color}{status_char}{reset_color} {result.name:<30} {duration_str:>8}")
                
                if self.verbose and result.message:
                    print(f"    {result.message}")
                if result.status in [TestStatus.FAILED, TestStatus.ERROR]:
                    if result.error:
                        print(f"    Error: {result.error}")
                    if result.output and self.verbose:
                        print(f"    Output: {result.output}")
            
            results[category_name] = category_results
        
        self.end_time = time.time()
        return results
    
    def print_summary(self):
        """Print test summary."""
        total_duration = self.end_time - self.start_time
        
        print("\n" + "=" * 60)
        print("TEST SUMMARY")
        print("=" * 60)
        
        print(f"Total tests: {self.total_tests}")
        print(f"Passed:      {self.passed_tests} ({self.passed_tests/self.total_tests*100:.1f}%)")
        print(f"Failed:      {self.failed_tests} ({self.failed_tests/self.total_tests*100:.1f}%)")
        print(f"Errors:      {self.error_tests} ({self.error_tests/self.total_tests*100:.1f}%)")
        print(f"Skipped:     {self.skipped_tests} ({self.skipped_tests/self.total_tests*100:.1f}%)")
        print(f"Duration:    {total_duration:.3f}s")
        
        if self.failed_tests > 0 or self.error_tests > 0:
            print("\n❌ SOME TESTS FAILED")
            return False
        else:
            print("\n✅ ALL TESTS PASSED")
            return True
    
    def save_results(self, results: Dict[str, List[TestResult]], output_file: str):
        """Save test results to JSON file."""
        json_results = {
            "summary": {
                "total_tests": self.total_tests,
                "passed": self.passed_tests,
                "failed": self.failed_tests,
                "errors": self.error_tests,
                "skipped": self.skipped_tests,
                "duration": self.end_time - self.start_time,
                "timestamp": time.time()
            },
            "categories": {}
        }
        
        for category_name, category_results in results.items():
            json_results["categories"][category_name] = [
                {
                    "name": result.name,
                    "status": result.status.value,
                    "duration": result.duration,
                    "message": result.message,
                    "output": result.output,
                    "error": result.error
                }
                for result in category_results
            ]
        
        with open(output_file, 'w') as f:
            json.dump(json_results, f, indent=2)

def main():
    parser = argparse.ArgumentParser(description="CCC Compiler Test Runner")
    parser.add_argument("--compiler", "-c", default="./ccc", 
                       help="Path to compiler executable")
    parser.add_argument("--verbose", "-v", action="store_true",
                       help="Verbose output")
    parser.add_argument("--test-dir", "-t", default="tests",
                       help="Test directory")
    parser.add_argument("--output", "-o", 
                       help="Save results to JSON file")
    parser.add_argument("--category", 
                       help="Run only tests in specific category")
    
    args = parser.parse_args()
    
    # Change to project root if running from tests directory
    if os.path.basename(os.getcwd()) == "tests":
        os.chdir("..")
    
    try:
        runner = TestRunner(args.compiler, args.verbose)
        runner.discover_tests(args.test_dir)
        
        # Filter by category if specified
        if args.category:
            if args.category not in runner.categories:
                print(f"Unknown category: {args.category}")
                print(f"Available categories: {', '.join(runner.categories.keys())}")
                return 1
            
            # Keep only the specified category
            filtered_categories = {args.category: runner.categories[args.category]}
            runner.categories = filtered_categories
        
        results = runner.run_all_tests()
        success = runner.print_summary()
        
        if args.output:
            runner.save_results(results, args.output)
            print(f"\nResults saved to: {args.output}")
        
        return 0 if success else 1
        
    except KeyboardInterrupt:
        print("\n\nTest run interrupted by user")
        return 1
    except Exception as e:
        print(f"Test runner error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())