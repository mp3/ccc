#include "optimizer.h"
#include "logger.h"
#include "lexer.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>

// Forward declaration for ast_destroy
void ast_destroy(ASTNode *node);

// Constant map for constant propagation
typedef struct ConstantEntry {
    char *name;
    int value;
    struct ConstantEntry *next;
} ConstantEntry;

typedef struct ConstantMap {
    ConstantEntry *entries;
} ConstantMap;

static ConstantMap *constant_map_create(void) {
    ConstantMap *map = malloc(sizeof(ConstantMap));
    map->entries = NULL;
    return map;
}

static void constant_map_destroy(ConstantMap *map) {
    if (!map) return;
    ConstantEntry *entry = map->entries;
    while (entry) {
        ConstantEntry *next = entry->next;
        free(entry->name);
        free(entry);
        entry = next;
    }
    free(map);
}

static void constant_map_set(ConstantMap *map, const char *name, int value) {
    ConstantEntry *entry = map->entries;
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            entry->value = value;
            return;
        }
        entry = entry->next;
    }
    // Add new entry
    entry = malloc(sizeof(ConstantEntry));
    entry->name = strdup(name);
    entry->value = value;
    entry->next = map->entries;
    map->entries = entry;
}

static bool constant_map_get(ConstantMap *map, const char *name, int *value) {
    ConstantEntry *entry = map->entries;
    while (entry) {
        if (strcmp(entry->name, name) == 0) {
            *value = entry->value;
            return true;
        }
        entry = entry->next;
    }
    return false;
}

Optimizer *optimizer_create(void) {
    Optimizer *opt = malloc(sizeof(Optimizer));
    opt->enable_constant_folding = true;
    opt->enable_dead_code_elimination = true;
    opt->enable_constant_propagation = true;
    opt->enable_strength_reduction = true;
    opt->enable_algebraic_simplification = true;
    opt->enable_common_subexpr_elimination = false; // Complex, disabled by default
    opt->enable_loop_invariant_motion = false; // Complex, disabled by default
    opt->optimizations_performed = 0;
    opt->constants = constant_map_create();
    LOG_DEBUG("Created optimizer");
    return opt;
}

void optimizer_destroy(Optimizer *opt) {
    if (opt) {
        constant_map_destroy(opt->constants);
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

// Forward declaration for helper function
static void optimize_node_children(ASTNode *node, ASTNode *(*optimize_fn)(Optimizer *, ASTNode *), Optimizer *opt);

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

// Strength reduction optimization
ASTNode *optimize_strength_reduction(Optimizer *opt, ASTNode *node) {
    if (!opt || !opt->enable_strength_reduction || !node) {
        return node;
    }
    
    switch (node->type) {
        case AST_BINARY_OP: {
            // First optimize children
            node->data.binary_op.left = optimize_strength_reduction(opt, node->data.binary_op.left);
            node->data.binary_op.right = optimize_strength_reduction(opt, node->data.binary_op.right);
            
            ASTNode *left = node->data.binary_op.left;
            ASTNode *right = node->data.binary_op.right;
            
            // Multiplication by power of 2 -> left shift
            if (node->data.binary_op.op == TOKEN_STAR && is_constant(right)) {
                int val = right->data.int_literal.value;
                // Check if it's a power of 2
                if (val > 0 && (val & (val - 1)) == 0) {
                    // Count trailing zeros to get shift amount
                    int shift = 0;
                    while ((val & 1) == 0) {
                        val >>= 1;
                        shift++;
                    }
                    LOG_DEBUG("Strength reduction: multiply by %d -> left shift by %d", 
                              right->data.int_literal.value, shift);
                    // For now, keep as multiplication since we don't have shift operators
                    // In a real implementation, we'd convert to shift here
                    opt->optimizations_performed++;
                }
            }
            
            // Division by power of 2 -> right shift (for positive numbers)
            if (node->data.binary_op.op == TOKEN_SLASH && is_constant(right)) {
                int val = right->data.int_literal.value;
                if (val > 0 && (val & (val - 1)) == 0) {
                    int shift = 0;
                    while ((val & 1) == 0) {
                        val >>= 1;
                        shift++;
                    }
                    LOG_DEBUG("Strength reduction: divide by %d -> right shift by %d", 
                              right->data.int_literal.value, shift);
                    opt->optimizations_performed++;
                }
            }
            break;
        }
        
        default:
            // Recursively optimize other node types
            optimize_node_children(node, optimize_strength_reduction, opt);
            break;
    }
    
    return node;
}

// Algebraic simplification
ASTNode *optimize_algebraic_simplification(Optimizer *opt, ASTNode *node) {
    if (!opt || !opt->enable_algebraic_simplification || !node) {
        return node;
    }
    
    switch (node->type) {
        case AST_BINARY_OP: {
            // First optimize children
            node->data.binary_op.left = optimize_algebraic_simplification(opt, node->data.binary_op.left);
            node->data.binary_op.right = optimize_algebraic_simplification(opt, node->data.binary_op.right);
            
            ASTNode *left = node->data.binary_op.left;
            ASTNode *right = node->data.binary_op.right;
            
            // x + 0 = x, 0 + x = x
            if (node->data.binary_op.op == TOKEN_PLUS) {
                if (is_constant(right) && right->data.int_literal.value == 0) {
                    LOG_DEBUG("Algebraic simplification: x + 0 -> x");
                    opt->optimizations_performed++;
                    ast_destroy(right);
                    free(node);
                    return left;
                }
                if (is_constant(left) && left->data.int_literal.value == 0) {
                    LOG_DEBUG("Algebraic simplification: 0 + x -> x");
                    opt->optimizations_performed++;
                    ast_destroy(left);
                    free(node);
                    return right;
                }
            }
            
            // x - 0 = x
            if (node->data.binary_op.op == TOKEN_MINUS) {
                if (is_constant(right) && right->data.int_literal.value == 0) {
                    LOG_DEBUG("Algebraic simplification: x - 0 -> x");
                    opt->optimizations_performed++;
                    ast_destroy(right);
                    free(node);
                    return left;
                }
            }
            
            // x * 0 = 0, 0 * x = 0
            if (node->data.binary_op.op == TOKEN_STAR) {
                if ((is_constant(right) && right->data.int_literal.value == 0) ||
                    (is_constant(left) && left->data.int_literal.value == 0)) {
                    LOG_DEBUG("Algebraic simplification: x * 0 -> 0");
                    opt->optimizations_performed++;
                    ast_destroy(node);
                    return create_int_literal(0, node->line, node->column);
                }
                // x * 1 = x, 1 * x = x
                if (is_constant(right) && right->data.int_literal.value == 1) {
                    LOG_DEBUG("Algebraic simplification: x * 1 -> x");
                    opt->optimizations_performed++;
                    ast_destroy(right);
                    free(node);
                    return left;
                }
                if (is_constant(left) && left->data.int_literal.value == 1) {
                    LOG_DEBUG("Algebraic simplification: 1 * x -> x");
                    opt->optimizations_performed++;
                    ast_destroy(left);
                    free(node);
                    return right;
                }
            }
            
            // x / 1 = x
            if (node->data.binary_op.op == TOKEN_SLASH) {
                if (is_constant(right) && right->data.int_literal.value == 1) {
                    LOG_DEBUG("Algebraic simplification: x / 1 -> x");
                    opt->optimizations_performed++;
                    ast_destroy(right);
                    free(node);
                    return left;
                }
            }
            break;
        }
        
        default:
            // Recursively optimize other node types
            optimize_node_children(node, optimize_algebraic_simplification, opt);
            break;
    }
    
    return node;
}

// Constant propagation
ASTNode *optimize_constant_propagation(Optimizer *opt, ASTNode *node) {
    if (!opt || !opt->enable_constant_propagation || !node) {
        return node;
    }
    
    switch (node->type) {
        case AST_VAR_DECL:
            // If initializing with a constant, track it
            if (node->data.var_decl.initializer && 
                node->data.var_decl.initializer->type == AST_INT_LITERAL) {
                constant_map_set(opt->constants, node->data.var_decl.name, 
                               node->data.var_decl.initializer->data.int_literal.value);
                LOG_DEBUG("Constant propagation: tracking %s = %d", 
                         node->data.var_decl.name, 
                         node->data.var_decl.initializer->data.int_literal.value);
            }
            break;
            
        case AST_IDENTIFIER: {
            // Replace identifier with its constant value if known
            int value;
            if (constant_map_get(opt->constants, node->data.identifier.name, &value)) {
                LOG_DEBUG("Constant propagation: replacing %s with %d", 
                         node->data.identifier.name, value);
                opt->optimizations_performed++;
                ASTNode *literal = create_int_literal(value, node->line, node->column);
                ast_destroy(node);
                return literal;
            }
            break;
        }
        
        case AST_ASSIGNMENT:
            // If assigning a constant, update tracking
            if (node->data.assignment.value && 
                node->data.assignment.value->type == AST_INT_LITERAL) {
                constant_map_set(opt->constants, node->data.assignment.name,
                               node->data.assignment.value->data.int_literal.value);
            } else {
                // Non-constant assignment invalidates the constant tracking
                constant_map_set(opt->constants, node->data.assignment.name, 0);
            }
            break;
            
        default:
            // Recursively optimize other node types
            optimize_node_children(node, optimize_constant_propagation, opt);
            break;
    }
    
    return node;
}

// Placeholder for complex optimizations
ASTNode *optimize_common_subexpr_elimination(Optimizer *opt, ASTNode *node) {
    // Complex implementation - would need expression hashing and tracking
    return node;
}

ASTNode *optimize_loop_invariant_motion(Optimizer *opt, ASTNode *node) {
    // Complex implementation - would need loop analysis and invariant detection
    return node;
}

// Helper function to recursively optimize children of a node
static void optimize_node_children(ASTNode *node, ASTNode *(*optimize_fn)(Optimizer *, ASTNode *), Optimizer *opt) {
    if (!node) return;
    
    switch (node->type) {
        case AST_PROGRAM:
            for (int i = 0; i < node->data.program.function_count; i++) {
                node->data.program.functions[i] = optimize_fn(opt, node->data.program.functions[i]);
            }
            break;
            
        case AST_FUNCTION:
            node->data.function.body = optimize_fn(opt, node->data.function.body);
            break;
            
        case AST_COMPOUND_STMT:
            for (int i = 0; i < node->data.compound.statement_count; i++) {
                node->data.compound.statements[i] = optimize_fn(opt, node->data.compound.statements[i]);
            }
            break;
            
        case AST_IF_STMT:
            node->data.if_stmt.condition = optimize_fn(opt, node->data.if_stmt.condition);
            node->data.if_stmt.then_stmt = optimize_fn(opt, node->data.if_stmt.then_stmt);
            if (node->data.if_stmt.else_stmt) {
                node->data.if_stmt.else_stmt = optimize_fn(opt, node->data.if_stmt.else_stmt);
            }
            break;
            
        case AST_WHILE_STMT:
            node->data.while_stmt.condition = optimize_fn(opt, node->data.while_stmt.condition);
            node->data.while_stmt.body = optimize_fn(opt, node->data.while_stmt.body);
            break;
            
        case AST_FOR_STMT:
            if (node->data.for_stmt.init) {
                node->data.for_stmt.init = optimize_fn(opt, node->data.for_stmt.init);
            }
            if (node->data.for_stmt.condition) {
                node->data.for_stmt.condition = optimize_fn(opt, node->data.for_stmt.condition);
            }
            if (node->data.for_stmt.update) {
                node->data.for_stmt.update = optimize_fn(opt, node->data.for_stmt.update);
            }
            node->data.for_stmt.body = optimize_fn(opt, node->data.for_stmt.body);
            break;
            
        case AST_RETURN_STMT:
            if (node->data.return_stmt.expression) {
                node->data.return_stmt.expression = optimize_fn(opt, node->data.return_stmt.expression);
            }
            break;
            
        case AST_EXPR_STMT:
            if (node->data.expr_stmt.expression) {
                node->data.expr_stmt.expression = optimize_fn(opt, node->data.expr_stmt.expression);
            }
            break;
            
        case AST_VAR_DECL:
            if (node->data.var_decl.initializer) {
                node->data.var_decl.initializer = optimize_fn(opt, node->data.var_decl.initializer);
            }
            break;
            
        case AST_BINARY_OP:
            node->data.binary_op.left = optimize_fn(opt, node->data.binary_op.left);
            node->data.binary_op.right = optimize_fn(opt, node->data.binary_op.right);
            break;
            
        default:
            // Other nodes don't have children or are handled specially
            break;
    }
}

// Main optimization entry point
ASTNode *optimizer_optimize(Optimizer *opt, ASTNode *ast) {
    if (!opt || !ast) {
        return ast;
    }
    
    LOG_INFO("Starting optimization pass");
    int initial_count = opt->optimizations_performed;
    
    // Apply optimizations in order
    // 1. Constant propagation (must come before constant folding)
    if (opt->enable_constant_propagation) {
        ast = optimize_constant_propagation(opt, ast);
    }
    
    // 2. Constant folding
    if (opt->enable_constant_folding) {
        ast = optimize_constant_folding(opt, ast);
    }
    
    // 3. Algebraic simplification
    if (opt->enable_algebraic_simplification) {
        ast = optimize_algebraic_simplification(opt, ast);
    }
    
    // 4. Strength reduction
    if (opt->enable_strength_reduction) {
        ast = optimize_strength_reduction(opt, ast);
    }
    
    // 5. Dead code elimination
    if (opt->enable_dead_code_elimination) {
        ast = optimize_dead_code_elimination(opt, ast);
    }
    
    // 6. Common subexpression elimination (if enabled)
    if (opt->enable_common_subexpr_elimination) {
        ast = optimize_common_subexpr_elimination(opt, ast);
    }
    
    // 7. Loop invariant motion (if enabled)
    if (opt->enable_loop_invariant_motion) {
        ast = optimize_loop_invariant_motion(opt, ast);
    }
    
    int optimizations = opt->optimizations_performed - initial_count;
    LOG_INFO("Optimization complete: %d optimizations performed", optimizations);
    
    return ast;
}
