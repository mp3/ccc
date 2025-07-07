# CCC Compiler Development Guide

This guide is for developers who want to contribute to or extend the CCC compiler.

## Development Environment Setup

### Prerequisites

- C compiler (gcc or clang)
- LLVM toolchain (llc, clang)
- Python 3.x (for testing)
- Make
- Git

### Building from Source

```bash
# Clone the repository
git clone <repository-url>
cd ccc

# Build the compiler
make clean
make

# Run tests
make test
```

### Development Tools

Enable debug logging:
```bash
# Set log level in main.c or via environment
export CCC_LOG_LEVEL=DEBUG
```

## Project Structure

```
ccc/
├── src/
│   ├── main.c          # Entry point, argument parsing
│   ├── lexer.[ch]      # Tokenization
│   ├── parser.[ch]     # Parsing and AST construction
│   ├── codegen.[ch]    # LLVM IR generation
│   ├── symtab.[ch]     # Symbol table management
│   ├── optimizer.[ch]  # AST optimizations
│   └── logger.[ch]     # Logging utilities
├── tests/              # Test files and test runners
├── samples/            # Example programs
├── docs/               # Documentation
└── Makefile           # Build configuration
```

## Adding New Features

### 1. Adding a New Token Type

**Step 1**: Define the token in `src/lexer.h`:
```c
typedef enum {
    // ... existing tokens
    TOKEN_YOUR_NEW_TOKEN,
    // ...
} TokenType;
```

**Step 2**: Update `token_type_to_string()` in `src/lexer.c`:
```c
case TOKEN_YOUR_NEW_TOKEN: return "YOUR_NEW_TOKEN";
```

**Step 3**: Add lexing logic in `lexer_next_token()`:
```c
case 'x':  // or whatever character starts your token
    if (/* check for your token pattern */) {
        // consume characters
        return create_token(TOKEN_YOUR_NEW_TOKEN, value, line, column);
    }
    break;
```

### 2. Adding a New AST Node Type

**Step 1**: Define the node type in `src/parser.h`:
```c
typedef enum {
    // ... existing types
    AST_YOUR_NODE_TYPE,
    // ...
} ASTNodeType;
```

**Step 2**: Add node data to the union:
```c
union {
    // ... existing node data
    struct {
        // Your node's data fields
        ASTNode *child;
        int value;
    } your_node;
    // ...
} data;
```

**Step 3**: Update `ast_destroy()` to handle cleanup:
```c
case AST_YOUR_NODE_TYPE:
    if (node->data.your_node.child) {
        ast_destroy(node->data.your_node.child);
    }
    break;
```

### 3. Adding a New Statement Type

**Step 1**: Create parsing function in `src/parser.c`:
```c
static ASTNode *parse_your_statement(Parser *parser) {
    // Expect your keyword
    expect_token(parser, TOKEN_YOUR_KEYWORD);
    
    // Parse statement components
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_YOUR_STATEMENT;
    node->line = parser->current_token->line;
    node->column = parser->current_token->column;
    
    // Parse statement body
    // ...
    
    return node;
}
```

**Step 2**: Add to statement dispatcher:
```c
static ASTNode *parse_statement(Parser *parser) {
    switch (parser->current_token->type) {
        // ... existing cases
        case TOKEN_YOUR_KEYWORD:
            return parse_your_statement(parser);
        // ...
    }
}
```

**Step 3**: Add code generation in `src/codegen.c`:
```c
case AST_YOUR_STATEMENT:
    codegen_your_statement(gen, node);
    break;
```

### 4. Adding a New Operator

**Example: Adding the `**` (power) operator**

**Step 1**: Add token:
```c
TOKEN_POWER,  // ** operator
```

**Step 2**: Lex the operator:
```c
case '*':
    lexer_advance(lexer);
    if (lexer->current_char == '*') {
        lexer_advance(lexer);
        return create_token(TOKEN_POWER, "**", line, column);
    } else if (lexer->current_char == '=') {
        lexer_advance(lexer);
        return create_token(TOKEN_STAR_ASSIGN, "*=", line, column);
    }
    return create_token(TOKEN_STAR, "*", line, column);
```

**Step 3**: Add to expression parser at correct precedence:
```c
static ASTNode *parse_power(Parser *parser) {
    ASTNode *left = parse_unary(parser);
    
    while (parser->current_token->type == TOKEN_POWER) {
        TokenType op = parser->current_token->type;
        parser_advance(parser);
        
        ASTNode *right = parse_unary(parser);  // Right associative
        
        ASTNode *power = malloc(sizeof(ASTNode));
        power->type = AST_BINARY_OP;
        power->data.binary_op.left = left;
        power->data.binary_op.right = right;
        power->data.binary_op.op = op;
        
        left = power;
    }
    
    return left;
}
```

**Step 4**: Generate code:
```c
case TOKEN_POWER:
    // Generate power calculation
    // Note: Would need to implement or call a power function
    break;
```

### 5. Adding a New Type

**Example: Adding `short` type**

**Step 1**: Add keyword token:
```c
TOKEN_KEYWORD_SHORT,
```

**Step 2**: Recognize in lexer:
```c
{"short", TOKEN_KEYWORD_SHORT},
```

**Step 3**: Parse type:
```c
case TOKEN_KEYWORD_SHORT:
    return "short";
```

**Step 4**: Add LLVM type mapping:
```c
else if (strcmp(c_type, "short") == 0) {
    strcpy(llvm_type, "i16");
}
```

## Testing Guidelines

### Unit Tests

Create test files in `tests/`:

**Lexer test** (`test_my_feature_lex.c`):
```c
// Test tokenization of new feature
my_keyword x = 10;
```

**Parser test** (`test_my_feature_parse.c`):
```c
// Test parsing of new feature
int main() {
    my_statement {
        // body
    }
    return 0;
}
```

**Codegen test** (`test_my_feature.c`):
```c
// Full feature test
int main() {
    // Use your feature
    return 0;  // Return expected value
}
```

### Test Runner

Create Python test runner:
```python
#!/usr/bin/env python3
import subprocess
import sys

def test_my_feature():
    # Compile
    result = subprocess.run([
        './ccc', 'tests/test_my_feature.c', 
        '-o', 'tests/test_my_feature.ll'
    ], capture_output=True)
    
    assert result.returncode == 0, "Compilation failed"
    
    # Verify LLVM IR
    with open('tests/test_my_feature.ll', 'r') as f:
        ir = f.read()
        assert 'my_expected_ir' in ir
    
    # Run if needed
    # ...

if __name__ == '__main__':
    test_my_feature()
    print("All tests passed!")
```

### Integration Tests

Add to Makefile:
```makefile
test: $(TEST_TARGETS)
    python3 tests/test_my_feature.py
```

## Debugging Tips

### Enable Trace Logging

In `src/parser.c`:
```c
LOG_TRACE("Parsing my_statement at %d:%d", 
          parser->current_token->line, 
          parser->current_token->column);
```

### Add Debug Output

In `src/codegen.c`:
```c
LOG_DEBUG("Generating code for %s", 
          ast_type_to_string(node->type));
```

### Common Issues and Solutions

| Issue | Cause | Solution |
|-------|-------|----------|
| Segmentation fault | Null pointer access | Check all mallocs and pointer derefs |
| Parser infinite loop | Missing `parser_advance()` | Ensure token consumption |
| Wrong LLVM IR | Type mismatch | Verify C to LLVM type conversion |
| Memory leaks | Missing cleanup | Add to `ast_destroy()` |

### Using GDB

```bash
# Compile with debug symbols
make clean
make CFLAGS="-g -O0"

# Debug with GDB
gdb ./ccc
(gdb) run tests/test_file.c -o output.ll
(gdb) bt  # backtrace on crash
```

### Using Valgrind

```bash
# Check for memory leaks
valgrind --leak-check=full ./ccc tests/test_file.c -o output.ll

# Check for memory errors
valgrind --track-origins=yes ./ccc tests/test_file.c -o output.ll
```

## Code Style Guidelines

### Naming Conventions

- **Functions**: `snake_case` (e.g., `parse_expression`)
- **Types**: `PascalCase` (e.g., `ASTNode`, `TokenType`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `MAX_STRING_LENGTH`)
- **Struct members**: `snake_case`

### Function Organization

```c
// Static functions first
static void helper_function(void) {
    // ...
}

// Then public functions
void public_function(void) {
    // ...
}
```

### Error Handling

```c
// Check allocation
ASTNode *node = malloc(sizeof(ASTNode));
if (!node) {
    LOG_ERROR("Failed to allocate AST node");
    exit(1);
}

// Check expected tokens
if (!expect_token(parser, TOKEN_SEMICOLON)) {
    // Error already reported by expect_token
    return NULL;
}
```

### Memory Management

```c
// Always initialize pointers
char *str = NULL;

// Always free allocated memory
if (str) {
    free(str);
    str = NULL;  // Prevent use-after-free
}

// Use consistent allocation pattern
Type *obj = malloc(sizeof(Type));
memset(obj, 0, sizeof(Type));  // or use calloc
```

## Performance Considerations

### Optimization Opportunities

1. **String Pooling**: Reuse identical string literals
2. **Constant Folding**: Evaluate more expressions at compile time
3. **Dead Code Elimination**: Remove more unreachable code
4. **Register Allocation**: Better temporary usage

### Profiling

```bash
# Compile with profiling
make CFLAGS="-pg"

# Run and generate profile
./ccc large_program.c -o output.ll
gprof ccc gmon.out > profile.txt
```

## Contributing Guidelines

### Before Submitting

1. **Run all tests**: `make test`
2. **Check formatting**: Ensure consistent style
3. **Update documentation**: Document new features
4. **Add tests**: Cover new functionality
5. **Check memory**: Run valgrind tests

### Commit Messages

```
<type>: <subject>

<body>

<footer>
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `test`: Testing
- `refactor`: Code restructuring
- `perf`: Performance improvement

Example:
```
feat: Add support for short integers

- Add TOKEN_KEYWORD_SHORT to lexer
- Update type parser to recognize short
- Map short to i16 in LLVM IR
- Add comprehensive test suite

Closes #123
```

### Pull Request Process

1. Fork the repository
2. Create feature branch: `git checkout -b feat/my-feature`
3. Make changes and commit
4. Push to your fork
5. Create pull request with description

## Future Development Ideas

### High Priority
1. **Floating-point support**: Add float/double types
2. **Preprocessor**: Implement #include, #define
3. **Better errors**: Line/column in all errors, error recovery
4. **Standard library**: More built-in functions

### Medium Priority
1. **Optimizer improvements**: More optimization passes
2. **Debug information**: Generate DWARF debug info
3. **Multiple files**: Support linking multiple .c files
4. **Type checking**: Stricter type checking

### Low Priority
1. **C99 features**: VLAs, compound literals
2. **GNU extensions**: Statement expressions, typeof
3. **Inline assembly**: asm() support
4. **Link-time optimization**: LTO support

## Resources

### References
- [LLVM Language Reference](https://llvm.org/docs/LangRef.html)
- [C89/C90 Standard](http://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf)
- [Crafting Interpreters](https://craftinginterpreters.com/)
- [Engineering a Compiler](https://www.elsevier.com/books/engineering-a-compiler/cooper/978-0-12-088478-0)

### Tools
- [LLVM IR Viewer](https://godbolt.org/)
- [AST Explorer](https://astexplorer.net/)
- [Regex101](https://regex101.com/) (for lexer patterns)

### Community
- Project Issues: Report bugs and request features
- Discussions: Ask questions and share ideas
- Wiki: Additional documentation and guides