#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdbool.h>

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_COMPOUND_STMT,
    AST_RETURN_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_DO_WHILE_STMT,
    AST_FOR_STMT,
    AST_BREAK_STMT,
    AST_CONTINUE_STMT,
    AST_EXPR_STMT,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_INT_LITERAL,
    AST_CHAR_LITERAL,
    AST_STRING_LITERAL,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
    AST_VAR_DECL,
    AST_FUNCTION_CALL,
    AST_PARAM_DECL,
    AST_ARRAY_ACCESS,
    AST_ADDRESS_OF,
    AST_DEREFERENCE,
    AST_STRUCT_DECL,
    AST_MEMBER_ACCESS,
    AST_SIZEOF,
    AST_SWITCH_STMT,
    AST_CASE_STMT,
    AST_DEFAULT_STMT,
    AST_TERNARY,
    AST_CAST,
    AST_TYPEDEF_DECL
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    union {
        struct {
            struct ASTNode **functions;
            int function_count;
            struct ASTNode **global_vars;
            int global_var_count;
            struct ASTNode **typedefs;
            int typedef_count;
        } program;
        struct {
            char *name;
            char *return_type;
            struct ASTNode *body;
            struct ASTNode **params;
            int param_count;
        } function;
        struct {
            struct ASTNode **statements;
            int statement_count;
        } compound;
        struct {
            struct ASTNode *expression;
        } return_stmt;
        struct {
            struct ASTNode *condition;
            struct ASTNode *then_stmt;
            struct ASTNode *else_stmt;
        } if_stmt;
        struct {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        struct {
            struct ASTNode *body;
            struct ASTNode *condition;
        } do_while_stmt;
        struct {
            struct ASTNode *init;       // Can be NULL
            struct ASTNode *condition;  // Can be NULL
            struct ASTNode *update;     // Can be NULL
            struct ASTNode *body;
        } for_stmt;
        struct {
            struct ASTNode *expression;
        } expr_stmt;
        struct {
            TokenType op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;
        struct {
            TokenType op;
            struct ASTNode *operand;
            bool is_prefix;  // true for prefix ++/--; false for postfix ++/--
        } unary_op;
        struct {
            int value;
        } int_literal;
        struct {
            char value;
        } char_literal;
        struct {
            char *value;
        } string_literal;
        struct {
            char *name;
        } identifier;
        struct {
            char *name;
            struct ASTNode *value;
        } assignment;
        struct {
            char *type;
            char *name;
            struct ASTNode *initializer;
            struct ASTNode *array_size;  // NULL for non-arrays
        } var_decl;
        struct {
            char *name;
            struct ASTNode **arguments;
            int argument_count;
        } function_call;
        struct {
            char *type;
            char *name;
        } param_decl;
        struct {
            struct ASTNode *array;
            struct ASTNode *index;
        } array_access;
        struct {
            char *name;
            struct ASTNode **members;
            int member_count;
        } struct_decl;
        struct {
            struct ASTNode *object;
            char *member_name;
        } member_access;
        struct {
            char *type_name;               // For sizeof(type) - e.g., "int", "char*"
            struct ASTNode *expression;    // For sizeof(expression) - mutually exclusive with type_name
        } sizeof_op;
        struct {
            struct ASTNode *expression;    // Expression to switch on
            struct ASTNode **cases;        // Array of case statements
            int case_count;
            struct ASTNode *default_case;  // Optional default case
        } switch_stmt;
        struct {
            struct ASTNode *value;         // Case value (must be constant)
            struct ASTNode **statements;   // Statements for this case
            int statement_count;
        } case_stmt;
        struct {
            struct ASTNode **statements;   // Statements for default case
            int statement_count;
        } default_stmt;
        struct {
            struct ASTNode *condition;      // Condition expression
            struct ASTNode *true_expr;      // Expression if true
            struct ASTNode *false_expr;     // Expression if false
        } ternary;
        struct {
            char *target_type;              // Type to cast to (e.g., "int", "char*")
            struct ASTNode *expression;     // Expression to cast
        } cast;
        struct {
            char *name;                     // The new type name being defined
            char *base_type;                // The existing type it's based on
        } typedef_decl;
    } data;
} ASTNode;

typedef struct {
    Lexer *lexer;
    Token *current_token;
    Token *peek_token;
} Parser;

Parser *parser_create(Lexer *lexer);
void parser_destroy(Parser *parser);
ASTNode *parser_parse(Parser *parser);
void ast_destroy(ASTNode *node);
void ast_print(ASTNode *node, int indent);
ASTNode *ast_clone(ASTNode *node);

#endif
