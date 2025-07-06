#include <stdio.h>
#include "src/lexer.h"

int main() {
    FILE *f = fopen("tests/test_expr_only.c", "r");
    Lexer *lexer = lexer_create(f, "tests/test_expr_only.c");
    
    Token *token;
    while ((token = lexer_next_token(lexer))->type != TOKEN_EOF) {
        printf("Token: %s (%s) at %d:%d\n", 
               token_type_to_string(token->type), 
               token->text, 
               token->line, 
               token->column);
        token_destroy(token);
    }
    
    token_destroy(token);
    lexer_destroy(lexer);
    fclose(f);
    return 0;
}