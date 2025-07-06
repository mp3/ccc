#include "parser.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static void parser_advance(Parser *parser) {
    token_destroy(parser->current_token);
    parser->current_token = parser->peek_token;
    parser->peek_token = lexer_next_token(parser->lexer);
}

static bool parser_match(Parser *parser, TokenType type) {
    if (parser->current_token->type == type) {
        parser_advance(parser);
        return true;
    }
    return false;
}

static void parser_expect(Parser *parser, TokenType type) {
    if (parser->current_token->type != type) {
        LOG_ERROR("Expected %s but got %s at %d:%d",
                  token_type_to_string(type),
                  token_type_to_string(parser->current_token->type),
                  parser->current_token->line,
                  parser->current_token->column);
        exit(1);
    }
    parser_advance(parser);
}

static ASTNode *create_ast_node(ASTNodeType type, int line, int column) {
    ASTNode *node = calloc(1, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    node->column = column;
    return node;
}

Parser *parser_create(Lexer *lexer) {
    Parser *parser = malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    parser->peek_token = lexer_next_token(lexer);
    LOG_DEBUG("Created parser");
    return parser;
}

void parser_destroy(Parser *parser) {
    if (parser) {
        token_destroy(parser->current_token);
        token_destroy(parser->peek_token);
        free(parser);
    }
}

static ASTNode *parse_primary(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_statement(Parser *parser);

static ASTNode *parse_primary(Parser *parser) {
    Token *token = parser->current_token;
    
    if (token->type == TOKEN_INT_LITERAL) {
        ASTNode *node = create_ast_node(AST_INT_LITERAL, token->line, token->column);
        node->data.int_literal.value = token->value.int_value;
        parser_advance(parser);
        LOG_TRACE("Parsed int literal: %d", node->data.int_literal.value);
        return node;
    }
    
    if (token->type == TOKEN_IDENTIFIER) {
        ASTNode *node = create_ast_node(AST_IDENTIFIER, token->line, token->column);
        node->data.identifier.name = strdup(token->text);
        parser_advance(parser);
        LOG_TRACE("Parsed identifier: %s", node->data.identifier.name);
        return node;
    }
    
    if (parser_match(parser, TOKEN_LPAREN)) {
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        return expr;
    }
    
    LOG_ERROR("Unexpected token in primary expression: %s at %d:%d",
              token_type_to_string(token->type), token->line, token->column);
    exit(1);
}

static ASTNode *parse_multiplicative(Parser *parser) {
    ASTNode *left = parse_primary(parser);
    
    while (parser->current_token->type == TOKEN_STAR ||
           parser->current_token->type == TOKEN_SLASH) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_primary(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_additive(Parser *parser) {
    ASTNode *left = parse_multiplicative(parser);
    
    while (parser->current_token->type == TOKEN_PLUS ||
           parser->current_token->type == TOKEN_MINUS) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_multiplicative(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_comparison(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    
    while (parser->current_token->type >= TOKEN_EQ &&
           parser->current_token->type <= TOKEN_GE) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_additive(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_expression(Parser *parser) {
    return parse_comparison(parser);
}

static ASTNode *parse_compound_statement(Parser *parser) {
    parser_expect(parser, TOKEN_LBRACE);
    
    ASTNode *node = create_ast_node(AST_COMPOUND_STMT, 
                                    parser->current_token->line,
                                    parser->current_token->column);
    
    int capacity = 8;
    node->data.compound.statements = malloc(capacity * sizeof(ASTNode*));
    node->data.compound.statement_count = 0;
    
    while (parser->current_token->type != TOKEN_RBRACE &&
           parser->current_token->type != TOKEN_EOF) {
        if (node->data.compound.statement_count >= capacity) {
            capacity *= 2;
            node->data.compound.statements = realloc(node->data.compound.statements,
                                                    capacity * sizeof(ASTNode*));
        }
        node->data.compound.statements[node->data.compound.statement_count++] = 
            parse_statement(parser);
    }
    
    parser_expect(parser, TOKEN_RBRACE);
    return node;
}

static ASTNode *parse_statement(Parser *parser) {
    Token *token = parser->current_token;
    
    if (token->type == TOKEN_KEYWORD_RETURN) {
        parser_advance(parser);
        ASTNode *node = create_ast_node(AST_RETURN_STMT, token->line, token->column);
        node->data.return_stmt.expression = parse_expression(parser);
        parser_expect(parser, TOKEN_SEMICOLON);
        LOG_TRACE("Parsed return statement");
        return node;
    }
    
    if (token->type == TOKEN_LBRACE) {
        return parse_compound_statement(parser);
    }
    
    // Expression statement
    ASTNode *node = create_ast_node(AST_EXPR_STMT, token->line, token->column);
    node->data.expr_stmt.expression = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    return node;
}

static ASTNode *parse_function(Parser *parser) {
    parser_expect(parser, TOKEN_KEYWORD_INT);
    
    Token *name_token = parser->current_token;
    char *func_name = strdup(name_token->text);  // Save the name before advancing
    parser_expect(parser, TOKEN_IDENTIFIER);
    
    parser_expect(parser, TOKEN_LPAREN);
    parser_expect(parser, TOKEN_RPAREN);
    
    ASTNode *node = create_ast_node(AST_FUNCTION, name_token->line, name_token->column);
    node->data.function.name = func_name;
    node->data.function.return_type = strdup("int");
    node->data.function.body = parse_compound_statement(parser);
    
    LOG_DEBUG("Parsed function: %s", node->data.function.name);
    return node;
}

ASTNode *parser_parse(Parser *parser) {
    ASTNode *program = create_ast_node(AST_PROGRAM, 1, 1);
    
    int capacity = 8;
    program->data.program.functions = malloc(capacity * sizeof(ASTNode*));
    program->data.program.function_count = 0;
    
    while (parser->current_token->type != TOKEN_EOF) {
        if (program->data.program.function_count >= capacity) {
            capacity *= 2;
            program->data.program.functions = realloc(program->data.program.functions,
                                                     capacity * sizeof(ASTNode*));
        }
        program->data.program.functions[program->data.program.function_count++] = 
            parse_function(parser);
    }
    
    LOG_INFO("Parsed program with %d functions", program->data.program.function_count);
    return program;
}

void ast_destroy(ASTNode *node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.function_count; i++) {
                ast_destroy(node->data.program.functions[i]);
            }
            free(node->data.program.functions);
            break;
        case AST_FUNCTION:
            free(node->data.function.name);
            free(node->data.function.return_type);
            ast_destroy(node->data.function.body);
            break;
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound.statement_count; i++) {
                ast_destroy(node->data.compound.statements[i]);
            }
            free(node->data.compound.statements);
            break;
        case AST_RETURN_STMT:
            ast_destroy(node->data.return_stmt.expression);
            break;
        case AST_EXPR_STMT:
            ast_destroy(node->data.expr_stmt.expression);
            break;
        case AST_BINARY_OP:
            ast_destroy(node->data.binary_op.left);
            ast_destroy(node->data.binary_op.right);
            break;
        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;
        default:
            break;
    }
    
    free(node);
}

void ast_print(ASTNode *node, int indent) {
    if (!node) return;
    
    for (int i = 0; i < indent; i++) printf("  ");
    
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program\n");
            for (int i = 0; i < node->data.program.function_count; i++) {
                ast_print(node->data.program.functions[i], indent + 1);
            }
            break;
        case AST_FUNCTION:
            printf("Function: %s returns %s\n", 
                   node->data.function.name, node->data.function.return_type);
            ast_print(node->data.function.body, indent + 1);
            break;
        case AST_COMPOUND_STMT:
            printf("Compound Statement\n");
            for (int i = 0; i < node->data.compound.statement_count; i++) {
                ast_print(node->data.compound.statements[i], indent + 1);
            }
            break;
        case AST_RETURN_STMT:
            printf("Return\n");
            ast_print(node->data.return_stmt.expression, indent + 1);
            break;
        case AST_BINARY_OP:
            printf("Binary Op: %s\n", token_type_to_string(node->data.binary_op.op));
            ast_print(node->data.binary_op.left, indent + 1);
            ast_print(node->data.binary_op.right, indent + 1);
            break;
        case AST_INT_LITERAL:
            printf("Int: %d\n", node->data.int_literal.value);
            break;
        case AST_IDENTIFIER:
            printf("Identifier: %s\n", node->data.identifier.name);
            break;
        default:
            printf("Unknown node type: %d\n", node->type);
    }
}
