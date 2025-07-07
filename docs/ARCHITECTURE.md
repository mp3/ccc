# CCC Compiler Architecture

This document describes the internal architecture and design of the CCC compiler.

## Overview

The CCC compiler follows a traditional compiler architecture with distinct phases:

```
Source Code → Lexer → Parser → AST → Optimizer → Code Generator → LLVM IR
```

## Component Architecture

### 1. Lexer (lexer.c/h)

The lexical analyzer converts source code into a stream of tokens.

**Key Components:**
- `Lexer` structure: Maintains file handle and current position
- `Token` structure: Represents a lexical token with type, value, and location
- `lexer_next_token()`: Main tokenization function

**Token Types:**
- Keywords: `TOKEN_KEYWORD_INT`, `TOKEN_KEYWORD_IF`, etc.
- Identifiers: `TOKEN_IDENTIFIER`
- Literals: `TOKEN_INT_LITERAL`, `TOKEN_CHAR_LITERAL`, `TOKEN_STRING_LITERAL`
- Operators: `TOKEN_PLUS`, `TOKEN_MINUS`, etc.
- Delimiters: `TOKEN_LPAREN`, `TOKEN_SEMICOLON`, etc.

**Lexing Process:**
1. Skip whitespace and comments
2. Identify token type based on first character
3. Consume characters to form complete token
4. Track line and column for error reporting

### 2. Parser (parser.c/h)

The parser builds an Abstract Syntax Tree (AST) from the token stream.

**Parsing Strategy:**
- Recursive descent parsing
- Operator precedence parsing for expressions
- Left-to-right parsing with one token lookahead

**Key Functions:**
```c
// Top-level parsing
ASTNode *parser_parse(Parser *parser);

// Declaration parsing
static ASTNode *parse_function(Parser *parser);
static ASTNode *parse_struct_declaration(Parser *parser);
static ASTNode *parse_enum_declaration(Parser *parser);

// Statement parsing
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_if_statement(Parser *parser);
static ASTNode *parse_while_statement(Parser *parser);
static ASTNode *parse_for_statement(Parser *parser);

// Expression parsing with precedence
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_assignment(Parser *parser);
static ASTNode *parse_ternary(Parser *parser);
static ASTNode *parse_logical_or(Parser *parser);
// ... more precedence levels
```

**Operator Precedence (highest to lowest):**
1. Postfix: `()`, `[]`, `.`, `->`, `++`, `--`
2. Prefix: `++`, `--`, `+`, `-`, `!`, `~`, `*`, `&`, `sizeof`, cast
3. Multiplicative: `*`, `/`, `%`
4. Additive: `+`, `-`
5. Shift: `<<`, `>>`
6. Relational: `<`, `>`, `<=`, `>=`
7. Equality: `==`, `!=`
8. Bitwise AND: `&`
9. Bitwise XOR: `^`
10. Bitwise OR: `|`
11. Logical AND: `&&`
12. Logical OR: `||`
13. Ternary: `? :`
14. Assignment: `=`, `+=`, `-=`, etc.
15. Comma: `,`

### 3. Abstract Syntax Tree (parser.h)

The AST represents the program structure.

**Node Types:**
```c
typedef enum {
    // Literals
    AST_INT_LITERAL,
    AST_CHAR_LITERAL,
    AST_STRING_LITERAL,
    
    // Expressions
    AST_IDENTIFIER,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_ASSIGNMENT,
    AST_FUNCTION_CALL,
    
    // Statements
    AST_VAR_DECL,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_FOR_STMT,
    AST_RETURN_STMT,
    
    // Declarations
    AST_FUNCTION,
    AST_STRUCT_DECL,
    AST_ENUM_DECL,
    
    // ... more types
} ASTNodeType;
```

**Node Structure:**
```c
typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    union {
        // Node-specific data
        struct { int value; } int_literal;
        struct { char *name; } identifier;
        struct { 
            struct ASTNode *left;
            struct ASTNode *right;
            TokenType op;
        } binary_op;
        // ... more node data
    } data;
} ASTNode;
```

### 4. Symbol Table (symtab.c/h)

Manages variable and function declarations with scoping.

**Key Features:**
- Hierarchical scope management
- Type information storage
- Function signature tracking
- Static variable management

**Symbol Structure:**
```c
typedef struct Symbol {
    char *name;
    char *data_type;
    bool is_function;
    bool is_func_ptr;
    bool is_variadic;
    int param_count;
    char **param_types;
    int offset;  // For local variables
    struct Symbol *next;
} Symbol;
```

**Scope Management:**
```c
typedef struct SymbolTable {
    struct Symbol *symbols;
    struct SymbolTable *parent;
    int current_offset;
} SymbolTable;
```

### 5. Code Generator (codegen.c/h)

Generates LLVM IR from the AST.

**Key Components:**
- SSA value generation with automatic temporaries
- Type conversion between C and LLVM types
- Function and variable management
- String literal pooling

**Code Generation Process:**
1. Generate function declarations
2. Generate global variables
3. Generate string literals
4. Generate function definitions
5. Generate expressions in SSA form

**LLVM IR Features Used:**
- SSA form with phi nodes
- Typed instructions
- Function declarations and definitions
- Global constants for strings
- Proper calling conventions

**Example Generation:**
```c
// C code
int add(int a, int b) {
    return a + b;
}

// Generated LLVM IR
define i32 @add(i32 %a.param, i32 %b.param) {
entry:
  %a = alloca i32
  store i32 %a.param, i32* %a
  %b = alloca i32
  store i32 %b.param, i32* %b
  %tmp0 = load i32, i32* %a
  %tmp1 = load i32, i32* %b
  %tmp2 = add i32 %tmp0, %tmp1
  ret i32 %tmp2
}
```

### 6. Optimizer (optimizer.c/h)

Performs AST-level optimizations before code generation.

**Optimization Passes:**

1. **Constant Folding**
   - Evaluates constant expressions at compile time
   - Reduces arithmetic, comparison, and logical operations
   - Propagates constants through the AST

2. **Dead Code Elimination**
   - Removes unreachable code
   - Eliminates if statements with constant conditions
   - Removes while loops that never execute

**Optimization Infrastructure:**
```c
typedef struct Optimizer {
    bool enable_constant_folding;
    bool enable_dead_code_elimination;
    int optimizations_performed;
} Optimizer;
```

### 7. Logger (logger.c/h)

Provides debugging and diagnostic output.

**Log Levels:**
- `LOG_TRACE`: Detailed execution trace
- `LOG_DEBUG`: Debug information
- `LOG_INFO`: General information
- `LOG_WARN`: Warnings
- `LOG_ERROR`: Error messages

**Features:**
- Timestamped messages
- File and line information
- Configurable log levels
- Output to file or stderr

## Data Flow

### 1. Lexing Phase
```
Source File → Character Stream → Token Stream
```

### 2. Parsing Phase
```
Token Stream → Parse Tree → Abstract Syntax Tree
```

### 3. Semantic Analysis
```
AST + Symbol Table → Type-Checked AST
```

### 4. Optimization Phase
```
AST → Optimized AST
```

### 5. Code Generation Phase
```
Optimized AST → LLVM IR
```

## Memory Management

The compiler uses manual memory management:

- **AST Nodes**: Allocated with `malloc`, freed with `ast_destroy()`
- **Symbol Tables**: Hierarchical structure with parent pointers
- **Strings**: Duplicated with `strdup()` for ownership
- **Temporaries**: Allocated during code generation

**Cleanup Process:**
1. Destroy AST recursively
2. Destroy symbol tables
3. Free string literals
4. Free static variable records

## Error Handling

Error handling is implemented at multiple levels:

### Lexer Errors
- Invalid characters
- Unterminated strings
- Invalid escape sequences

### Parser Errors
- Syntax errors with expected/found tokens
- Unexpected end of file
- Invalid declarations

### Semantic Errors
- Undefined variables/functions
- Type mismatches
- Invalid operations
- Redefinitions

### Code Generation Errors
- Internal compiler errors
- Unsupported features

## Extensions and Modifications

### Adding a New Operator

1. Add token type to `lexer.h`
2. Update lexer to recognize operator
3. Add parsing logic at appropriate precedence level
4. Add AST node handling
5. Implement code generation

### Adding a New Statement Type

1. Add AST node type
2. Implement parsing function
3. Update statement parser dispatch
4. Add code generation logic
5. Update optimizer if needed

### Adding a New Type

1. Update type parsing
2. Add type conversion functions
3. Update symbol table handling
4. Implement code generation mapping
5. Add test cases

## Performance Considerations

- **Single-pass compilation**: No intermediate representation between AST and LLVM IR
- **Minimal memory allocation**: Reuses buffers where possible
- **Direct LLVM IR generation**: No additional IR transformation passes
- **Simple optimization passes**: Focus on common patterns

## Limitations and Trade-offs

### Design Decisions
- **No preprocessor**: Simplifies compilation model
- **Single file compilation**: Avoids linking complexity
- **Limited type system**: Focuses on core C89 features
- **Direct LLVM IR**: Skips intermediate representations

### Technical Limitations
- **No register allocation**: Relies on LLVM's optimizer
- **Simple error recovery**: Stops at first error
- **Limited optimization**: Only basic AST-level optimizations
- **No debug information**: No DWARF generation

## Future Architecture Improvements

1. **Multi-pass optimization**: Separate analysis and transformation passes
2. **Better error recovery**: Continue parsing after errors
3. **Incremental compilation**: Cache parsed modules
4. **Debug information**: Generate LLVM debug metadata
5. **Language server protocol**: Support for IDE integration