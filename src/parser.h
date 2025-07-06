#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    AST_PROGRAM,
    AST_FUNCTION,
    AST_COMPOUND_STMT,
    AST_RETURN_STMT,
    AST_IF_STMT,
    AST_WHILE_STMT,
    AST_EXPR_STMT,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_INT_LITERAL,
    AST_IDENTIFIER,
    AST_ASSIGNMENT,
    AST_VAR_DECL
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    int line;
    int column;
    union {
        struct {
            struct ASTNode **functions;
            int function_count;
        } program;
        struct {
            char *name;
            char *return_type;
            struct ASTNode *body;
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
        } unary_op;
        struct {
            int value;
        } int_literal;
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
        } var_decl;
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

#endif
