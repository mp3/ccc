// Minimal self-hosting test
// This simulates a simplified version of CCC components

// Token types

// Simple token structure
struct Token {
    int type;
    char *text;
    int value;
};

// Simple lexer state
struct Lexer {
    char *input;
    int position;
};

// Create a new token
struct Token *create_token(int type, char *text, int value) {
    struct Token *token = (struct Token *)malloc(sizeof(struct Token));
    token->type = type;
    token->text = text;
    token->value = value;
    return token;
}

// Simple parser state
struct Parser {
    struct Token *current_token;
    int had_error;
};

// AST node types

// Simple AST node
struct ASTNode {
    int type;
    int value;
    struct ASTNode *left;
    struct ASTNode *right;
};

// Create AST node
struct ASTNode *create_ast_node(int type, int value) {
    struct ASTNode *node = (struct ASTNode *)malloc(sizeof(struct ASTNode));
    node->type = type;
    node->value = value;
    node->left = NULL;
    node->right = NULL;
    return node;
}

// Simple code generator
void codegen_node(struct ASTNode *node) {
    if (node->type == 1) {
        printf("  %%tmp = add i32 0, %d\n", node->value);
    } else if (node->type == 2) {
        codegen_node(node->left);
        codegen_node(node->right);
        puts("  %result = add i32 %tmp1, %tmp2");
    }
}

// Main compiler function
int main() {
    // Simulate lexing
    struct Token *token1 = create_token(1, "10", 10);
    struct Token *token2 = create_token(3, "+", 0);
    struct Token *token3 = create_token(1, "20", 20);
    
    // Simulate parsing
    struct ASTNode *left = create_ast_node(1, 10);
    struct ASTNode *right = create_ast_node(1, 20);
    struct ASTNode *add = create_ast_node(2, 3);
    add->left = left;
    add->right = right;
    
    // Simulate code generation
    puts("; ModuleID = 'test'");
    puts("source_filename = \"test.c\"");
    puts("");
    puts("define i32 @main() {");
    puts("entry:");
    
    codegen_node(add);
    
    puts("  ret i32 %result");
    puts("}");
    
    // Clean up
    free(token1);
    free(token2);
    free(token3);
    free(left);
    free(right);
    free(add);
    
    return 0;
}
