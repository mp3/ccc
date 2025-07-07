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

static char *process_escape_sequences(const char *str) {
    char *result = malloc(strlen(str) + 1);
    int i = 0, j = 0;
    
    while (str[i]) {
        if (str[i] == '\\' && str[i+1]) {
            i++; // skip backslash
            switch (str[i]) {
                case 'n': result[j++] = '\n'; break;
                case 't': result[j++] = '\t'; break;
                case 'r': result[j++] = '\r'; break;
                case '\\': result[j++] = '\\'; break;
                case '"': result[j++] = '"'; break;
                case '\'': result[j++] = '\''; break;
                case '0': result[j++] = '\0'; break;
                default: 
                    result[j++] = '\\';
                    result[j++] = str[i];
            }
            i++;
        } else {
            result[j++] = str[i++];
        }
    }
    result[j] = '\0';
    return result;
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
static ASTNode *parse_postfix(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_statement(Parser *parser);

static ASTNode *parse_primary(Parser *parser) {
    Token *token = parser->current_token;
    LOG_TRACE("parse_primary called with token: %s at %d:%d", 
              token_type_to_string(token->type),
              token->line,
              token->column);
    
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
    
    // Handle logical NOT operator
    if (token->type == TOKEN_NOT) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser);
        ASTNode *operand = parse_primary(parser);
        ASTNode *node = create_ast_node(AST_UNARY_OP, line, column);
        node->data.unary_op.op = TOKEN_NOT;
        node->data.unary_op.operand = operand;
        LOG_TRACE("Parsed logical NOT operator");
        return node;
    }
    
    // Handle bitwise NOT operator
    if (token->type == TOKEN_TILDE) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser);
        ASTNode *operand = parse_primary(parser);
        ASTNode *node = create_ast_node(AST_UNARY_OP, line, column);
        node->data.unary_op.op = TOKEN_TILDE;
        node->data.unary_op.operand = operand;
        LOG_TRACE("Parsed bitwise NOT operator");
        return node;
    }
    
    // Handle prefix increment operator
    if (token->type == TOKEN_INCREMENT) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser);
        ASTNode *operand = parse_postfix(parser);
        ASTNode *node = create_ast_node(AST_UNARY_OP, line, column);
        node->data.unary_op.op = TOKEN_INCREMENT;
        node->data.unary_op.operand = operand;
        node->data.unary_op.is_prefix = true;
        LOG_TRACE("Parsed prefix increment operator");
        return node;
    }
    
    // Handle prefix decrement operator
    if (token->type == TOKEN_DECREMENT) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser);
        ASTNode *operand = parse_postfix(parser);
        ASTNode *node = create_ast_node(AST_UNARY_OP, line, column);
        node->data.unary_op.op = TOKEN_DECREMENT;
        node->data.unary_op.operand = operand;
        node->data.unary_op.is_prefix = true;
        LOG_TRACE("Parsed prefix decrement operator");
        return node;
    }
    
    // Handle sizeof operator
    if (token->type == TOKEN_KEYWORD_SIZEOF) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser);
        parser_expect(parser, TOKEN_LPAREN);
        
        ASTNode *node = create_ast_node(AST_SIZEOF, line, column);
        
        // Check if next token is a type keyword
        if (parser->current_token->type == TOKEN_KEYWORD_INT || 
            parser->current_token->type == TOKEN_KEYWORD_CHAR) {
            // sizeof(type)
            char *base_type = strdup(parser->current_token->type == TOKEN_KEYWORD_INT ? "int" : "char");
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
            free(base_type);
            
            node->data.sizeof_op.type_name = strdup(type_name);
            node->data.sizeof_op.expression = NULL;
        } else {
            // sizeof(expression)
            node->data.sizeof_op.type_name = NULL;
            node->data.sizeof_op.expression = parse_expression(parser);
        }
        
        parser_expect(parser, TOKEN_RPAREN);
        LOG_TRACE("Parsed sizeof operator");
        return node;
    }
    
    // Handle negative numbers
    if (token->type == TOKEN_MINUS && parser->peek_token->type == TOKEN_INT_LITERAL) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser); // consume '-'
        int value = -(parser->current_token->value.int_value);
        parser_advance(parser); // consume number
        ASTNode *node = create_ast_node(AST_INT_LITERAL, line, column);
        node->data.int_literal.value = value;
        LOG_TRACE("Parsed negative int literal: %d", node->data.int_literal.value);
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
            char *unquoted = str + 1;
            // Process escape sequences
            node->data.string_literal.value = process_escape_sequences(unquoted);
            free(str);
        } else {
            node->data.string_literal.value = process_escape_sequences(str);
            free(str);
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
        } else if (parser->current_token->type == TOKEN_DOT) {
            // Member access
            parser_advance(parser); // consume '.'
            
            Token *member_token = parser->current_token;
            char *member_name = strdup(member_token->text);
            parser_expect(parser, TOKEN_IDENTIFIER);
            
            ASTNode *object_node = create_ast_node(AST_IDENTIFIER, line, column);
            object_node->data.identifier.name = name;
            
            ASTNode *node = create_ast_node(AST_MEMBER_ACCESS, line, column);
            node->data.member_access.object = object_node;
            node->data.member_access.member_name = member_name;
            LOG_TRACE("Parsed member access: %s.%s", name, member_name);
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
        // Check if this is a type cast or a parenthesized expression
        if (parser->current_token->type == TOKEN_KEYWORD_INT || 
            parser->current_token->type == TOKEN_KEYWORD_CHAR ||
            parser->current_token->type == TOKEN_KEYWORD_STRUCT) {
            // This is a type cast
            int line = parser->peek_token->line;
            int column = parser->peek_token->column;
            
            // Parse the type
            char *base_type = NULL;
            if (parser->current_token->type == TOKEN_KEYWORD_INT) {
                base_type = strdup("int");
                parser_advance(parser);
            } else if (parser->current_token->type == TOKEN_KEYWORD_CHAR) {
                base_type = strdup("char");
                parser_advance(parser);
            } else if (parser->current_token->type == TOKEN_KEYWORD_STRUCT) {
                parser_advance(parser);
                if (parser->current_token->type != TOKEN_IDENTIFIER) {
                    LOG_ERROR("Expected struct name after 'struct' in cast");
                    exit(1);
                }
                char struct_type[256];
                sprintf(struct_type, "struct %s", parser->current_token->text);
                base_type = strdup(struct_type);
                parser_advance(parser);
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
            
            parser_expect(parser, TOKEN_RPAREN);
            
            // Parse the expression to cast
            ASTNode *expr = parse_primary(parser);
            
            // Create the cast node
            ASTNode *node = create_ast_node(AST_CAST, line, column);
            node->data.cast.target_type = strdup(type_name);
            node->data.cast.expression = expr;
            LOG_TRACE("Parsed type cast to: %s", type_name);
            return node;
        } else {
            // This is a parenthesized expression
            ASTNode *expr = parse_expression(parser);
            parser_expect(parser, TOKEN_RPAREN);
            return expr;
        }
    }
    
    LOG_ERROR("Unexpected token in primary expression: %s at %d:%d",
              token_type_to_string(token->type), token->line, token->column);
    exit(1);
}

static ASTNode *parse_postfix(Parser *parser) {
    ASTNode *expr = parse_primary(parser);
    
    // Handle postfix operators
    while (true) {
        if (parser->current_token->type == TOKEN_INCREMENT || 
            parser->current_token->type == TOKEN_DECREMENT) {
            Token *op_token = parser->current_token;
            TokenType op_type = op_token->type;
            int op_line = op_token->line;
            int op_column = op_token->column;
            parser_advance(parser);
            
            ASTNode *node = create_ast_node(AST_UNARY_OP, op_line, op_column);
            node->data.unary_op.op = op_type;
            node->data.unary_op.operand = expr;
            node->data.unary_op.is_prefix = false;  // postfix
            LOG_TRACE("Parsed postfix %s operator", 
                     op_type == TOKEN_INCREMENT ? "increment" : "decrement");
            expr = node;
        } else {
            break;
        }
    }
    
    return expr;
}

static ASTNode *parse_multiplicative(Parser *parser) {
    LOG_TRACE("parse_multiplicative called");
    ASTNode *left = parse_postfix(parser);
    
    while (parser->current_token->type == TOKEN_STAR ||
           parser->current_token->type == TOKEN_SLASH) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_postfix(parser);
        
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

static ASTNode *parse_shift(Parser *parser) {
    ASTNode *left = parse_additive(parser);
    
    while (parser->current_token->type == TOKEN_LSHIFT ||
           parser->current_token->type == TOKEN_RSHIFT) {
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

static ASTNode *parse_comparison(Parser *parser) {
    ASTNode *left = parse_shift(parser);
    
    while (parser->current_token->type >= TOKEN_EQ &&
           parser->current_token->type <= TOKEN_GE) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_shift(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_bitwise_or(Parser *parser);
static ASTNode *parse_bitwise_xor(Parser *parser);
static ASTNode *parse_bitwise_and(Parser *parser);

static ASTNode *parse_bitwise_and(Parser *parser) {
    LOG_TRACE("parse_bitwise_and called, current token: %s",
              token_type_to_string(parser->current_token->type));
    ASTNode *left = parse_comparison(parser);
    
    while (parser->current_token->type == TOKEN_AMPERSAND) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_comparison(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_bitwise_xor(Parser *parser) {
    ASTNode *left = parse_bitwise_and(parser);
    
    while (parser->current_token->type == TOKEN_CARET) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_bitwise_and(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_bitwise_or(Parser *parser) {
    ASTNode *left = parse_bitwise_xor(parser);
    
    while (parser->current_token->type == TOKEN_PIPE) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_bitwise_xor(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_logical_and(Parser *parser) {
    ASTNode *left = parse_bitwise_or(parser);
    
    while (parser->current_token->type == TOKEN_AND) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_bitwise_or(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_logical_or(Parser *parser) {
    ASTNode *left = parse_logical_and(parser);
    
    while (parser->current_token->type == TOKEN_OR) {
        Token *op_token = parser->current_token;
        TokenType op_type = op_token->type;
        int op_line = op_token->line;
        int op_column = op_token->column;
        parser_advance(parser);
        ASTNode *right = parse_logical_and(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_line, op_column);
        node->data.binary_op.op = op_type;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

// Forward declaration for ast_clone (defined at end of file)

static ASTNode *parse_ternary(Parser *parser) {
    ASTNode *condition = parse_logical_or(parser);
    
    if (parser->current_token->type == TOKEN_QUESTION) {
        int line = parser->current_token->line;
        int column = parser->current_token->column;
        parser_advance(parser); // consume '?'
        
        ASTNode *true_expr = parse_expression(parser);
        parser_expect(parser, TOKEN_COLON);
        ASTNode *false_expr = parse_ternary(parser); // Right associative
        
        ASTNode *node = create_ast_node(AST_TERNARY, line, column);
        node->data.ternary.condition = condition;
        node->data.ternary.true_expr = true_expr;
        node->data.ternary.false_expr = false_expr;
        
        LOG_TRACE("Parsed ternary operator");
        return node;
    }
    
    return condition;
}

static ASTNode *parse_assignment(Parser *parser) {
    ASTNode *left = parse_ternary(parser);
    
    TokenType op_type = parser->current_token->type;
    if (op_type == TOKEN_ASSIGN || 
        op_type == TOKEN_PLUS_ASSIGN ||
        op_type == TOKEN_MINUS_ASSIGN ||
        op_type == TOKEN_STAR_ASSIGN ||
        op_type == TOKEN_SLASH_ASSIGN) {
        
        if (left->type != AST_IDENTIFIER && left->type != AST_ARRAY_ACCESS && left->type != AST_DEREFERENCE) {
            LOG_ERROR("Invalid assignment target at %d:%d", 
                     left->line, left->column);
            exit(1);
        }
        
        Token *op_token = parser->current_token;
        parser_advance(parser);
        ASTNode *right = parse_assignment(parser);  // Right associative
        
        // For compound assignments, expand to: a = a op b
        if (op_type != TOKEN_ASSIGN) {
            // Create the binary operation
            ASTNode *binary_op = create_ast_node(AST_BINARY_OP, op_token->line, op_token->column);
            
            // Determine the base operator
            TokenType base_op;
            switch (op_type) {
                case TOKEN_PLUS_ASSIGN: base_op = TOKEN_PLUS; break;
                case TOKEN_MINUS_ASSIGN: base_op = TOKEN_MINUS; break;
                case TOKEN_STAR_ASSIGN: base_op = TOKEN_STAR; break;
                case TOKEN_SLASH_ASSIGN: base_op = TOKEN_SLASH; break;
                default: base_op = TOKEN_UNKNOWN; // Should never happen
            }
            
            binary_op->data.binary_op.op = base_op;
            // We need to clone the left side for the operation
            binary_op->data.binary_op.left = ast_clone(left);
            binary_op->data.binary_op.right = right;
            right = binary_op;
        }
        
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

static ASTNode *parse_comma(Parser *parser) {
    LOG_TRACE("parse_comma called, current token: %s",
              token_type_to_string(parser->current_token->type));
    
    ASTNode *left = parse_assignment(parser);
    
    while (parser->current_token->type == TOKEN_COMMA) {
        Token *op_token = parser->current_token;
        parser_advance(parser); // consume comma
        
        ASTNode *right = parse_assignment(parser);
        
        ASTNode *node = create_ast_node(AST_BINARY_OP, op_token->line, op_token->column);
        node->data.binary_op.op = TOKEN_COMMA;
        node->data.binary_op.left = left;
        node->data.binary_op.right = right;
        left = node;
    }
    
    return left;
}

static ASTNode *parse_expression(Parser *parser) {
    LOG_TRACE("parse_expression called, current token: %s",
              token_type_to_string(parser->current_token->type));
    return parse_comma(parser);
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
    LOG_TRACE("parse_statement called with token: %s at %d:%d", 
              token_type_to_string(token->type),
              token->line,
              token->column);
    
    // Struct declaration
    if (token->type == TOKEN_KEYWORD_STRUCT) {
        parser_advance(parser); // consume 'struct'
        
        Token *name_token = parser->current_token;
        char *struct_name = strdup(name_token->text);
        parser_expect(parser, TOKEN_IDENTIFIER);
        parser_expect(parser, TOKEN_LBRACE);
        
        // Parse struct members
        int capacity = 8;
        ASTNode **members = malloc(capacity * sizeof(ASTNode*));
        int member_count = 0;
        
        while (parser->current_token->type != TOKEN_RBRACE) {
            if (member_count >= capacity) {
                capacity *= 2;
                members = realloc(members, capacity * sizeof(ASTNode*));
            }
            
            // Parse member declaration (similar to variable declaration)
            if (parser->current_token->type == TOKEN_KEYWORD_INT || 
                parser->current_token->type == TOKEN_KEYWORD_CHAR) {
                
                char *member_type = strdup(parser->current_token->type == TOKEN_KEYWORD_INT ? "int" : "char");
                parser_advance(parser);
                
                // Check for pointer type
                int pointer_count = 0;
                while (parser->current_token->type == TOKEN_STAR) {
                    pointer_count++;
                    parser_advance(parser);
                }
                
                // Build the complete type string  
                char type_name[256];
                strcpy(type_name, member_type);
                for (int i = 0; i < pointer_count; i++) {
                    strcat(type_name, "*");
                }
                
                Token *member_name_token = parser->current_token;
                char *member_name = strdup(member_name_token->text);
                parser_expect(parser, TOKEN_IDENTIFIER);
                parser_expect(parser, TOKEN_SEMICOLON);
                
                // Create member declaration node
                ASTNode *member = create_ast_node(AST_VAR_DECL, member_name_token->line, member_name_token->column);
                member->data.var_decl.type = strdup(type_name);
                member->data.var_decl.name = member_name;
                member->data.var_decl.initializer = NULL;
                member->data.var_decl.array_size = NULL;
                
                members[member_count++] = member;
                free(member_type);
            } else {
                LOG_ERROR("Expected type in struct member declaration");
                exit(1);
            }
        }
        
        parser_expect(parser, TOKEN_RBRACE);
        parser_expect(parser, TOKEN_SEMICOLON);
        
        // Create struct declaration node
        ASTNode *struct_node = create_ast_node(AST_STRUCT_DECL, name_token->line, name_token->column);
        struct_node->data.struct_decl.name = struct_name;
        struct_node->data.struct_decl.members = members;
        struct_node->data.struct_decl.member_count = member_count;
        
        return struct_node;
    }
    
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
    
    if (token->type == TOKEN_KEYWORD_DO) {
        parser_advance(parser);
        
        ASTNode *body = parse_statement(parser);
        
        parser_expect(parser, TOKEN_KEYWORD_WHILE);
        parser_expect(parser, TOKEN_LPAREN);
        ASTNode *condition = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        parser_expect(parser, TOKEN_SEMICOLON);
        
        ASTNode *node = create_ast_node(AST_DO_WHILE_STMT, token->line, token->column);
        node->data.do_while_stmt.body = body;
        node->data.do_while_stmt.condition = condition;
        LOG_TRACE("Parsed do-while statement");
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
    
    if (token->type == TOKEN_KEYWORD_FOR) {
        parser_advance(parser);
        parser_expect(parser, TOKEN_LPAREN);
        
        // Parse init (can be a variable declaration or expression)
        ASTNode *init = NULL;
        if (parser->current_token->type != TOKEN_SEMICOLON) {
            if (parser->current_token->type == TOKEN_KEYWORD_INT || 
                parser->current_token->type == TOKEN_KEYWORD_CHAR) {
                // Variable declaration as init
                init = parse_statement(parser);
                // The parse_statement will consume the semicolon for var decl
            } else {
                // Expression as init
                init = parse_expression(parser);
                parser_expect(parser, TOKEN_SEMICOLON);
            }
        } else {
            parser_advance(parser); // consume semicolon
        }
        
        // Parse condition (optional)
        ASTNode *condition = NULL;
        if (parser->current_token->type != TOKEN_SEMICOLON) {
            condition = parse_expression(parser);
        }
        parser_expect(parser, TOKEN_SEMICOLON);
        
        // Parse update (optional)
        ASTNode *update = NULL;
        if (parser->current_token->type != TOKEN_RPAREN) {
            update = parse_expression(parser);
        }
        parser_expect(parser, TOKEN_RPAREN);
        
        // Parse body
        ASTNode *body = parse_statement(parser);
        
        ASTNode *node = create_ast_node(AST_FOR_STMT, token->line, token->column);
        node->data.for_stmt.init = init;
        node->data.for_stmt.condition = condition;
        node->data.for_stmt.update = update;
        node->data.for_stmt.body = body;
        LOG_TRACE("Parsed for statement");
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
    
    if (token->type == TOKEN_KEYWORD_BREAK) {
        parser_advance(parser);
        ASTNode *node = create_ast_node(AST_BREAK_STMT, token->line, token->column);
        parser_expect(parser, TOKEN_SEMICOLON);
        LOG_TRACE("Parsed break statement");
        return node;
    }
    
    if (token->type == TOKEN_KEYWORD_CONTINUE) {
        parser_advance(parser);
        ASTNode *node = create_ast_node(AST_CONTINUE_STMT, token->line, token->column);
        parser_expect(parser, TOKEN_SEMICOLON);
        LOG_TRACE("Parsed continue statement");
        return node;
    }
    
    if (token->type == TOKEN_KEYWORD_SWITCH) {
        parser_advance(parser);
        parser_expect(parser, TOKEN_LPAREN);
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        parser_expect(parser, TOKEN_LBRACE);
        
        ASTNode *node = create_ast_node(AST_SWITCH_STMT, token->line, token->column);
        node->data.switch_stmt.expression = expr;
        node->data.switch_stmt.cases = NULL;
        node->data.switch_stmt.case_count = 0;
        node->data.switch_stmt.default_case = NULL;
        
        // Parse cases
        int case_capacity = 10;
        node->data.switch_stmt.cases = malloc(case_capacity * sizeof(ASTNode*));
        
        while (parser->current_token->type != TOKEN_RBRACE &&
               parser->current_token->type != TOKEN_EOF) {
            
            if (parser->current_token->type == TOKEN_KEYWORD_CASE) {
                int case_line = parser->current_token->line;
                int case_column = parser->current_token->column;
                parser_advance(parser);
                
                // Parse case value (must be constant)
                ASTNode *case_value = parse_primary(parser);
                parser_expect(parser, TOKEN_COLON);
                
                // Create case node
                ASTNode *case_node = create_ast_node(AST_CASE_STMT, case_line, case_column);
                case_node->data.case_stmt.value = case_value;
                case_node->data.case_stmt.statements = NULL;
                case_node->data.case_stmt.statement_count = 0;
                
                // Parse statements until next case/default/closing brace
                int stmt_capacity = 10;
                case_node->data.case_stmt.statements = malloc(stmt_capacity * sizeof(ASTNode*));
                
                while (parser->current_token->type != TOKEN_KEYWORD_CASE &&
                       parser->current_token->type != TOKEN_KEYWORD_DEFAULT &&
                       parser->current_token->type != TOKEN_RBRACE &&
                       parser->current_token->type != TOKEN_EOF) {
                    
                    if (case_node->data.case_stmt.statement_count >= stmt_capacity) {
                        stmt_capacity *= 2;
                        case_node->data.case_stmt.statements = realloc(
                            case_node->data.case_stmt.statements,
                            stmt_capacity * sizeof(ASTNode*)
                        );
                    }
                    
                    case_node->data.case_stmt.statements[case_node->data.case_stmt.statement_count++] = 
                        parse_statement(parser);
                }
                
                // Add case to switch
                if (node->data.switch_stmt.case_count >= case_capacity) {
                    case_capacity *= 2;
                    node->data.switch_stmt.cases = realloc(
                        node->data.switch_stmt.cases,
                        case_capacity * sizeof(ASTNode*)
                    );
                }
                node->data.switch_stmt.cases[node->data.switch_stmt.case_count++] = case_node;
                
            } else if (parser->current_token->type == TOKEN_KEYWORD_DEFAULT) {
                int default_line = parser->current_token->line;
                int default_column = parser->current_token->column;
                parser_advance(parser);
                parser_expect(parser, TOKEN_COLON);
                
                // Create default node
                ASTNode *default_node = create_ast_node(AST_DEFAULT_STMT, default_line, default_column);
                default_node->data.default_stmt.statements = NULL;
                default_node->data.default_stmt.statement_count = 0;
                
                // Parse statements
                int stmt_capacity = 10;
                default_node->data.default_stmt.statements = malloc(stmt_capacity * sizeof(ASTNode*));
                
                while (parser->current_token->type != TOKEN_KEYWORD_CASE &&
                       parser->current_token->type != TOKEN_KEYWORD_DEFAULT &&
                       parser->current_token->type != TOKEN_RBRACE &&
                       parser->current_token->type != TOKEN_EOF) {
                    
                    if (default_node->data.default_stmt.statement_count >= stmt_capacity) {
                        stmt_capacity *= 2;
                        default_node->data.default_stmt.statements = realloc(
                            default_node->data.default_stmt.statements,
                            stmt_capacity * sizeof(ASTNode*)
                        );
                    }
                    
                    default_node->data.default_stmt.statements[default_node->data.default_stmt.statement_count++] = 
                        parse_statement(parser);
                }
                
                node->data.switch_stmt.default_case = default_node;
                
            } else {
                LOG_ERROR("Expected 'case' or 'default' in switch statement at %d:%d",
                         parser->current_token->line, parser->current_token->column);
                exit(1);
            }
        }
        
        parser_expect(parser, TOKEN_RBRACE);
        LOG_TRACE("Parsed switch statement with %d cases", node->data.switch_stmt.case_count);
        return node;
    }
    
    if (token->type == TOKEN_LBRACE) {
        return parse_compound_statement(parser);
    }
    
    // Expression statement
    ASTNode *node = create_ast_node(AST_EXPR_STMT, token->line, token->column);
    node->data.expr_stmt.expression = parse_expression(parser);
    LOG_TRACE("After parsing expression statement, current token: %s at %d:%d", 
              token_type_to_string(parser->current_token->type),
              parser->current_token->line,
              parser->current_token->column);
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

static bool is_function_declaration(Parser *parser) {
    // Simplified approach: try to parse as function first, if it fails, parse as variable
    // This is based on the observation that functions have '(' after the identifier
    // while variables have ';' or '=' after the identifier
    
    // We'll implement this by attempting to look for specific patterns
    // For a proper implementation, we'd need better lookahead infrastructure
    
    // Simple heuristic: if line contains '(' before ';', it's likely a function
    // For now, let's default to the previous behavior and fix this incrementally
    
    return true;  // Start by defaulting to function parsing
}

static ASTNode *parse_global_variable(Parser *parser) {
    // Parse global variable declaration (similar to local but at global scope)
    char *base_type = strdup(parser->current_token->type == TOKEN_KEYWORD_INT ? "int" : "char");
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
    free(base_type);
    
    Token *name_token = parser->current_token;
    char *var_name = strdup(name_token->text);
    int var_line = name_token->line;
    int var_column = name_token->column;
    parser_expect(parser, TOKEN_IDENTIFIER);
    
    ASTNode *initializer = NULL;
    if (parser->current_token->type == TOKEN_ASSIGN) {
        parser_advance(parser); // consume '='
        initializer = parse_expression(parser);
    }
    
    parser_expect(parser, TOKEN_SEMICOLON);
    
    ASTNode *var_decl = create_ast_node(AST_VAR_DECL, var_line, var_column);
    var_decl->data.var_decl.type = strdup(type_name);
    var_decl->data.var_decl.name = var_name;
    var_decl->data.var_decl.initializer = initializer;
    var_decl->data.var_decl.array_size = NULL;
    
    return var_decl;
}

ASTNode *parser_parse(Parser *parser) {
    ASTNode *program = create_ast_node(AST_PROGRAM, 1, 1);
    
    int func_capacity = 8;
    int var_capacity = 8;
    program->data.program.functions = malloc(func_capacity * sizeof(ASTNode*));
    program->data.program.function_count = 0;
    program->data.program.global_vars = malloc(var_capacity * sizeof(ASTNode*));
    program->data.program.global_var_count = 0;
    
    while (parser->current_token->type != TOKEN_EOF) {
        // For now, revert to function-only parsing to fix the immediate issue
        // We'll implement global variables in a follow-up step
        if (program->data.program.function_count >= func_capacity) {
            func_capacity *= 2;
            program->data.program.functions = realloc(program->data.program.functions,
                                                     func_capacity * sizeof(ASTNode*));
        }
        program->data.program.functions[program->data.program.function_count++] = 
            parse_function(parser);
    }
    
    LOG_INFO("Parsed program with %d functions and %d global variables", 
             program->data.program.function_count, program->data.program.global_var_count);
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
            for (int i = 0; i < node->data.program.global_var_count; i++) {
                ast_destroy(node->data.program.global_vars[i]);
            }
            free(node->data.program.global_vars);
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
        case AST_BREAK_STMT:
        case AST_CONTINUE_STMT:
            // No dynamic memory to free
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
        case AST_DO_WHILE_STMT:
            ast_destroy(node->data.do_while_stmt.body);
            ast_destroy(node->data.do_while_stmt.condition);
            break;
        case AST_FOR_STMT:
            ast_destroy(node->data.for_stmt.init);
            ast_destroy(node->data.for_stmt.condition);
            ast_destroy(node->data.for_stmt.update);
            ast_destroy(node->data.for_stmt.body);
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
        case AST_STRUCT_DECL:
            free(node->data.struct_decl.name);
            for (int i = 0; i < node->data.struct_decl.member_count; i++) {
                ast_destroy(node->data.struct_decl.members[i]);
            }
            free(node->data.struct_decl.members);
            break;
        case AST_MEMBER_ACCESS:
            ast_destroy(node->data.member_access.object);
            free(node->data.member_access.member_name);
            break;
        case AST_SIZEOF:
            free(node->data.sizeof_op.type_name);
            ast_destroy(node->data.sizeof_op.expression);
            break;
        case AST_SWITCH_STMT:
            ast_destroy(node->data.switch_stmt.expression);
            for (int i = 0; i < node->data.switch_stmt.case_count; i++) {
                ast_destroy(node->data.switch_stmt.cases[i]);
            }
            free(node->data.switch_stmt.cases);
            ast_destroy(node->data.switch_stmt.default_case);
            break;
        case AST_CASE_STMT:
            ast_destroy(node->data.case_stmt.value);
            for (int i = 0; i < node->data.case_stmt.statement_count; i++) {
                ast_destroy(node->data.case_stmt.statements[i]);
            }
            free(node->data.case_stmt.statements);
            break;
        case AST_DEFAULT_STMT:
            for (int i = 0; i < node->data.default_stmt.statement_count; i++) {
                ast_destroy(node->data.default_stmt.statements[i]);
            }
            free(node->data.default_stmt.statements);
            break;
        case AST_TERNARY:
            ast_destroy(node->data.ternary.condition);
            ast_destroy(node->data.ternary.true_expr);
            ast_destroy(node->data.ternary.false_expr);
            break;
        case AST_CAST:
            free(node->data.cast.target_type);
            ast_destroy(node->data.cast.expression);
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

// Clone an AST node (deep copy)
ASTNode *ast_clone(ASTNode *node) {
    if (!node) return NULL;
    
    ASTNode *clone = create_ast_node(node->type, node->line, node->column);
    
    switch (node->type) {
        case AST_IDENTIFIER:
            clone->data.identifier.name = strdup(node->data.identifier.name);
            break;
        case AST_ARRAY_ACCESS:
            clone->data.array_access.array = ast_clone(node->data.array_access.array);
            clone->data.array_access.index = ast_clone(node->data.array_access.index);
            break;
        case AST_DEREFERENCE:
            clone->data.unary_op.operand = ast_clone(node->data.unary_op.operand);
            break;
        case AST_UNARY_OP:
            clone->data.unary_op.op = node->data.unary_op.op;
            clone->data.unary_op.operand = ast_clone(node->data.unary_op.operand);
            clone->data.unary_op.is_prefix = node->data.unary_op.is_prefix;
            break;
        case AST_INT_LITERAL:
            clone->data.int_literal.value = node->data.int_literal.value;
            break;
        case AST_MEMBER_ACCESS:
            clone->data.member_access.object = ast_clone(node->data.member_access.object);
            clone->data.member_access.member_name = strdup(node->data.member_access.member_name);
            break;
        case AST_TERNARY:
            clone->data.ternary.condition = ast_clone(node->data.ternary.condition);
            clone->data.ternary.true_expr = ast_clone(node->data.ternary.true_expr);
            clone->data.ternary.false_expr = ast_clone(node->data.ternary.false_expr);
            break;
        case AST_CAST:
            clone->data.cast.target_type = strdup(node->data.cast.target_type);
            clone->data.cast.expression = ast_clone(node->data.cast.expression);
            break;
        default:
            LOG_ERROR("ast_clone not implemented for node type: %d", node->type);
            exit(1);
    }
    
    return clone;
}
