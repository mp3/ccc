#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "parser.h"
#include <stdbool.h>

typedef struct {
    bool enable_constant_folding;
    bool enable_dead_code_elimination;
    bool enable_constant_propagation;
    bool enable_strength_reduction;
    bool enable_algebraic_simplification;
    bool enable_common_subexpr_elimination;
    bool enable_loop_invariant_motion;
    int optimizations_performed;
    // For constant propagation tracking
    struct ConstantMap *constants;
} Optimizer;

Optimizer *optimizer_create(void);
void optimizer_destroy(Optimizer *opt);
ASTNode *optimizer_optimize(Optimizer *opt, ASTNode *ast);

// Individual optimization passes
ASTNode *optimize_constant_folding(Optimizer *opt, ASTNode *node);
ASTNode *optimize_dead_code_elimination(Optimizer *opt, ASTNode *node);
ASTNode *optimize_constant_propagation(Optimizer *opt, ASTNode *node);
ASTNode *optimize_strength_reduction(Optimizer *opt, ASTNode *node);
ASTNode *optimize_algebraic_simplification(Optimizer *opt, ASTNode *node);
ASTNode *optimize_common_subexpr_elimination(Optimizer *opt, ASTNode *node);
ASTNode *optimize_loop_invariant_motion(Optimizer *opt, ASTNode *node);

#endif
