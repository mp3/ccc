#include "optimizer.h"
#include "logger.h"
#include "lexer.h"
#include <stdlib.h>
#include <string.h>

Optimizer *optimizer_create(void) {
    Optimizer *opt = malloc(sizeof(Optimizer));
    opt->enable_constant_folding = true;
    opt->enable_dead_code_elimination = true;
    opt->optimizations_performed = 0;
    LOG_DEBUG("Created optimizer");
    return opt;
}

void optimizer_destroy(Optimizer *opt) {
    if (opt) {
        free(opt);
    }
}

// Helper function to check if a node is a constant
static bool is_constant(ASTNode *node) {
    return node && node->type == AST_INT_LITERAL;
}

// Helper function to create a new integer literal node
static ASTNode *create_int_literal(int value, int line, int column) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = AST_INT_LITERAL;
    node->line = line;
    node->column = column;
    node->data.int_literal.value = value;
    return node;
}

// Constant folding for binary operations
static ASTNode *fold_binary_op(Optimizer *opt, ASTNode *node) {
    if (!node || node->type != AST_BINARY_OP) {
        return node;
    }
    
    ASTNode *left = node->data.binary_op.left;
    ASTNode *right = node->data.binary_op.right;
    
    // First, recursively optimize children
    node->data.binary_op.left = optimize_constant_folding(opt, left);
    node->data.binary_op.right = optimize_constant_folding(opt, right);
    
    // Update pointers after potential optimization
    left = node->data.binary_op.left;
    right = node->data.binary_op.right;
    
    // If both operands are constants, fold them
    if (is_constant(left) && is_constant(right)) {
        int left_val = left->data.int_literal.value;
        int right_val = right->data.int_literal.value;
        int result;
        bool can_fold = true;
        
        switch (node->data.binary_op.op) {
            case TOKEN_PLUS:
                result = left_val + right_val;
                break;
            case TOKEN_MINUS:
                result = left_val - right_val;
                break;
            case TOKEN_STAR:
                result = left_val * right_val;
                break;
            case TOKEN_SLASH:
                if (right_val == 0) {
                    LOG_WARN("Division by zero in constant folding, skipping optimization");
                    can_fold = false;
                } else {
                    result = left_val / right_val;
                }
                break;
            case TOKEN_EQ:
                result = (left_val == right_val) ? 1 : 0;
                break;
            case TOKEN_NE:
                result = (left_val != right_val) ? 1 : 0;
                break;
            case TOKEN_LT:
                result = (left_val < right_val) ? 1 : 0;
                break;
            case TOKEN_GT:
                result = (left_val > right_val) ? 1 : 0;
                break;
            case TOKEN_LE:
                result = (left_val <= right_val) ? 1 : 0;
                break;
            case TOKEN_GE:
                result = (left_val >= right_val) ? 1 : 0;
                break;
            case TOKEN_AND:
                result = (left_val && right_val) ? 1 : 0;
                break;
            case TOKEN_OR:
                result = (left_val || right_val) ? 1 : 0;
                break;
            default:
                can_fold = false;
                break;
        }
        
        if (can_fold) {
            LOG_DEBUG("Constant folding: %d %s %d = %d", 
                     left_val, token_type_to_string(node->data.binary_op.op), 
                     right_val, result);
            
            // Create new constant node
            ASTNode *folded = create_int_literal(result, node->line, node->column);
            
            // Clean up old nodes
            ast_destroy(left);
            ast_destroy(right);
            free(node);
            
            opt->optimizations_performed++;
            return folded;
        }
    }
    
    return node;
}

// Main constant folding optimization
ASTNode *optimize_constant_folding(Optimizer *opt, ASTNode *node) {
    if (!node || !opt->enable_constant_folding) {
        return node;
    }
    
    switch (node->type) {
        case AST_BINARY_OP:
            return fold_binary_op(opt, node);
            
        case AST_UNARY_OP:
            // Optimize the operand first
            node->data.unary_op.operand = 
                optimize_constant_folding(opt, node->data.unary_op.operand);
            
            // If operand is constant and operator is NOT, fold it
            if (node->data.unary_op.op == TOKEN_NOT && 
                is_constant(node->data.unary_op.operand)) {
                int operand_val = node->data.unary_op.operand->data.int_literal.value;
                int result = !operand_val ? 1 : 0;
                
                LOG_DEBUG("Constant folding: !%d = %d", operand_val, result);
                
                // Create new constant node
                ASTNode *folded = create_int_literal(result, node->line, node->column);
                
                // Clean up old nodes
                ast_destroy(node->data.unary_op.operand);
                free(node);
                
                opt->optimizations_performed++;
                return folded;
            }
            break;
            
        case AST_VAR_DECL:
            if (node->data.var_decl.initializer) {
                node->data.var_decl.initializer = 
                    optimize_constant_folding(opt, node->data.var_decl.initializer);
            }
            break;
            
        case AST_ASSIGNMENT:
            node->data.assignment.value = 
                optimize_constant_folding(opt, node->data.assignment.value);
            break;
            
        case AST_RETURN_STMT:
            node->data.return_stmt.expression = 
                optimize_constant_folding(opt, node->data.return_stmt.expression);
            break;
            
        case AST_BREAK_STMT:
        case AST_CONTINUE_STMT:
            // No expressions to optimize
            break;
            
        case AST_IF_STMT:
            node->data.if_stmt.condition = 
                optimize_constant_folding(opt, node->data.if_stmt.condition);
            node->data.if_stmt.then_stmt = 
                optimize_constant_folding(opt, node->data.if_stmt.then_stmt);
            if (node->data.if_stmt.else_stmt) {
                node->data.if_stmt.else_stmt = 
                    optimize_constant_folding(opt, node->data.if_stmt.else_stmt);
            }
            break;
            
        case AST_WHILE_STMT:
            node->data.while_stmt.condition = 
                optimize_constant_folding(opt, node->data.while_stmt.condition);
            node->data.while_stmt.body = 
                optimize_constant_folding(opt, node->data.while_stmt.body);
            break;
        case AST_DO_WHILE_STMT:
            node->data.do_while_stmt.body = 
                optimize_constant_folding(opt, node->data.do_while_stmt.body);
            node->data.do_while_stmt.condition = 
                optimize_constant_folding(opt, node->data.do_while_stmt.condition);
            break;
        case AST_FOR_STMT:
            if (node->data.for_stmt.init) {
                node->data.for_stmt.init = 
                    optimize_constant_folding(opt, node->data.for_stmt.init);
            }
            if (node->data.for_stmt.condition) {
                node->data.for_stmt.condition = 
                    optimize_constant_folding(opt, node->data.for_stmt.condition);
            }
            if (node->data.for_stmt.update) {
                node->data.for_stmt.update = 
                    optimize_constant_folding(opt, node->data.for_stmt.update);
            }
            node->data.for_stmt.body = 
                optimize_constant_folding(opt, node->data.for_stmt.body);
            break;
            
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound.statement_count; i++) {
                node->data.compound.statements[i] = 
                    optimize_constant_folding(opt, node->data.compound.statements[i]);
            }
            break;
            
        case AST_EXPR_STMT:
            node->data.expr_stmt.expression = 
                optimize_constant_folding(opt, node->data.expr_stmt.expression);
            break;
            
        case AST_FUNCTION:
            node->data.function.body = 
                optimize_constant_folding(opt, node->data.function.body);
            break;
            
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.function_count; i++) {
                node->data.program.functions[i] = 
                    optimize_constant_folding(opt, node->data.program.functions[i]);
            }
            break;
            
        case AST_FUNCTION_CALL:
            for (int i = 0; i < node->data.function_call.argument_count; i++) {
                node->data.function_call.arguments[i] = 
                    optimize_constant_folding(opt, node->data.function_call.arguments[i]);
            }
            break;
            
        case AST_SIZEOF:
            // sizeof is already a compile-time constant in codegen
            // but we can optimize any expression inside sizeof(expression)
            if (node->data.sizeof_op.expression) {
                node->data.sizeof_op.expression = 
                    optimize_constant_folding(opt, node->data.sizeof_op.expression);
            }
            break;
            
        case AST_SWITCH_STMT:
            node->data.switch_stmt.expression = 
                optimize_constant_folding(opt, node->data.switch_stmt.expression);
            for (int i = 0; i < node->data.switch_stmt.case_count; i++) {
                node->data.switch_stmt.cases[i] = 
                    optimize_constant_folding(opt, node->data.switch_stmt.cases[i]);
            }
            if (node->data.switch_stmt.default_case) {
                node->data.switch_stmt.default_case = 
                    optimize_constant_folding(opt, node->data.switch_stmt.default_case);
            }
            break;
            
        case AST_CASE_STMT:
            // Case value must remain constant, but optimize statements
            for (int i = 0; i < node->data.case_stmt.statement_count; i++) {
                node->data.case_stmt.statements[i] = 
                    optimize_constant_folding(opt, node->data.case_stmt.statements[i]);
            }
            break;
            
        case AST_DEFAULT_STMT:
            for (int i = 0; i < node->data.default_stmt.statement_count; i++) {
                node->data.default_stmt.statements[i] = 
                    optimize_constant_folding(opt, node->data.default_stmt.statements[i]);
            }
            break;
            
        default:
            // No optimization needed for other node types
            break;
    }
    
    return node;
}

// Dead code elimination for if statements with constant conditions
static ASTNode *eliminate_dead_if(Optimizer *opt, ASTNode *node) {
    if (!node || node->type != AST_IF_STMT) {
        return node;
    }
    
    // First optimize the condition
    node->data.if_stmt.condition = 
        optimize_constant_folding(opt, node->data.if_stmt.condition);
    
    // If condition is a constant, eliminate dead branch
    if (is_constant(node->data.if_stmt.condition)) {
        int cond_value = node->data.if_stmt.condition->data.int_literal.value;
        ASTNode *kept_branch;
        
        if (cond_value != 0) {
            // Condition is true, keep then branch
            LOG_DEBUG("Dead code elimination: if condition is always true");
            kept_branch = node->data.if_stmt.then_stmt;
            node->data.if_stmt.then_stmt = NULL;  // Prevent double-free
            
            // Clean up else branch
            if (node->data.if_stmt.else_stmt) {
                ast_destroy(node->data.if_stmt.else_stmt);
            }
        } else {
            // Condition is false, keep else branch (or nothing)
            LOG_DEBUG("Dead code elimination: if condition is always false");
            kept_branch = node->data.if_stmt.else_stmt;
            node->data.if_stmt.else_stmt = NULL;  // Prevent double-free
            
            // Clean up then branch
            ast_destroy(node->data.if_stmt.then_stmt);
        }
        
        // Clean up the if node itself
        ast_destroy(node->data.if_stmt.condition);
        free(node);
        
        opt->optimizations_performed++;
        
        // Recursively optimize the kept branch
        if (kept_branch) {
            return optimize_dead_code_elimination(opt, kept_branch);
        } else {
            // No else branch and condition is false - return empty compound statement
            ASTNode *empty = malloc(sizeof(ASTNode));
            empty->type = AST_COMPOUND_STMT;
            empty->line = node->line;
            empty->column = node->column;
            empty->data.compound.statements = NULL;
            empty->data.compound.statement_count = 0;
            return empty;
        }
    }
    
    // Condition is not constant, optimize branches normally
    node->data.if_stmt.then_stmt = 
        optimize_dead_code_elimination(opt, node->data.if_stmt.then_stmt);
    if (node->data.if_stmt.else_stmt) {
        node->data.if_stmt.else_stmt = 
            optimize_dead_code_elimination(opt, node->data.if_stmt.else_stmt);
    }
    
    return node;
}

// Dead code elimination for while loops with constant false conditions
static ASTNode *eliminate_dead_while(Optimizer *opt, ASTNode *node) {
    if (!node || node->type != AST_WHILE_STMT) {
        return node;
    }
    
    // First optimize the condition
    node->data.while_stmt.condition = 
        optimize_constant_folding(opt, node->data.while_stmt.condition);
    
    // If condition is constant false, eliminate the loop
    if (is_constant(node->data.while_stmt.condition)) {
        int cond_value = node->data.while_stmt.condition->data.int_literal.value;
        
        if (cond_value == 0) {
            LOG_DEBUG("Dead code elimination: while condition is always false");
            
            // Clean up the while node
            ast_destroy(node->data.while_stmt.condition);
            ast_destroy(node->data.while_stmt.body);
            free(node);
            
            opt->optimizations_performed++;
            
            // Return empty compound statement
            ASTNode *empty = malloc(sizeof(ASTNode));
            empty->type = AST_COMPOUND_STMT;
            empty->line = node->line;
            empty->column = node->column;
            empty->data.compound.statements = NULL;
            empty->data.compound.statement_count = 0;
            return empty;
        }
        // Note: We don't optimize infinite loops (condition always true)
        // as they might be intentional
    }
    
    // Optimize the body
    node->data.while_stmt.body = 
        optimize_dead_code_elimination(opt, node->data.while_stmt.body);
    
    return node;
}

static ASTNode *eliminate_dead_for(Optimizer *opt, ASTNode *node) {
    // Optimize components first
    if (node->data.for_stmt.init) {
        node->data.for_stmt.init = 
            optimize_dead_code_elimination(opt, node->data.for_stmt.init);
    }
    
    if (node->data.for_stmt.condition) {
        node->data.for_stmt.condition = 
            optimize_dead_code_elimination(opt, node->data.for_stmt.condition);
        
        // Check if condition is constant false
        if (node->data.for_stmt.condition->type == AST_INT_LITERAL &&
            node->data.for_stmt.condition->data.int_literal.value == 0) {
            
            LOG_DEBUG("Eliminating for loop with constant false condition");
            opt->optimizations_performed++;
            
            // For loop never executes - return just the init statement if present
            if (node->data.for_stmt.init) {
                return node->data.for_stmt.init;
            } else {
                // Return empty compound statement
                ASTNode *empty = malloc(sizeof(ASTNode));
                empty->type = AST_COMPOUND_STMT;
                empty->line = node->line;
                empty->column = node->column;
                empty->data.compound.statements = NULL;
                empty->data.compound.statement_count = 0;
                ast_destroy(node);
                return empty;
            }
        }
    }
    
    // Optimize update and body
    if (node->data.for_stmt.update) {
        node->data.for_stmt.update = 
            optimize_dead_code_elimination(opt, node->data.for_stmt.update);
    }
    
    node->data.for_stmt.body = 
        optimize_dead_code_elimination(opt, node->data.for_stmt.body);
    
    return node;
}

// Main dead code elimination optimization
ASTNode *optimize_dead_code_elimination(Optimizer *opt, ASTNode *node) {
    if (!node || !opt->enable_dead_code_elimination) {
        return node;
    }
    
    switch (node->type) {
        case AST_IF_STMT:
            return eliminate_dead_if(opt, node);
            
        case AST_WHILE_STMT:
            return eliminate_dead_while(opt, node);
        case AST_DO_WHILE_STMT:
            // Optimize body first (it always executes at least once)
            node->data.do_while_stmt.body = 
                optimize_dead_code_elimination(opt, node->data.do_while_stmt.body);
            
            // Then optimize condition
            node->data.do_while_stmt.condition = 
                optimize_dead_code_elimination(opt, node->data.do_while_stmt.condition);
            
            // Note: We can't eliminate do-while based on condition being false
            // because the body executes at least once
            break;
        case AST_FOR_STMT:
            return eliminate_dead_for(opt, node);
            
        case AST_SWITCH_STMT:
            // Optimize switch expression first
            node->data.switch_stmt.expression = 
                optimize_dead_code_elimination(opt, node->data.switch_stmt.expression);
            
            // Optimize case statements
            for (int i = 0; i < node->data.switch_stmt.case_count; i++) {
                node->data.switch_stmt.cases[i] = 
                    optimize_dead_code_elimination(opt, node->data.switch_stmt.cases[i]);
            }
            
            // Optimize default case
            if (node->data.switch_stmt.default_case) {
                node->data.switch_stmt.default_case = 
                    optimize_dead_code_elimination(opt, node->data.switch_stmt.default_case);
            }
            break;
            
        case AST_CASE_STMT:
            for (int i = 0; i < node->data.case_stmt.statement_count; i++) {
                node->data.case_stmt.statements[i] = 
                    optimize_dead_code_elimination(opt, node->data.case_stmt.statements[i]);
            }
            break;
            
        case AST_DEFAULT_STMT:
            for (int i = 0; i < node->data.default_stmt.statement_count; i++) {
                node->data.default_stmt.statements[i] = 
                    optimize_dead_code_elimination(opt, node->data.default_stmt.statements[i]);
            }
            break;
            
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound.statement_count; i++) {
                node->data.compound.statements[i] = 
                    optimize_dead_code_elimination(opt, node->data.compound.statements[i]);
            }
            break;
            
        case AST_FUNCTION:
            node->data.function.body = 
                optimize_dead_code_elimination(opt, node->data.function.body);
            break;
            
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.function_count; i++) {
                node->data.program.functions[i] = 
                    optimize_dead_code_elimination(opt, node->data.program.functions[i]);
            }
            break;
            
        default:
            // Recursively optimize other nodes
            if (node->type == AST_VAR_DECL && node->data.var_decl.initializer) {
                node->data.var_decl.initializer = 
                    optimize_dead_code_elimination(opt, node->data.var_decl.initializer);
            } else if (node->type == AST_ASSIGNMENT) {
                node->data.assignment.value = 
                    optimize_dead_code_elimination(opt, node->data.assignment.value);
            } else if (node->type == AST_RETURN_STMT) {
                node->data.return_stmt.expression = 
                    optimize_dead_code_elimination(opt, node->data.return_stmt.expression);
            } else if (node->type == AST_BREAK_STMT || node->type == AST_CONTINUE_STMT) {
                // No sub-expressions to optimize
            } else if (node->type == AST_EXPR_STMT) {
                node->data.expr_stmt.expression = 
                    optimize_dead_code_elimination(opt, node->data.expr_stmt.expression);
            }
            break;
    }
    
    return node;
}

// Main optimization entry point
ASTNode *optimizer_optimize(Optimizer *opt, ASTNode *ast) {
    if (!opt || !ast) {
        return ast;
    }
    
    LOG_INFO("Starting optimization pass");
    int initial_count = opt->optimizations_performed;
    
    // Apply optimizations in order
    ast = optimize_constant_folding(opt, ast);
    ast = optimize_dead_code_elimination(opt, ast);
    
    int optimizations = opt->optimizations_performed - initial_count;
    LOG_INFO("Optimization complete: %d optimizations performed", optimizations);
    
    return ast;
}
