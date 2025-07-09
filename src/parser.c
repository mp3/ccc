#include "parser.h"
#include "logger.h"
#include "symtab.h"
#include "error.h"
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
        ErrorContext ctx = error_context_from_token(parser->filename, parser->current_token);
        error_syntax(parser->error_manager, &ctx,
                    token_type_to_string(type),
                    token_type_to_string(parser->current_token->type));
        parser->had_error = true;
        
        // Try to recover for common cases
        if (type == TOKEN_SEMICOLON) {
            // Check if next token suggests a new statement
            TokenType next = parser->current_token->type;
            if (next == TOKEN_RBRACE ||              // End of block
                next == TOKEN_KEYWORD_INT || next == TOKEN_KEYWORD_CHAR ||  // Type declarations
                next == TOKEN_KEYWORD_IF || next == TOKEN_KEYWORD_WHILE || next == TOKEN_KEYWORD_FOR ||   // Control flow
                next == TOKEN_KEYWORD_RETURN || next == TOKEN_KEYWORD_BREAK || next == TOKEN_KEYWORD_CONTINUE ||
                next == TOKEN_EOF) {
                LOG_DEBUG("Recovering from missing semicolon before %s", 
                         token_type_to_string(next));
                return;
            }
        }
        
        // For now, still exit until we have full error recovery
        error_print_all(parser->error_manager);
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
    Parser *parser = calloc(1, sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    parser->peek_token = lexer_next_token(lexer);
    parser->error_manager = error_manager_create();
    parser->filename = lexer->filename;
    parser->had_error = false;
    // Initialize typedef tracking
    parser->typedef_capacity = 16;
    parser->typedef_count = 0;
    parser->typedef_names = malloc(parser->typedef_capacity * sizeof(char*));
    
    // Initialize symbol table
    parser->symtab = symtab_create(NULL);
    
    LOG_DEBUG("Created parser with error manager");
    return parser;
}

void parser_destroy(Parser *parser) {
    if (parser) {
        token_destroy(parser->current_token);
        token_destroy(parser->peek_token);
        if (parser->error_manager) {
            error_manager_destroy(parser->error_manager);
        }
        if (parser->typedef_names) {
            for (int i = 0; i < parser->typedef_count; i++) {
                free(parser->typedef_names[i]);
            }
            free(parser->typedef_names);
        }
        if (parser->symtab) {
            symtab_destroy(parser->symtab);
        }
        free(parser);
    }
}

static ASTNode *parse_primary(Parser *parser);
static ASTNode *parse_postfix(Parser *parser);
static ASTNode *parse_assignment(Parser *parser);
static ASTNode *parse_expression(Parser *parser);
// parse_statement is declared in parser.h
static char *parse_type(Parser *parser, char **identifier);
static ASTNode *parse_struct_declaration(Parser *parser, char *struct_name);

static bool is_typedef_name(Parser *parser, const char *name) {
    for (int i = 0; i < parser->typedef_count; i++) {
        if (strcmp(parser->typedef_names[i], name) == 0) {
            return true;
        }
    }
    return false;
}

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
    
    // Handle negative floats
    if (token->type == TOKEN_MINUS && parser->peek_token->type == TOKEN_FLOAT_LITERAL) {
        int line = token->line;
        int column = token->column;
        parser_advance(parser); // consume '-'
        double value = -(parser->current_token->value.float_value);
        parser_advance(parser); // consume number
        ASTNode *node = create_ast_node(AST_FLOAT_LITERAL, line, column);
        node->data.float_literal.value = value;
        LOG_TRACE("Parsed negative float literal: %f", node->data.float_literal.value);
        return node;
    }
    
    if (token->type == TOKEN_INT_LITERAL) {
        ASTNode *node = create_ast_node(AST_INT_LITERAL, token->line, token->column);
        node->data.int_literal.value = token->value.int_value;
        parser_advance(parser);
        LOG_TRACE("Parsed int literal: %d", node->data.int_literal.value);
        return node;
    }
    
    if (token->type == TOKEN_FLOAT_LITERAL) {
        ASTNode *node = create_ast_node(AST_FLOAT_LITERAL, token->line, token->column);
        node->data.float_literal.value = token->value.float_value;
        parser_advance(parser);
        LOG_TRACE("Parsed float literal: %f", node->data.float_literal.value);
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
                // Parse assignment expression (which stops at comma)
                arguments[arg_count++] = parse_assignment(parser);
                
                while (parser->current_token->type == TOKEN_COMMA) {
                    parser_advance(parser);
                    if (arg_count >= arg_capacity) {
                        arg_capacity *= 2;
                        arguments = realloc(arguments, arg_capacity * sizeof(ASTNode*));
                    }
                    arguments[arg_count++] = parse_assignment(parser);
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
        // Save position to potentially backtrack
        int line = token->line;
        int column = token->column;
        
        // Check if this might be a cast by looking for a type keyword
        bool could_be_cast = false;
        if (parser->current_token->type == TOKEN_KEYWORD_INT ||
            parser->current_token->type == TOKEN_KEYWORD_CHAR ||
            parser->current_token->type == TOKEN_KEYWORD_FLOAT ||
            parser->current_token->type == TOKEN_KEYWORD_DOUBLE ||
            parser->current_token->type == TOKEN_KEYWORD_VOID ||
            parser->current_token->type == TOKEN_KEYWORD_STRUCT) {
            could_be_cast = true;
        } else if (parser->current_token->type == TOKEN_IDENTIFIER) {
            // Could be a typedef'd type, but for now we'll be conservative
            // and only treat it as a cast if followed immediately by ) or *
            if (parser->peek_token->type == TOKEN_RPAREN ||
                parser->peek_token->type == TOKEN_STAR) {
                could_be_cast = true;
            }
        }
        
        if (could_be_cast) {
            // Try to parse a type
            char *type_name = parse_type(parser, NULL);
            
            if (type_name && parser->current_token->type == TOKEN_RPAREN) {
                // This looks like a cast
                parser_advance(parser); // consume ')'
                
                // Parse the expression to cast
                ASTNode *expr = parse_primary(parser);
                
                // Create the cast node
                ASTNode *node = create_ast_node(AST_CAST, line, column);
                node->data.cast.target_type = type_name;
                node->data.cast.expression = expr;
                LOG_TRACE("Parsed type cast to: %s", type_name);
                return node;
            } else {
                // Not a valid cast
                if (type_name) {
                    free(type_name);
                }
                LOG_ERROR("Invalid cast syntax at %d:%d", line, column);
                exit(1);
            }
        } else {
            // Definitely not a cast, parse as parenthesized expression
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
           parser->current_token->type == TOKEN_SLASH ||
           parser->current_token->type == TOKEN_PERCENT) {
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
    // LOG_TRACE("parse_assignment: after parse_ternary, current token: %s at %d:%d",
    //           token_type_to_string(op_type), 
    //           parser->current_token->line, 
    //           parser->current_token->column);
    
    if (op_type == TOKEN_ASSIGN || 
        op_type == TOKEN_PLUS_ASSIGN ||
        op_type == TOKEN_MINUS_ASSIGN ||
        op_type == TOKEN_STAR_ASSIGN ||
        op_type == TOKEN_SLASH_ASSIGN) {
        
        if (left->type != AST_IDENTIFIER && left->type != AST_ARRAY_ACCESS && 
            left->type != AST_DEREFERENCE && left->type != AST_MEMBER_ACCESS) {
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
            // LOG_DEBUG("Creating AST_ASSIGNMENT for %s", left->data.identifier.name);
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

static char *parse_type(Parser *parser, char **identifier) {
    LOG_TRACE("parse_type called, current token: %s at %d:%d",
              token_type_to_string(parser->current_token->type),
              parser->current_token->line,
              parser->current_token->column);
    
    // Check for const qualifier
    bool is_const = false;
    if (parser->current_token->type == TOKEN_KEYWORD_CONST) {
        is_const = true;
        parser_advance(parser);
    }
    
    // Parse base type
    char *base_type = NULL;
    if (parser->current_token->type == TOKEN_KEYWORD_INT) {
        base_type = strdup("int");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_CHAR) {
        base_type = strdup("char");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_FLOAT) {
        base_type = strdup("float");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_DOUBLE) {
        base_type = strdup("double");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_VOID) {
        base_type = strdup("void");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_STRUCT) {
        parser_advance(parser); // consume 'struct'
        if (parser->current_token->type != TOKEN_IDENTIFIER) {
            LOG_ERROR("Expected struct name after 'struct'");
            return NULL;
        }
        // Create type name as "struct StructName"
        base_type = malloc(strlen(parser->current_token->text) + 8);
        sprintf(base_type, "struct %s", parser->current_token->text);
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_ENUM) {
        parser_advance(parser); // consume 'enum'
        if (parser->current_token->type != TOKEN_IDENTIFIER) {
            LOG_ERROR("Expected enum name after 'enum'");
            return NULL;
        }
        // Create type name as "enum EnumName"
        base_type = malloc(strlen(parser->current_token->text) + 6);
        sprintf(base_type, "enum %s", parser->current_token->text);
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_IDENTIFIER) {
        // Check if it's a typedef'd type
        if (is_typedef_name(parser, parser->current_token->text)) {
            base_type = strdup(parser->current_token->text);
            parser_advance(parser);
        } else {
            // Not a type
            return NULL;
        }
    } else {
        return NULL;
    }
    
    LOG_TRACE("Base type: %s, next token: %s, peek: %s",
              base_type,
              token_type_to_string(parser->current_token->type),
              token_type_to_string(parser->peek_token->type));
    
    // Check for function pointer syntax: return_type (*
    if (parser->current_token->type == TOKEN_LPAREN &&
        parser->peek_token->type == TOKEN_STAR) {
        
        parser_advance(parser); // consume '('
        parser_advance(parser); // consume '*'
        
        // Get the identifier
        if (identifier && parser->current_token->type == TOKEN_IDENTIFIER) {
            *identifier = strdup(parser->current_token->text);
            parser_advance(parser);
        }
        
        parser_expect(parser, TOKEN_RPAREN);
        parser_expect(parser, TOKEN_LPAREN);
        
        // Parse parameter types
        char param_types[512] = "";
        bool first = true;
        
        while (parser->current_token->type != TOKEN_RPAREN &&
               parser->current_token->type != TOKEN_EOF) {
            
            if (!first) {
                parser_expect(parser, TOKEN_COMMA);
                strcat(param_types, ",");
            }
            first = false;
            
            // Parse parameter type
            char *param_type = NULL;
            if (parser->current_token->type == TOKEN_KEYWORD_INT) {
                param_type = "int";
                parser_advance(parser);
            } else if (parser->current_token->type == TOKEN_KEYWORD_CHAR) {
                param_type = "char";
                parser_advance(parser);
            } else {
                LOG_ERROR("Expected parameter type at %d:%d",
                         parser->current_token->line, parser->current_token->column);
                exit(1);
            }
            
            strcat(param_types, param_type);
            
            // Handle pointer parameters
            while (parser->current_token->type == TOKEN_STAR) {
                strcat(param_types, "*");
                parser_advance(parser);
            }
            
            // Skip parameter name if present
            if (parser->current_token->type == TOKEN_IDENTIFIER) {
                parser_advance(parser);
            }
        }
        
        parser_expect(parser, TOKEN_RPAREN);
        
        // Build function pointer type string
        char *func_ptr_type = malloc(512);
        if (is_const) {
            snprintf(func_ptr_type, 512, "const %s(*)(%s)", base_type, param_types);
        } else {
            snprintf(func_ptr_type, 512, "%s(*)(%s)", base_type, param_types);
        }
        free(base_type);
        return func_ptr_type;
    }
    
    // Handle regular pointers
    int pointer_count = 0;
    while (parser->current_token->type == TOKEN_STAR) {
        pointer_count++;
        parser_advance(parser);
    }
    
    // Get identifier if not a function pointer
    if (identifier && parser->current_token->type == TOKEN_IDENTIFIER) {
        *identifier = strdup(parser->current_token->text);
        parser_advance(parser);
    }
    
    // Build the complete type string
    char type_name[256];
    if (is_const) {
        strcpy(type_name, "const ");
        strcat(type_name, base_type);
    } else {
        strcpy(type_name, base_type);
    }
    for (int i = 0; i < pointer_count; i++) {
        strcat(type_name, "*");
    }
    free(base_type);
    
    return strdup(type_name);
}

static ASTNode *parse_compound_statement(Parser *parser) {
    LOG_TRACE("parse_compound_statement called");
    parser_expect(parser, TOKEN_LBRACE);
    
    ASTNode *node = create_ast_node(AST_COMPOUND_STMT, 
                                    parser->current_token->line,
                                    parser->current_token->column);
    
    int capacity = 8;
    node->data.compound.statements = malloc(capacity * sizeof(ASTNode*));
    node->data.compound.statement_count = 0;
    
    while (parser->current_token->type != TOKEN_RBRACE &&
           parser->current_token->type != TOKEN_EOF) {
        LOG_TRACE("In compound statement loop, current token: %s at %d:%d",
                  token_type_to_string(parser->current_token->type),
                  parser->current_token->line,
                  parser->current_token->column);
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

ASTNode *parse_statement(Parser *parser) {
    Token *token = parser->current_token;
    LOG_TRACE("parse_statement called with token: %s at %d:%d", 
              token_type_to_string(token->type),
              token->line,
              token->column);
    
    // Don't parse struct declarations inside functions - they should be at top level
    // Just fall through to variable declaration handling
    
    // Update token  
    token = parser->current_token;
    
    // Check for static keyword
    bool is_static = false;
    if (token->type == TOKEN_KEYWORD_STATIC) {
        is_static = true;
        parser_advance(parser);
        token = parser->current_token;
    }
    
    // Check for extern keyword
    bool is_extern = false;
    if (token->type == TOKEN_KEYWORD_EXTERN) {
        is_extern = true;
        parser_advance(parser);
        token = parser->current_token;
    }
    
    // Check for const keyword (can appear before type)
    bool is_const = false;
    if (token->type == TOKEN_KEYWORD_CONST) {
        is_const = true;
        parser_advance(parser);
        token = parser->current_token;
    }
    
    // Variable declaration (including function pointers)
    LOG_TRACE("Checking for variable declaration, current token: %s at %d:%d",
              token_type_to_string(parser->current_token->type),
              parser->current_token->line,
              parser->current_token->column);
    if (parser->current_token->type == TOKEN_KEYWORD_INT || 
        parser->current_token->type == TOKEN_KEYWORD_CHAR ||
        parser->current_token->type == TOKEN_KEYWORD_FLOAT ||
        parser->current_token->type == TOKEN_KEYWORD_DOUBLE ||
        parser->current_token->type == TOKEN_KEYWORD_VOID ||
        parser->current_token->type == TOKEN_KEYWORD_STRUCT ||
        parser->current_token->type == TOKEN_KEYWORD_ENUM ||
        (parser->current_token->type == TOKEN_IDENTIFIER && 
         is_typedef_name(parser, parser->current_token->text)) ||
        (is_const && parser->current_token->type == TOKEN_IDENTIFIER && 
         is_typedef_name(parser, parser->current_token->text))) {
        LOG_TRACE("Attempting to parse variable declaration starting with %s",
                  token_type_to_string(token->type));
        char *var_name = NULL;
        char *type_name = parse_type(parser, &var_name);
        
        if (!type_name) {
            LOG_ERROR("Failed to parse type at %d:%d", token->line, token->column);
            exit(1);
        }
        
        LOG_TRACE("Parsed type: %s, identifier: %s", type_name, var_name ? var_name : "(null)");
        
        // Check for const after type (e.g., int const)
        if (!is_const && parser->current_token->type == TOKEN_KEYWORD_CONST) {
            is_const = true;
            parser_advance(parser);
        }
        
        int var_line = token->line;
        int var_column = token->column;
        
        // If parse_type didn't get the identifier (for non-function-pointer types),
        // we need to get it now
        if (!var_name && parser->current_token->type == TOKEN_IDENTIFIER) {
            var_name = strdup(parser->current_token->text);
            var_line = parser->current_token->line;
            var_column = parser->current_token->column;
            parser_advance(parser);
        }
        
        if (!var_name) {
            LOG_ERROR("Expected variable name at %d:%d", 
                     parser->current_token->line, parser->current_token->column);
            exit(1);
        }
        
        ASTNode *node = create_ast_node(AST_VAR_DECL, var_line, var_column);
        node->data.var_decl.type = strdup(type_name);
        node->data.var_decl.name = var_name;
        node->data.var_decl.array_size = NULL;
        node->data.var_decl.is_static = is_static;
        node->data.var_decl.is_extern = is_extern;
        node->data.var_decl.is_const = is_const;
        node->data.var_decl.is_global = false;  // Local variable
        
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
    // Use parse_type to handle const and other qualifiers
    char *param_name = NULL;
    char *type_name = parse_type(parser, &param_name);
    
    if (!type_name) {
        LOG_ERROR("Expected type specifier but got %s", 
                  token_type_to_string(parser->current_token->type));
        exit(1);
    }
    
    // If parse_type didn't get the identifier, get it now
    if (!param_name && parser->current_token->type == TOKEN_IDENTIFIER) {
        param_name = strdup(parser->current_token->text);
        parser_advance(parser);
    }
    
    if (!param_name) {
        LOG_ERROR("Expected parameter name");
        exit(1);
    }
    
    int param_line = parser->current_token->line;
    int param_column = parser->current_token->column;
    
    ASTNode *param = create_ast_node(AST_PARAM_DECL, param_line, param_column);
    param->data.param_decl.type = type_name;
    param->data.param_decl.name = param_name;
    
    return param;
}

static ASTNode *parse_function(Parser *parser, bool is_static, bool is_extern) {
    LOG_TRACE("parse_function called at %d:%d", 
              parser->current_token->line, parser->current_token->column);
    
    int saved_line = parser->current_token->line;
    int saved_col = parser->current_token->column;
    
    // Parse the type
    char *type_name = parse_type(parser, NULL);
    if (!type_name) {
        LOG_ERROR("Expected type at %d:%d", saved_line, saved_col);
        return NULL;
    }
    
    // Should now be at identifier
    if (parser->current_token->type != TOKEN_IDENTIFIER) {
        LOG_ERROR("Expected identifier after type");
        free(type_name);
        return NULL;
    }
    
    char *name = strdup(parser->current_token->text);
    int name_line = parser->current_token->line;
    int name_col = parser->current_token->column;
    parser_advance(parser);
    
    // Check what follows - if not '(', it's a global variable
    if (parser->current_token->type != TOKEN_LPAREN) {
        // It's a global variable, not a function
        ASTNode *var_node = create_ast_node(AST_VAR_DECL, saved_line, saved_col);
        var_node->data.var_decl.type = type_name;
        var_node->data.var_decl.name = name;
        var_node->data.var_decl.is_static = is_static;
        var_node->data.var_decl.is_extern = is_extern;
        var_node->data.var_decl.is_const = (strstr(type_name, "const") != NULL);
        var_node->data.var_decl.is_global = true;
        
        // Check for array
        if (parser->current_token->type == TOKEN_LBRACKET) {
            parser_advance(parser);
            if (parser->current_token->type != TOKEN_RBRACKET) {
                var_node->data.var_decl.array_size = parse_expression(parser);
            }
            parser_expect(parser, TOKEN_RBRACKET);
        } else {
            var_node->data.var_decl.array_size = NULL;
        }
        
        // Check for initializer
        if (parser->current_token->type == TOKEN_ASSIGN) {
            parser_advance(parser);
            var_node->data.var_decl.initializer = parse_expression(parser);
        } else {
            var_node->data.var_decl.initializer = NULL;
        }
        
        parser_expect(parser, TOKEN_SEMICOLON);
        
        LOG_DEBUG("Parsed global variable: %s", name);
        return var_node;
    }
    
    // It's a function - continue parsing
    ASTNode *node = create_ast_node(AST_FUNCTION, saved_line, saved_col);
    node->data.function.name = name;
    node->data.function.return_type = type_name;
    node->data.function.is_static = is_static;
    node->data.function.is_extern = is_extern;
    
    parser_advance(parser); // consume '('
    
    // Parse parameters
    int param_capacity = 4;
    ASTNode **params = malloc(param_capacity * sizeof(ASTNode*));
    int param_count = 0;
    bool is_variadic = false;
    
    if (parser->current_token->type != TOKEN_RPAREN) {
        // Special case: void parameter list
        if (parser->current_token->type == TOKEN_KEYWORD_VOID && 
            parser->peek_token->type == TOKEN_RPAREN) {
            parser_advance(parser); // consume 'void'
            // Empty parameter list
        } else {
            params[param_count++] = parse_parameter(parser);
        }
        if (param_count > 0) {
            LOG_TRACE("After first parameter, current token: %s at %d:%d", 
                      token_type_to_string(parser->current_token->type),
                      parser->current_token->line,
                      parser->current_token->column);
        }
        
        while (param_count > 0 && parser->current_token->type == TOKEN_COMMA) {
            LOG_TRACE("Found comma in parameter list");
            parser_advance(parser);
            
            // Check for ellipsis after comma
            if (parser->current_token->type == TOKEN_ELLIPSIS) {
                LOG_TRACE("Found ellipsis after comma");
                is_variadic = true;
                parser_advance(parser);
                break; // Ellipsis must be last
            }
            
            if (param_count >= param_capacity) {
                param_capacity *= 2;
                params = realloc(params, param_capacity * sizeof(ASTNode*));
            }
            params[param_count++] = parse_parameter(parser);
        }
    }
    
    LOG_TRACE("Before expecting RPAREN, current token: %s at %d:%d",
              token_type_to_string(parser->current_token->type),
              parser->current_token->line,
              parser->current_token->column);
    parser_expect(parser, TOKEN_RPAREN);
    
    node->data.function.params = params;
    node->data.function.param_count = param_count;
    node->data.function.is_variadic = is_variadic;
    
    // Check if this is a function declaration (prototype) or definition
    if (parser->current_token->type == TOKEN_SEMICOLON) {
        // Function declaration (prototype)
        parser_advance(parser); // consume ';'
        node->data.function.body = NULL;
        LOG_DEBUG("Parsed function declaration: %s with %d parameters", node->data.function.name, param_count);
    } else {
        // Function definition with body
        node->data.function.body = parse_compound_statement(parser);
        LOG_DEBUG("Parsed function definition: %s with %d parameters", node->data.function.name, param_count);
    }
    
    return node;
}

static bool is_function_declaration(Parser *parser) {
    // Try to parse as function first since that's more common
    // Check current pattern: type [*]* identifier (
    
    // We must be at a type keyword
    if (parser->current_token->type != TOKEN_KEYWORD_INT &&
        parser->current_token->type != TOKEN_KEYWORD_CHAR &&
        parser->current_token->type != TOKEN_KEYWORD_VOID &&
        parser->current_token->type != TOKEN_KEYWORD_FLOAT &&
        parser->current_token->type != TOKEN_KEYWORD_DOUBLE) {
        return false;
    }
    
    // Simple heuristic: if type is void, it's almost certainly a function
    if (parser->current_token->type == TOKEN_KEYWORD_VOID) {
        return true;
    }
    
    // For other types, check if peek token helps us decide
    Token *peek = parser->peek_token;
    
    // If next token is identifier, it could be either
    if (peek && peek->type == TOKEN_IDENTIFIER) {
        // No stars between type and identifier
        // We can't look ahead further, so we'll parse as function by default
        // and let parse_function fail if it's not
        return true;
    }
    
    // If next token is *, we might have a pointer
    if (peek && peek->type == TOKEN_STAR) {
        // Could be either int *func() or int *var
        // Default to function
        return true;
    }
    
    // Default to function (most common at global scope)
    return true;
}

static ASTNode *parse_enum(Parser *parser) {
    LOG_TRACE("parse_enum called");
    
    int line = parser->current_token->line;
    int column = parser->current_token->column;
    
    parser_expect(parser, TOKEN_KEYWORD_ENUM);
    
    // Get enum name (optional)
    char *enum_name = NULL;
    if (parser->current_token->type == TOKEN_IDENTIFIER) {
        enum_name = strdup(parser->current_token->text);
        parser_advance(parser);
    }
    
    parser_expect(parser, TOKEN_LBRACE);
    
    // Parse enumerators
    int capacity = 8;
    char **names = malloc(capacity * sizeof(char*));
    int *values = malloc(capacity * sizeof(int));
    int count = 0;
    int next_value = 0;
    
    while (parser->current_token->type != TOKEN_RBRACE && 
           parser->current_token->type != TOKEN_EOF) {
        
        // Get enumerator name
        if (parser->current_token->type != TOKEN_IDENTIFIER) {
            LOG_ERROR("Expected identifier in enum at %d:%d",
                      parser->current_token->line, parser->current_token->column);
            exit(1);
        }
        
        if (count >= capacity) {
            capacity *= 2;
            names = realloc(names, capacity * sizeof(char*));
            values = realloc(values, capacity * sizeof(int));
        }
        
        names[count] = strdup(parser->current_token->text);
        parser_advance(parser);
        
        // Check for explicit value
        if (parser->current_token->type == TOKEN_ASSIGN) {
            parser_advance(parser);
            
            if (parser->current_token->type != TOKEN_INT_LITERAL) {
                LOG_ERROR("Expected integer literal after = in enum at %d:%d",
                          parser->current_token->line, parser->current_token->column);
                exit(1);
            }
            
            next_value = parser->current_token->value.int_value;
            parser_advance(parser);
        }
        
        values[count] = next_value;
        next_value++;
        count++;
        
        // Handle comma
        if (parser->current_token->type == TOKEN_COMMA) {
            parser_advance(parser);
            // Allow trailing comma
            if (parser->current_token->type == TOKEN_RBRACE) {
                break;
            }
        } else if (parser->current_token->type != TOKEN_RBRACE) {
            LOG_ERROR("Expected comma or } in enum at %d:%d",
                      parser->current_token->line, parser->current_token->column);
            exit(1);
        }
    }
    
    parser_expect(parser, TOKEN_RBRACE);
    parser_expect(parser, TOKEN_SEMICOLON);
    
    // Create the enum node
    ASTNode *node = create_ast_node(AST_ENUM_DECL, line, column);
    node->data.enum_decl.name = enum_name;
    node->data.enum_decl.enumerator_names = names;
    node->data.enum_decl.enumerator_values = values;
    node->data.enum_decl.enumerator_count = count;
    
    LOG_TRACE("Parsed enum: %s with %d enumerators", 
              enum_name ? enum_name : "<anonymous>", count);
    return node;
}

static ASTNode *parse_typedef(Parser *parser) {
    LOG_TRACE("parse_typedef called");
    
    int line = parser->current_token->line;
    int column = parser->current_token->column;
    ASTNode *struct_decl = NULL;
    
    parser_expect(parser, TOKEN_KEYWORD_TYPEDEF);
    
    // Parse the base type
    char *base_type = NULL;
    if (parser->current_token->type == TOKEN_KEYWORD_INT) {
        base_type = strdup("int");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_CHAR) {
        base_type = strdup("char");
        parser_advance(parser);
    } else if (parser->current_token->type == TOKEN_KEYWORD_STRUCT) {
        parser_advance(parser);
        
        // Check if this is an anonymous struct (has { directly)
        if (parser->current_token->type == TOKEN_LBRACE) {
            // Anonymous struct - generate a unique name
            static int anon_struct_counter = 0;
            char anon_name[64];
            snprintf(anon_name, sizeof(anon_name), "__anon_struct_%d", anon_struct_counter++);
            
            // Parse the struct body
            struct_decl = parse_struct_declaration(parser, strdup(anon_name));
            
            // Use the anonymous struct name as the type
            char struct_type[256];
            snprintf(struct_type, sizeof(struct_type), "struct %s", anon_name);
            base_type = strdup(struct_type);
        } else if (parser->current_token->type == TOKEN_IDENTIFIER) {
            // Named struct reference
            char struct_type[256];
            snprintf(struct_type, sizeof(struct_type), "struct %s", parser->current_token->text);
            base_type = strdup(struct_type);
            parser_advance(parser);
        } else {
            LOG_ERROR("Expected struct name or '{' after 'struct' at %d:%d",
                      parser->current_token->line, parser->current_token->column);
            exit(1);
        }
    } else {
        LOG_ERROR("Expected type after typedef at %d:%d", 
                  parser->current_token->line, parser->current_token->column);
        exit(1);
    }
    
    // Handle pointer types
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
    
    // Get the new type name
    if (parser->current_token->type != TOKEN_IDENTIFIER) {
        LOG_ERROR("Expected identifier after type in typedef at %d:%d",
                  parser->current_token->line, parser->current_token->column);
        exit(1);
    }
    char *new_type_name = strdup(parser->current_token->text);
    parser_advance(parser);
    
    parser_expect(parser, TOKEN_SEMICOLON);
    
    // Create the typedef node
    ASTNode *node = create_ast_node(AST_TYPEDEF_DECL, line, column);
    node->data.typedef_decl.name = new_type_name;
    node->data.typedef_decl.base_type = strdup(type_name);
    
    // Store the struct declaration if we created one  
    node->data.typedef_decl.struct_decl = struct_decl;
    
    // Add typedef name to tracking list
    if (parser->typedef_count >= parser->typedef_capacity) {
        parser->typedef_capacity *= 2;
        parser->typedef_names = realloc(parser->typedef_names, 
                                       parser->typedef_capacity * sizeof(char*));
    }
    parser->typedef_names[parser->typedef_count++] = strdup(new_type_name);
    
    LOG_TRACE("Parsed typedef: %s as %s", new_type_name, type_name);
    return node;
}

static ASTNode *parse_global_variable(Parser *parser) {
    // Parse global variable declaration (similar to local but at global scope)
    char *type_name = parse_type(parser, NULL);
    
    if (!type_name) {
        LOG_ERROR("Failed to parse type for global variable");
        return NULL;
    }
    
    Token *name_token = parser->current_token;
    char *var_name = strdup(name_token->text);
    int var_line = name_token->line;
    int var_column = name_token->column;
    parser_expect(parser, TOKEN_IDENTIFIER);
    
    ASTNode *array_size = NULL;
    // Check for array declaration
    if (parser->current_token->type == TOKEN_LBRACKET) {
        parser_advance(parser);
        if (parser->current_token->type != TOKEN_RBRACKET) {
            array_size = parse_expression(parser);
        }
        parser_expect(parser, TOKEN_RBRACKET);
    }
    
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
    var_decl->data.var_decl.array_size = array_size;
    var_decl->data.var_decl.is_static = false;
    var_decl->data.var_decl.is_global = true;  // Mark as global
    
    free(type_name);
    return var_decl;
}

static ASTNode *parse_struct_declaration(Parser *parser, char *struct_name) {
    LOG_DEBUG("Parsing struct declaration: %s", struct_name);
    
    ASTNode *struct_node = create_ast_node(AST_STRUCT_DECL, 
                                          parser->current_token->line, 
                                          parser->current_token->column);
    struct_node->data.struct_decl.name = struct_name;
    
    parser_expect(parser, TOKEN_LBRACE); // consume '{'
    
    // Parse struct members
    int member_capacity = 8;
    struct_node->data.struct_decl.members = malloc(member_capacity * sizeof(ASTNode*));
    struct_node->data.struct_decl.member_count = 0;
    
    while (parser->current_token->type != TOKEN_RBRACE && 
           parser->current_token->type != TOKEN_EOF) {
        // Parse member type
        char *member_type = parse_type(parser, NULL);
        if (!member_type) {
            LOG_ERROR("Expected member type in struct");
            ast_destroy(struct_node);
            return NULL;
        }
        
        // Parse member name
        if (parser->current_token->type != TOKEN_IDENTIFIER) {
            LOG_ERROR("Expected member name after type");
            free(member_type);
            ast_destroy(struct_node);
            return NULL;
        }
        
        // Create member node
        ASTNode *member = create_ast_node(AST_VAR_DECL, 
                                        parser->current_token->line,
                                        parser->current_token->column);
        member->data.var_decl.type = member_type;
        member->data.var_decl.name = strdup(parser->current_token->text);
        member->data.var_decl.initializer = NULL;
        member->data.var_decl.array_size = NULL;
        member->data.var_decl.is_static = false;
        member->data.var_decl.is_const = false;
        member->data.var_decl.is_global = false;
        
        parser_advance(parser); // consume member name
        
        // Check for array
        if (parser->current_token->type == TOKEN_LBRACKET) {
            parser_advance(parser);
            if (parser->current_token->type != TOKEN_RBRACKET) {
                member->data.var_decl.array_size = parse_expression(parser);
            }
            parser_expect(parser, TOKEN_RBRACKET);
        }
        
        parser_expect(parser, TOKEN_SEMICOLON);
        
        // Add member to struct
        if (struct_node->data.struct_decl.member_count >= member_capacity) {
            member_capacity *= 2;
            struct_node->data.struct_decl.members = realloc(struct_node->data.struct_decl.members,
                                                           member_capacity * sizeof(ASTNode*));
        }
        struct_node->data.struct_decl.members[struct_node->data.struct_decl.member_count++] = member;
    }
    
    parser_expect(parser, TOKEN_RBRACE); // consume '}'
    
    // Only expect semicolon if not in typedef context
    // Check if next token is a semicolon (struct declaration) or identifier (typedef)
    if (parser->current_token->type == TOKEN_SEMICOLON) {
        parser_advance(parser); // consume ';'
    }
    
    LOG_DEBUG("Parsed struct %s with %d members", struct_name, struct_node->data.struct_decl.member_count);
    
    return struct_node;
}

ASTNode *parser_parse(Parser *parser) {
    ASTNode *program = create_ast_node(AST_PROGRAM, 1, 1);
    
    int func_capacity = 8;
    int var_capacity = 8;
    int typedef_capacity = 8;
    int enum_capacity = 8;
    program->data.program.functions = malloc(func_capacity * sizeof(ASTNode*));
    program->data.program.function_count = 0;
    program->data.program.global_vars = malloc(var_capacity * sizeof(ASTNode*));
    program->data.program.global_var_count = 0;
    program->data.program.typedefs = malloc(typedef_capacity * sizeof(ASTNode*));
    program->data.program.typedef_count = 0;
    program->data.program.enums = malloc(enum_capacity * sizeof(ASTNode*));
    program->data.program.enum_count = 0;
    
    while (parser->current_token->type != TOKEN_EOF) {
        if (parser->current_token->type == TOKEN_KEYWORD_TYPEDEF) {
            // Parse typedef
            if (program->data.program.typedef_count >= typedef_capacity) {
                typedef_capacity *= 2;
                program->data.program.typedefs = realloc(program->data.program.typedefs,
                                                        typedef_capacity * sizeof(ASTNode*));
            }
            program->data.program.typedefs[program->data.program.typedef_count++] = 
                parse_typedef(parser);
        } else if (parser->current_token->type == TOKEN_KEYWORD_ENUM) {
            // Parse enum
            if (program->data.program.enum_count >= enum_capacity) {
                enum_capacity *= 2;
                program->data.program.enums = realloc(program->data.program.enums,
                                                      enum_capacity * sizeof(ASTNode*));
            }
            program->data.program.enums[program->data.program.enum_count++] = 
                parse_enum(parser);
        } else if (parser->current_token->type == TOKEN_KEYWORD_STRUCT) {
            // Check if this is a struct declaration or just a struct variable
            Token *saved_token = parser->current_token;
            parser_advance(parser); // consume 'struct'
            
            if (parser->current_token->type == TOKEN_IDENTIFIER) {
                char *struct_name = strdup(parser->current_token->text);
                parser_advance(parser); // consume struct name
                
                if (parser->current_token->type == TOKEN_LBRACE) {
                    // This is a struct declaration - parse it
                    ASTNode *struct_decl = parse_struct_declaration(parser, struct_name);
                    // For now, we'll add it to typedefs since there's no structs array in program
                    if (program->data.program.typedef_count >= typedef_capacity) {
                        typedef_capacity *= 2;
                        program->data.program.typedefs = realloc(program->data.program.typedefs,
                                                                typedef_capacity * sizeof(ASTNode*));
                    }
                    program->data.program.typedefs[program->data.program.typedef_count++] = struct_decl;
                    // Don't free struct_name - parse_struct_declaration takes ownership
                } else {
                    // Not a struct declaration, rewind and let parse_function handle it
                    parser->current_token = saved_token;
                    parser->peek_token = lexer_next_token(parser->lexer);
                    
                    // Parse as function or global variable
                    if (program->data.program.function_count >= func_capacity) {
                        func_capacity *= 2;
                        program->data.program.functions = realloc(program->data.program.functions,
                                                                 func_capacity * sizeof(ASTNode*));
                    }
                    
                    ASTNode *node = parse_function(parser, false, false);
                    if (node) {
                        if (node->type == AST_FUNCTION) {
                            program->data.program.functions[program->data.program.function_count++] = node;
                        } else if (node->type == AST_VAR_DECL) {
                            if (program->data.program.global_var_count >= var_capacity) {
                                var_capacity *= 2;
                                program->data.program.global_vars = realloc(program->data.program.global_vars,
                                                                           var_capacity * sizeof(ASTNode*));
                            }
                            program->data.program.global_vars[program->data.program.global_var_count++] = node;
                        }
                    }
                    free(struct_name);
                }
            } else {
                LOG_ERROR("Expected struct name after 'struct'");
                return NULL;
            }
        } else if (parser->current_token->type == TOKEN_KEYWORD_STATIC) {
            // Static function or variable
            parser_advance(parser); // consume 'static'
            
            if (program->data.program.function_count >= func_capacity) {
                func_capacity *= 2;
                program->data.program.functions = realloc(program->data.program.functions,
                                                         func_capacity * sizeof(ASTNode*));
            }
            
            // Parse with static flag
            ASTNode *node = parse_function(parser, true, false);
            if (node) {
                if (node->type == AST_FUNCTION) {
                    program->data.program.functions[program->data.program.function_count++] = node;
                } else if (node->type == AST_VAR_DECL) {
                    // Static global variable
                    if (program->data.program.global_var_count >= var_capacity) {
                        var_capacity *= 2;
                        program->data.program.global_vars = realloc(program->data.program.global_vars,
                                                                   var_capacity * sizeof(ASTNode*));
                    }
                    program->data.program.global_vars[program->data.program.global_var_count++] = node;
                }
            }
        } else if (parser->current_token->type == TOKEN_KEYWORD_EXTERN) {
            // Extern function or variable
            parser_advance(parser); // consume 'extern'
            
            if (program->data.program.function_count >= func_capacity) {
                func_capacity *= 2;
                program->data.program.functions = realloc(program->data.program.functions,
                                                         func_capacity * sizeof(ASTNode*));
            }
            
            // Parse with extern flag
            ASTNode *node = parse_function(parser, false, true);
            if (node) {
                if (node->type == AST_FUNCTION) {
                    program->data.program.functions[program->data.program.function_count++] = node;
                } else if (node->type == AST_VAR_DECL) {
                    // Extern global variable
                    if (program->data.program.global_var_count >= var_capacity) {
                        var_capacity *= 2;
                        program->data.program.global_vars = realloc(program->data.program.global_vars,
                                                                   var_capacity * sizeof(ASTNode*));
                    }
                    program->data.program.global_vars[program->data.program.global_var_count++] = node;
                }
            }
        } else {
            // Try to parse as function - this handles both functions and will
            // internally detect and parse global variables
            if (program->data.program.function_count >= func_capacity) {
                func_capacity *= 2;
                program->data.program.functions = realloc(program->data.program.functions,
                                                         func_capacity * sizeof(ASTNode*));
            }
            
            // Attempt to parse as function
            ASTNode *node = parse_function(parser, false, false);
            if (node) {
                if (node->type == AST_FUNCTION) {
                    program->data.program.functions[program->data.program.function_count++] = node;
                } else if (node->type == AST_VAR_DECL) {
                    // parse_function detected it was actually a global variable
                    if (program->data.program.global_var_count >= var_capacity) {
                        var_capacity *= 2;
                        program->data.program.global_vars = realloc(program->data.program.global_vars,
                                                                   var_capacity * sizeof(ASTNode*));
                    }
                    program->data.program.global_vars[program->data.program.global_var_count++] = node;
                }
            }
        }
    }
    
    LOG_INFO("Parsed program with %d functions, %d global variables, %d typedefs, and %d enums", 
             program->data.program.function_count, program->data.program.global_var_count,
             program->data.program.typedef_count, program->data.program.enum_count);
    
    // Check if we had any errors
    if (parser->had_error) {
        error_print_all(parser->error_manager);
        ast_destroy(program);
        return NULL;
    }
    
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
            for (int i = 0; i < node->data.program.typedef_count; i++) {
                ast_destroy(node->data.program.typedefs[i]);
            }
            free(node->data.program.typedefs);
            for (int i = 0; i < node->data.program.enum_count; i++) {
                ast_destroy(node->data.program.enums[i]);
            }
            free(node->data.program.enums);
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
        case AST_UNION_DECL:
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
        case AST_TYPEDEF_DECL:
            free(node->data.typedef_decl.name);
            free(node->data.typedef_decl.base_type);
            if (node->data.typedef_decl.struct_decl) {
                ast_destroy(node->data.typedef_decl.struct_decl);
            }
            break;
        case AST_ENUM_DECL:
            free(node->data.enum_decl.name);
            for (int i = 0; i < node->data.enum_decl.enumerator_count; i++) {
                free(node->data.enum_decl.enumerator_names[i]);
            }
            free(node->data.enum_decl.enumerator_names);
            free(node->data.enum_decl.enumerator_values);
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
            for (int i = 0; i < node->data.program.typedef_count; i++) {
                ast_print(node->data.program.typedefs[i], indent + 1);
            }
            for (int i = 0; i < node->data.program.enum_count; i++) {
                ast_print(node->data.program.enums[i], indent + 1);
            }
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
        case AST_FLOAT_LITERAL:
            printf("Float: %f\n", node->data.float_literal.value);
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
        case AST_TYPEDEF_DECL:
            printf("Typedef: %s = %s\n", node->data.typedef_decl.name, node->data.typedef_decl.base_type);
            break;
        case AST_ENUM_DECL:
            printf("Enum: %s {\n", node->data.enum_decl.name ? node->data.enum_decl.name : "<anonymous>");
            for (int i = 0; i < node->data.enum_decl.enumerator_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                printf("%s = %d\n", node->data.enum_decl.enumerator_names[i], 
                       node->data.enum_decl.enumerator_values[i]);
            }
            for (int i = 0; i < indent; i++) printf("  ");
            printf("}\n");
            break;
        case AST_STRUCT_DECL:
            printf("Struct: %s {\n", node->data.struct_decl.name);
            for (int i = 0; i < node->data.struct_decl.member_count; i++) {
                for (int j = 0; j < indent + 1; j++) printf("  ");
                ast_print(node->data.struct_decl.members[i], indent + 1);
            }
            for (int i = 0; i < indent; i++) printf("  ");
            printf("}\n");
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
        case AST_FLOAT_LITERAL:
            clone->data.float_literal.value = node->data.float_literal.value;
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
        case AST_TYPEDEF_DECL:
            clone->data.typedef_decl.name = strdup(node->data.typedef_decl.name);
            clone->data.typedef_decl.base_type = strdup(node->data.typedef_decl.base_type);
            break;
        case AST_ENUM_DECL:
            clone->data.enum_decl.name = node->data.enum_decl.name ? strdup(node->data.enum_decl.name) : NULL;
            clone->data.enum_decl.enumerator_count = node->data.enum_decl.enumerator_count;
            clone->data.enum_decl.enumerator_names = malloc(clone->data.enum_decl.enumerator_count * sizeof(char*));
            clone->data.enum_decl.enumerator_values = malloc(clone->data.enum_decl.enumerator_count * sizeof(int));
            for (int i = 0; i < clone->data.enum_decl.enumerator_count; i++) {
                clone->data.enum_decl.enumerator_names[i] = strdup(node->data.enum_decl.enumerator_names[i]);
                clone->data.enum_decl.enumerator_values[i] = node->data.enum_decl.enumerator_values[i];
            }
            break;
        case AST_STRUCT_DECL:
            clone->data.struct_decl.name = strdup(node->data.struct_decl.name);
            clone->data.struct_decl.member_count = node->data.struct_decl.member_count;
            clone->data.struct_decl.members = malloc(clone->data.struct_decl.member_count * sizeof(ASTNode*));
            for (int i = 0; i < clone->data.struct_decl.member_count; i++) {
                clone->data.struct_decl.members[i] = ast_clone(node->data.struct_decl.members[i]);
            }
            break;
        default:
            LOG_ERROR("ast_clone not implemented for node type: %d", node->type);
            exit(1);
    }
    
    return clone;
}
