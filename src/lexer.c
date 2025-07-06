#include "lexer.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static void lexer_advance(Lexer *lexer) {
    if (lexer->current_char == '\n') {
        lexer->line++;
        lexer->column = 0;
    } else {
        lexer->column++;
    }
    lexer->current_char = fgetc(lexer->input);
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (isspace(lexer->current_char)) {
        lexer_advance(lexer);
    }
}

static void lexer_skip_comment(Lexer *lexer) {
    if (lexer->current_char == '/' && fgetc(lexer->input) == '/') {
        while (lexer->current_char != '\n' && lexer->current_char != EOF) {
            lexer_advance(lexer);
        }
    }
}

static Token *create_token(TokenType type, const char *text, int line, int column) {
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->text = strdup(text);
    token->line = line;
    token->column = column;
    return token;
}

static Token *lexer_read_number(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    char buffer[256];
    int i = 0;
    
    while (isdigit(lexer->current_char) && i < 255) {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    Token *token = create_token(TOKEN_INT_LITERAL, buffer, start_line, start_column);
    token->value.int_value = atoi(buffer);
    LOG_TRACE("Lexed integer: %d", token->value.int_value);
    return token;
}

static Token *lexer_read_identifier(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    char buffer[256];
    int i = 0;
    
    while ((isalnum(lexer->current_char) || lexer->current_char == '_') && i < 255) {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }
    buffer[i] = '\0';
    
    TokenType type = TOKEN_IDENTIFIER;
    if (strcmp(buffer, "if") == 0) type = TOKEN_KEYWORD_IF;
    else if (strcmp(buffer, "while") == 0) type = TOKEN_KEYWORD_WHILE;
    else if (strcmp(buffer, "return") == 0) type = TOKEN_KEYWORD_RETURN;
    else if (strcmp(buffer, "int") == 0) type = TOKEN_KEYWORD_INT;
    
    Token *token = create_token(type, buffer, start_line, start_column);
    LOG_TRACE("Lexed %s: %s", token_type_to_string(type), buffer);
    return token;
}

Lexer *lexer_create(FILE *input, const char *filename) {
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->input = input;
    lexer->filename = strdup(filename);
    lexer->line = 1;
    lexer->column = 0;
    lexer->current_char = fgetc(input);
    LOG_DEBUG("Created lexer for file: %s", filename);
    return lexer;
}

void lexer_destroy(Lexer *lexer) {
    if (lexer) {
        free(lexer->filename);
        free(lexer);
    }
}

Token *lexer_next_token(Lexer *lexer) {
    lexer_skip_whitespace(lexer);
    
    while (lexer->current_char == '/') {
        int next_char = fgetc(lexer->input);
        ungetc(next_char, lexer->input);
        if (next_char == '/') {
            lexer_skip_comment(lexer);
            lexer_skip_whitespace(lexer);
        } else {
            break;
        }
    }
    
    int line = lexer->line;
    int column = lexer->column;
    
    if (lexer->current_char == EOF) {
        return create_token(TOKEN_EOF, "", line, column);
    }
    
    if (isdigit(lexer->current_char)) {
        return lexer_read_number(lexer);
    }
    
    if (isalpha(lexer->current_char) || lexer->current_char == '_') {
        return lexer_read_identifier(lexer);
    }
    
    char ch = lexer->current_char;
    lexer_advance(lexer);
    
    switch (ch) {
        case '+': return create_token(TOKEN_PLUS, "+", line, column);
        case '-': return create_token(TOKEN_MINUS, "-", line, column);
        case '*': return create_token(TOKEN_STAR, "*", line, column);
        case '/': return create_token(TOKEN_SLASH, "/", line, column);
        case '(': return create_token(TOKEN_LPAREN, "(", line, column);
        case ')': return create_token(TOKEN_RPAREN, ")", line, column);
        case '{': return create_token(TOKEN_LBRACE, "{", line, column);
        case '}': return create_token(TOKEN_RBRACE, "}", line, column);
        case ';': return create_token(TOKEN_SEMICOLON, ";", line, column);
        case ',': return create_token(TOKEN_COMMA, ",", line, column);
        case '=':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_EQ, "==", line, column);
            }
            return create_token(TOKEN_ASSIGN, "=", line, column);
        case '!':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_NE, "!=", line, column);
            }
            break;
        case '<':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_LE, "<=", line, column);
            }
            return create_token(TOKEN_LT, "<", line, column);
        case '>':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_GE, ">=", line, column);
            }
            return create_token(TOKEN_GT, ">", line, column);
    }
    
    char unknown[2] = {ch, '\0'};
    LOG_WARN("Unknown character: %c (0x%02X) at %d:%d", ch, ch, line, column);
    return create_token(TOKEN_UNKNOWN, unknown, line, column);
}

void token_destroy(Token *token) {
    if (token) {
        free(token->text);
        free(token);
    }
}

const char *token_type_to_string(TokenType type) {
    static const char *names[] = {
        "EOF", "INT_LITERAL", "IDENTIFIER", "IF", "WHILE", "RETURN", "INT",
        "PLUS", "MINUS", "STAR", "SLASH", "LPAREN", "RPAREN", "LBRACE", "RBRACE",
        "SEMICOLON", "ASSIGN", "EQ", "NE", "LT", "GT", "LE", "GE", "COMMA", "UNKNOWN"
    };
    return names[type];
}
