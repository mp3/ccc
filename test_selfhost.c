// Minimal self-hosting test
// This tests if CCC can compile basic C constructs it uses

// Test struct
struct Symbol {
    int type;
    char *name;
};

// Test function with parameters
int add(int a, int b) {
    return a + b;
}

// Test static variable
static int counter = 0;

// Test preprocessor
#define MAX_SIZE 100

// Test conditional compilation
#ifdef MAX_SIZE
int size = MAX_SIZE;
#else
int size = 50;
#endif

// Test main with various features
int main() {
    // Test variable declarations
    int x = 10;
    int y = 20;
    
    // Test function call
    int sum = add(x, y);
    
    // Test malloc/free
    struct Symbol *sym = (struct Symbol *)malloc(16); // sizeof(Symbol)
    if (sym) {
        sym->type = 1;
        sym->name = "test";
        free(sym);
    }
    
    // Test loops
    for (int i = 0; i < 5; i++) {
        counter++;
    }
    
    // Test conditional
    if (counter > 0) {
        puts("Success!");
    }
    
    return 0;
}