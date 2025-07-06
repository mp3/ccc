#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "parser.h"
#include <stdbool.h>

typedef struct {
    bool enable_constant_folding;
    bool enable_dead_code_elimination;
    int optimizations_performed;
} Optimizer;

Optimizer *optimizer_create(void);
void optimizer_destroy(Optimizer *opt);
ASTNode *optimizer_optimize(Optimizer *opt, ASTNode *ast);

// Individual optimization passes
ASTNode *optimize_constant_folding(Optimizer *opt, ASTNode *node);
ASTNode *optimize_dead_code_elimination(Optimizer *opt, ASTNode *node);

#endif
