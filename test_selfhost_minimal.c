// Minimal self-hosting test
// This simulates a simplified version of CCC components

// Token types
#define TOKEN_INT 1
#define TOKEN_IDENTIFIER 2
#define TOKEN_PLUS 3
#define TOKEN_SEMICOLON 4
#define TOKEN_EOF 5

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
#define AST_INT_LITERAL 1
#define AST_BINARY_OP 2
#define AST_PROGRAM 3

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
    if (node->type == AST_INT_LITERAL) {
        printf("  %%tmp = add i32 0, %d\n", node->value);
    } else if (node->type == AST_BINARY_OP) {
        codegen_node(node->left);
        codegen_node(node->right);
        puts("  %result = add i32 %tmp1, %tmp2");
    }
}

// Main compiler function
int main() {
    // Simulate lexing
    struct Token *token1 = create_token(TOKEN_INT, "10", 10);
    struct Token *token2 = create_token(TOKEN_PLUS, "+", 0);
    struct Token *token3 = create_token(TOKEN_INT, "20", 20);
    
    // Simulate parsing
    struct ASTNode *left = create_ast_node(AST_INT_LITERAL, 10);
    struct ASTNode *right = create_ast_node(AST_INT_LITERAL, 20);
    struct ASTNode *add = create_ast_node(AST_BINARY_OP, TOKEN_PLUS);
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