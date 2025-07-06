#include "parser.h"
#include "logger.h"
#include "symtab.h"
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
    
    // Handle address-of operator
    if (token->type == TOKEN_AMPERSAND) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser);
        ASTNode *operand = parse_primary(parser);
        ASTNode *node = create_ast_node(AST_ADDRESS_OF, line, column);
        node->data.unary_op.op = TOKEN_AMPERSAND;
        node->data.unary_op.operand = operand;
        LOG_TRACE("Parsed address-of operator");
        return node;
    }
    
    // Handle dereference operator
    if (token->type == TOKEN_STAR) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser);
        ASTNode *operand = parse_primary(parser);
        ASTNode *node = create_ast_node(AST_DEREFERENCE, line, column);
        node->data.unary_op.op = TOKEN_STAR;
        node->data.unary_op.operand = operand;
        LOG_TRACE("Parsed dereference operator");
        return node;
    }
    
    if (token->type == TOKEN_INT_LITERAL) {
        ASTNode *node = create_ast_node(AST_INT_LITERAL, token->line, token->column);
        node->data.int_literal.value = token->value.int_value;
        parser_advance(parser);
        LOG_TRACE("Parsed int literal: %d", node->data.int_literal.value);
        return node;
    }
    
    if (token->type == TOKEN_CHAR_LITERAL) {
        ASTNode *node = create_ast_node(AST_CHAR_LITERAL, token->line, token->column);
        node->data.char_literal.value = token->value.char_value;
        parser_advance(parser);
        LOG_TRACE("Parsed char literal: '%c'", node->data.char_literal.value);
        return node;
    }
    
    if (token->type == TOKEN_STRING_LITERAL) {
        ASTNode *node = create_ast_node(AST_STRING_LITERAL, token->line, token->column);
        // Remove quotes from the string
        char *str = strdup(token->text);
        int len = strlen(str);
        if (len >= 2 && str[0] == '"' && str[len-1] == '"') {
            str[len-1] = '\0';
            node->data.string_literal.value = strdup(str + 1);
            free(str);
        } else {
            node->data.string_literal.value = str;
        }
        parser_advance(parser);
        LOG_TRACE("Parsed string literal: \"%s\"", node->data.string_literal.value);
        return node;
    }
    
    if (token->type == TOKEN_IDENTIFIER) {
        char *name = strdup(token->text);
        int line = token->line;
        int column = token->column;
        parser_advance(parser);
        
        // Check if it's a function call
        if (parser->current_token->type == TOKEN_LPAREN) {
            parser_advance(parser);
            
            // Parse arguments
            int arg_capacity = 4;
            ASTNode **arguments = malloc(arg_capacity * sizeof(ASTNode*));
            int arg_count = 0;
            
            if (parser->current_token->type != TOKEN_RPAREN) {
                arguments[arg_count++] = parse_expression(parser);
                
                while (parser->current_token->type == TOKEN_COMMA) {
                    parser_advance(parser);
                    if (arg_count >= arg_capacity) {
                        arg_capacity *= 2;
                        arguments = realloc(arguments, arg_capacity * sizeof(ASTNode*));
                    }
                    arguments[arg_count++] = parse_expression(parser);
                }
            }
            
            parser_expect(parser, TOKEN_RPAREN);
            
            ASTNode *node = create_ast_node(AST_FUNCTION_CALL, line, column);
            node->data.function_call.name = name;
            node->data.function_call.arguments = arguments;
            node->data.function_call.argument_count = arg_count;
            LOG_TRACE("Parsed function call: %s with %d arguments", name, arg_count);
            return node;
        } else if (parser->current_token->type == TOKEN_LBRACKET) {
            // Array access
            parser_advance(parser);
            ASTNode *index = parse_expression(parser);
            parser_expect(parser, TOKEN_RBRACKET);
            
            ASTNode *array_node = create_ast_node(AST_IDENTIFIER, line, column);
            array_node->data.identifier.name = name;
            
            ASTNode *node = create_ast_node(AST_ARRAY_ACCESS, line, column);
            node->data.array_access.array = array_node;
            node->data.array_access.index = index;
            LOG_TRACE("Parsed array access: %s[...]", name);
            return node;
        } else {
            // Regular identifier
            ASTNode *node = create_ast_node(AST_IDENTIFIER, line, column);
            node->data.identifier.name = name;
            LOG_TRACE("Parsed identifier: %s", name);
            return node;
        }
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

static ASTNode *parse_assignment(Parser *parser) {
    ASTNode *left = parse_comparison(parser);
    
    if (parser->current_token->type == TOKEN_ASSIGN) {
        if (left->type != AST_IDENTIFIER && left->type != AST_ARRAY_ACCESS && left->type != AST_DEREFERENCE) {
            LOG_ERROR("Invalid assignment target at %d:%d", 
                     left->line, left->column);
            exit(1);
        }
        
        Token *op_token = parser->current_token;
        parser_advance(parser);
        ASTNode *right = parse_assignment(parser);  // Right associative
        
        if (left->type == AST_IDENTIFIER) {
            ASTNode *node = create_ast_node(AST_ASSIGNMENT, op_token->line, op_token->column);
            node->data.assignment.name = strdup(left->data.identifier.name);
            node->data.assignment.value = right;
            ast_destroy(left);  // We copied the name, so destroy the identifier node
            return node;
        } else {
            // Array element assignment or pointer dereference assignment
            // Keep as a binary op with TOKEN_ASSIGN
            ASTNode *assign_op = create_ast_node(AST_BINARY_OP, op_token->line, op_token->column);
            assign_op->data.binary_op.op = TOKEN_ASSIGN;
            assign_op->data.binary_op.left = left;
            assign_op->data.binary_op.right = right;
            return assign_op;
        }
    }
    
    return left;
}

static ASTNode *parse_expression(Parser *parser) {
    return parse_assignment(parser);
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
    
    // Variable declaration
    if (token->type == TOKEN_KEYWORD_INT || token->type == TOKEN_KEYWORD_CHAR) {
        char *base_type = strdup(token->type == TOKEN_KEYWORD_INT ? "int" : "char");
        parser_advance(parser);
        
        // Check for pointer type
        int pointer_count = 0;
        while (parser->current_token->type == TOKEN_STAR) {
            pointer_count++;
            parser_advance(parser);
        }
        
        // Build the complete type string
        char type_name[256];
        strcpy(type_name, base_type);
        for (int i = 0; i < pointer_count; i++) {
            strcat(type_name, "*");
        }
        
        Token *name_token = parser->current_token;
        char *var_name = strdup(name_token->text);
        int var_line = name_token->line;
        int var_column = name_token->column;
        parser_expect(parser, TOKEN_IDENTIFIER);
        
        free(base_type);
        
        ASTNode *node = create_ast_node(AST_VAR_DECL, var_line, var_column);
        node->data.var_decl.type = strdup(type_name);
        node->data.var_decl.name = var_name;
        node->data.var_decl.array_size = NULL;
        
        // Check for array declaration
        if (parser->current_token->type == TOKEN_LBRACKET) {
            parser_advance(parser);
            node->data.var_decl.array_size = parse_expression(parser);
            parser_expect(parser, TOKEN_RBRACKET);
        }
        
        // Optional initializer
        if (parser->current_token->type == TOKEN_ASSIGN) {
            parser_advance(parser);
            node->data.var_decl.initializer = parse_expression(parser);
        } else {
            node->data.var_decl.initializer = NULL;
        }
        
        parser_expect(parser, TOKEN_SEMICOLON);
        if (node->data.var_decl.array_size) {
            LOG_TRACE("Parsed array declaration: %s %s[...]", type_name, node->data.var_decl.name);
        } else {
            LOG_TRACE("Parsed variable declaration: %s %s", type_name, node->data.var_decl.name);
        }
        return node;
    }
    
    if (token->type == TOKEN_KEYWORD_IF) {
        parser_advance(parser);
        parser_expect(parser, TOKEN_LPAREN);
        
        ASTNode *condition = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        
        ASTNode *then_stmt = parse_statement(parser);
        ASTNode *else_stmt = NULL;
        
        // Check for else clause
        if (parser->current_token->type == TOKEN_KEYWORD_ELSE) {
            parser_advance(parser);
            else_stmt = parse_statement(parser);
        }
        
        ASTNode *node = create_ast_node(AST_IF_STMT, token->line, token->column);
        node->data.if_stmt.condition = condition;
        node->data.if_stmt.then_stmt = then_stmt;
        node->data.if_stmt.else_stmt = else_stmt;
        LOG_TRACE("Parsed if statement");
        return node;
    }
    
    if (token->type == TOKEN_KEYWORD_WHILE) {
        parser_advance(parser);
        parser_expect(parser, TOKEN_LPAREN);
        
        ASTNode *condition = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        
        ASTNode *body = parse_statement(parser);
        
        ASTNode *node = create_ast_node(AST_WHILE_STMT, token->line, token->column);
        node->data.while_stmt.condition = condition;
        node->data.while_stmt.body = body;
        LOG_TRACE("Parsed while statement");
        return node;
    }
    
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

static ASTNode *parse_parameter(Parser *parser) {
    char *base_type = NULL;
    if (parser->current_token->type == TOKEN_KEYWORD_INT) {
        base_type = strdup("int");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_CHAR) {
        base_type = strdup("char");
        parser_advance(parser);
    } else {
        LOG_ERROR("Expected type specifier but got %s", 
                  token_type_to_string(parser->current_token->type));
        exit(1);
    }
    
    // Check for pointer type
    int pointer_count = 0;
    while (parser->current_token->type == TOKEN_STAR) {
        pointer_count++;
        parser_advance(parser);
    }
    
    // Build the complete type string
    char type_name[256];
    strcpy(type_name, base_type);
    for (int i = 0; i < pointer_count; i++) {
        strcat(type_name, "*");
    }
    free(base_type);
    
    Token *name_token = parser->current_token;
    char *param_name = strdup(name_token->text);
    int param_line = name_token->line;
    int param_column = name_token->column;
    parser_expect(parser, TOKEN_IDENTIFIER);
    
    ASTNode *param = create_ast_node(AST_PARAM_DECL, param_line, param_column);
    param->data.param_decl.type = strdup(type_name);
    param->data.param_decl.name = param_name;
    
    return param;
}

static ASTNode *parse_function(Parser *parser) {
    char *base_type = NULL;
    if (parser->current_token->type == TOKEN_KEYWORD_INT) {
        base_type = strdup("int");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_CHAR) {
        base_type = strdup("char");
        parser_advance(parser);
    } else {
        LOG_ERROR("Expected return type but got %s", 
                  token_type_to_string(parser->current_token->type));
        exit(1);
    }
    
    // Check for pointer return type
    int pointer_count = 0;
    while (parser->current_token->type == TOKEN_STAR) {
        pointer_count++;
        parser_advance(parser);
    }
    
    // Build the complete return type string
    char return_type[256];
    strcpy(return_type, base_type);
    for (int i = 0; i < pointer_count; i++) {
        strcat(return_type, "*");
    }
    free(base_type);
    
    Token *name_token = parser->current_token;
    char *func_name = strdup(name_token->text);  // Save the name before advancing
    int func_line = name_token->line;
    int func_column = name_token->column;
    parser_expect(parser, TOKEN_IDENTIFIER);
    
    parser_expect(parser, TOKEN_LPAREN);
    
    // Parse parameters
    int param_capacity = 4;
    ASTNode **params = malloc(param_capacity * sizeof(ASTNode*));
    int param_count = 0;
    
    if (parser->current_token->type != TOKEN_RPAREN) {
        params[param_count++] = parse_parameter(parser);
        
        while (parser->current_token->type == TOKEN_COMMA) {
            parser_advance(parser);
            if (param_count >= param_capacity) {
                param_capacity *= 2;
                params = realloc(params, param_capacity * sizeof(ASTNode*));
            }
            params[param_count++] = parse_parameter(parser);
        }
    }
    
    parser_expect(parser, TOKEN_RPAREN);
    
    ASTNode *node = create_ast_node(AST_FUNCTION, func_line, func_column);
    node->data.function.name = func_name;
    node->data.function.return_type = strdup(return_type);
    node->data.function.params = params;
    node->data.function.param_count = param_count;
    node->data.function.body = parse_compound_statement(parser);
    
    LOG_DEBUG("Parsed function: %s with %d parameters", node->data.function.name, param_count);
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
            for (int i = 0; i < node->data.function.param_count; i++) {
                ast_destroy(node->data.function.params[i]);
            }
            free(node->data.function.params);
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
        case AST_ASSIGNMENT:
            free(node->data.assignment.name);
            ast_destroy(node->data.assignment.value);
            break;
        case AST_VAR_DECL:
            free(node->data.var_decl.type);
            free(node->data.var_decl.name);
            ast_destroy(node->data.var_decl.initializer);
            ast_destroy(node->data.var_decl.array_size);
            break;
        case AST_IF_STMT:
            ast_destroy(node->data.if_stmt.condition);
            ast_destroy(node->data.if_stmt.then_stmt);
            ast_destroy(node->data.if_stmt.else_stmt);
            break;
        case AST_WHILE_STMT:
            ast_destroy(node->data.while_stmt.condition);
            ast_destroy(node->data.while_stmt.body);
            break;
        case AST_FUNCTION_CALL:
            free(node->data.function_call.name);
            for (int i = 0; i < node->data.function_call.argument_count; i++) {
                ast_destroy(node->data.function_call.arguments[i]);
            }
            free(node->data.function_call.arguments);
            break;
        case AST_PARAM_DECL:
            free(node->data.param_decl.type);
            free(node->data.param_decl.name);
            break;
        case AST_CHAR_LITERAL:
            // No dynamic memory to free
            break;
        case AST_STRING_LITERAL:
            free(node->data.string_literal.value);
            break;
        case AST_ARRAY_ACCESS:
            ast_destroy(node->data.array_access.array);
            ast_destroy(node->data.array_access.index);
            break;
        case AST_ADDRESS_OF:
        case AST_DEREFERENCE:
            ast_destroy(node->data.unary_op.operand);
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
            printf("Function: %s returns %s", 
                   node->data.function.name, node->data.function.return_type);
            if (node->data.function.param_count > 0) {
                printf(" (");
                for (int i = 0; i < node->data.function.param_count; i++) {
                    if (i > 0) printf(", ");
                    printf("%s %s", node->data.function.params[i]->data.param_decl.type,
                           node->data.function.params[i]->data.param_decl.name);
                }
                printf(")");
            }
            printf("\n");
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
        case AST_ASSIGNMENT:
            printf("Assignment: %s =\n", node->data.assignment.name);
            ast_print(node->data.assignment.value, indent + 1);
            break;
        case AST_VAR_DECL:
            if (node->data.var_decl.array_size) {
                printf("Array Declaration: %s %s[...]\n", 
                       node->data.var_decl.type, node->data.var_decl.name);
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Size:\n");
                ast_print(node->data.var_decl.array_size, indent + 2);
            } else {
                printf("Variable Declaration: %s %s\n", 
                       node->data.var_decl.type, node->data.var_decl.name);
            }
            if (node->data.var_decl.initializer) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Initializer:\n");
                ast_print(node->data.var_decl.initializer, indent + 2);
            }
            break;
        case AST_EXPR_STMT:
            printf("Expression Statement\n");
            ast_print(node->data.expr_stmt.expression, indent + 1);
            break;
        case AST_IF_STMT:
            printf("If Statement\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Condition:\n");
            ast_print(node->data.if_stmt.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Then:\n");
            ast_print(node->data.if_stmt.then_stmt, indent + 2);
            if (node->data.if_stmt.else_stmt) {
                for (int i = 0; i < indent + 1; i++) printf("  ");
                printf("Else:\n");
                ast_print(node->data.if_stmt.else_stmt, indent + 2);
            }
            break;
        case AST_WHILE_STMT:
            printf("While Statement\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Condition:\n");
            ast_print(node->data.while_stmt.condition, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Body:\n");
            ast_print(node->data.while_stmt.body, indent + 2);
            break;
        case AST_FUNCTION_CALL:
            printf("Function Call: %s(", node->data.function_call.name);
            for (int i = 0; i < node->data.function_call.argument_count; i++) {
                if (i > 0) printf(", ");
                printf("arg%d", i);
            }
            printf(")\n");
            for (int i = 0; i < node->data.function_call.argument_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("Arg %d:\n", i);
                ast_print(node->data.function_call.arguments[i], indent + 2);
            }
            break;
        case AST_PARAM_DECL:
            printf("Parameter: %s %s\n", node->data.param_decl.type, node->data.param_decl.name);
            break;
        case AST_ARRAY_ACCESS:
            printf("Array Access\n");
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Array:\n");
            ast_print(node->data.array_access.array, indent + 2);
            for (int i = 0; i < indent + 1; i++) printf("  ");
            printf("Index:\n");
            ast_print(node->data.array_access.index, indent + 2);
            break;
        case AST_CHAR_LITERAL:
            printf("Char: '%c' (%d)\n", node->data.char_literal.value, node->data.char_literal.value);
            break;
        case AST_ADDRESS_OF:
            printf("Address Of (&)\n");
            ast_print(node->data.unary_op.operand, indent + 1);
            break;
        case AST_DEREFERENCE:
            printf("Dereference (*)\n");
            ast_print(node->data.unary_op.operand, indent + 1);
            break;
        default:
            printf("Unknown node type: %d\n", node->type);
    }
}
