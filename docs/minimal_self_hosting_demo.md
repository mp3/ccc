# Minimal Self-Hosting Demonstration

## Concept

To demonstrate self-hosting capability without implementing all missing features, we can create a minimal subset of the compiler that can compile itself with some limitations.

## Approach

### 1. Preprocessing Step
Create a simple preprocessor script that:
- Expands `#include` directives by inlining header contents
- Removes or expands simple macros
- Handles header guards

### 2. Runtime Library Stubs
Create minimal declarations for essential functions:

```c
// runtime_stubs.h
typedef struct FILE FILE;

// Memory functions
void* malloc(int size);
void free(void* ptr);
char* strdup(char* s);

// String functions  
int strcmp(char* s1, char* s2);
int strlen(char* s);

// I/O functions
int printf(char* fmt, ...);
FILE* fopen(char* path, char* mode);
void fclose(FILE* f);
int fgetc(FILE* f);

// Process control
void exit(int code);
```

### 3. Source Code Modifications
Modify the compiler source to avoid unsupported features:

1. **Replace static variables with struct members**
   ```c
   // Instead of: static FILE *log_file;
   // Use: logger->log_file in a Logger struct
   ```

2. **Simplify variadic functions**
   ```c
   // Instead of: log_message(level, format, ...)
   // Use: log_message_simple(level, message)
   ```

3. **Replace compound operators**
   ```c
   // Instead of: x += 1;
   // Use: x = x + 1;
   ```

4. **Remove preprocessor directives**
   - Manually include all code in one file
   - Or use a simple concatenation script

### 4. Bootstrap Process

1. **Stage 0**: Use existing C compiler (gcc/clang) to build CCC
2. **Stage 1**: Preprocess CCC source files
3. **Stage 2**: Use Stage 0 CCC to compile preprocessed source
4. **Stage 3**: Verify output matches Stage 0 binary

## Implementation Example

### Simple Preprocessor (Python)
```python
#!/usr/bin/env python3
import sys
import re

def process_includes(filename, included=set()):
    if filename in included:
        return ""
    included.add(filename)
    
    result = []
    with open(filename, 'r') as f:
        for line in f:
            if line.startswith('#include "'):
                # Extract local include
                match = re.match(r'#include "(.+)"', line)
                if match:
                    inc_file = match.group(1)
                    result.append(process_includes(inc_file, included))
            elif not line.startswith('#'):
                result.append(line)
    
    return ''.join(result)

if __name__ == "__main__":
    print(process_includes(sys.argv[1]))
```

### Minimal CCC Subset
Create simplified versions of core components:

1. **Lexer**: Remove macro support, keep core tokenization
2. **Parser**: Keep essential syntax, remove unsupported operators
3. **CodeGen**: Focus on essential LLVM IR generation
4. **No Logger**: Use simple printf statements
5. **No Optimizer**: Disable optimization passes

## Benefits

1. **Proof of Concept**: Demonstrates the compiler can process C code
2. **Testing Framework**: Validates core compilation pipeline
3. **Incremental Path**: Provides foundation for full self-hosting
4. **Educational Value**: Shows what features are truly essential

## Limitations

1. No preprocessor (requires manual preprocessing)
2. No standard library (requires stubs)
3. Reduced feature set
4. Manual process steps

## Next Steps

1. Implement simple preprocessor
2. Create runtime stubs
3. Modify subset of compiler source
4. Test self-compilation
5. Gradually add features until full self-hosting

This minimal approach provides a achievable milestone while working towards full self-hosting capability.