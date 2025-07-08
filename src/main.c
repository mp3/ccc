#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "logger.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "optimizer.h"
#include "semantic.h"

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s [options] <input.c> -o <output.ll>\n", program_name);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -O0    Disable optimizations\n");
    fprintf(stderr, "  -O1    Enable optimizations (default)\n");
    fprintf(stderr, "  -O2    Enable all optimizations\n");
}

int main(int argc, char **argv) {
    log_init("ccc.log", LOG_TRACE);
    LOG_INFO("ccc compiler starting");
    
    // Parse command line arguments
    bool optimize = true;  // Default to -O1
    int opt_level = 1;
    const char *input_file = NULL;
    const char *output_file = NULL;
    
    int i = 1;
    while (i < argc) {
        if (strcmp(argv[i], "-O0") == 0) {
            optimize = false;
            opt_level = 0;
            i++;
        } else if (strcmp(argv[i], "-O1") == 0) {
            optimize = true;
            opt_level = 1;
            i++;
        } else if (strcmp(argv[i], "-O2") == 0) {
            optimize = true;
            opt_level = 2;
            i++;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[i + 1];
            i += 2;
        } else if (!input_file && argv[i][0] != '-') {
            input_file = argv[i];
            i++;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            log_cleanup();
            return 1;
        }
    }
    
    if (!input_file || !output_file) {
        print_usage(argv[0]);
        log_cleanup();
        return 1;
    }
    
    LOG_INFO("Compiling %s to %s", input_file, output_file);
    
    FILE *input = fopen(input_file, "r");
    if (!input) {
        LOG_ERROR("Failed to open input file: %s", input_file);
        log_cleanup();
        return 1;
    }
    
    // Create lexer
    Lexer *lexer = lexer_create(input, input_file);
    
    // Create parser and parse
    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse(parser);
    
    if (!ast) {
        LOG_ERROR("Parsing failed");
        parser_destroy(parser);
        lexer_destroy(lexer);
        fclose(input);
        log_cleanup();
        return 1;
    }
    
    // Perform semantic analysis for warnings
    LOG_INFO("Performing semantic analysis");
    SemanticAnalyzer *analyzer = semantic_create(parser->error_manager);
    semantic_analyze(analyzer, ast);
    semantic_destroy(analyzer);
    
    // Print all errors and warnings
    error_print_all(parser->error_manager);
    
    // Check if we should continue (only errors prevent compilation)
    if (parser->error_manager->error_count > 0) {
        LOG_ERROR("Compilation failed due to errors");
        ast_destroy(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        fclose(input);
        log_cleanup();
        return 1;
    }
    
    // Apply optimizations if enabled
    if (optimize) {
        LOG_INFO("Applying optimizations (level %d)", opt_level);
        Optimizer *optimizer = optimizer_create();
        
        // Configure optimization levels
        if (opt_level == 0) {
            optimizer->enable_constant_folding = false;
            optimizer->enable_dead_code_elimination = false;
            optimizer->enable_constant_propagation = false;
            optimizer->enable_strength_reduction = false;
            optimizer->enable_algebraic_simplification = false;
        } else if (opt_level == 1) {
            optimizer->enable_constant_folding = true;
            optimizer->enable_dead_code_elimination = false;
            optimizer->enable_constant_propagation = true;
            optimizer->enable_strength_reduction = false;
            optimizer->enable_algebraic_simplification = true;
        } else { // opt_level >= 2
            optimizer->enable_constant_folding = true;
            optimizer->enable_dead_code_elimination = true;
            optimizer->enable_constant_propagation = true;
            optimizer->enable_strength_reduction = true;
            optimizer->enable_algebraic_simplification = true;
        }
        
        ast = optimizer_optimize(optimizer, ast);
        optimizer_destroy(optimizer);
    }
    
    // Open output file
    FILE *output = fopen(output_file, "w");
    if (!output) {
        LOG_ERROR("Failed to open output file: %s", output_file);
        ast_destroy(ast);
        parser_destroy(parser);
        lexer_destroy(lexer);
        fclose(input);
        log_cleanup();
        return 1;
    }
    
    // Generate code
    CodeGenerator *codegen = codegen_create(output);
    codegen_generate(codegen, ast);
    
    // Cleanup
    codegen_destroy(codegen);
    ast_destroy(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    fclose(input);
    fclose(output);
    
    LOG_INFO("Compilation complete");
    log_cleanup();
    return 0;
}
