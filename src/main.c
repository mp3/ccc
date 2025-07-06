#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s <input.c> -o <output.ll>\n", program_name);
}

int main(int argc, char **argv) {
    log_init("ccc.log", LOG_DEBUG);
    LOG_INFO("ccc compiler starting");
    
    if (argc < 4 || strcmp(argv[2], "-o") != 0) {
        print_usage(argv[0]);
        log_cleanup();
        return 1;
    }
    
    const char *input_file = argv[1];
    const char *output_file = argv[3];
    
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
