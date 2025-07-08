#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_IDENTIFIER,
    TOKEN_KEYWORD_IF,
    TOKEN_KEYWORD_ELSE,
    TOKEN_KEYWORD_WHILE,
    TOKEN_KEYWORD_DO,
    TOKEN_KEYWORD_FOR,
    TOKEN_KEYWORD_BREAK,
    TOKEN_KEYWORD_CONTINUE,
    TOKEN_KEYWORD_RETURN,
    TOKEN_KEYWORD_INT,
    TOKEN_KEYWORD_CHAR,
    TOKEN_KEYWORD_FLOAT,
    TOKEN_KEYWORD_DOUBLE,
    TOKEN_KEYWORD_STRUCT,
    TOKEN_KEYWORD_UNION,
    TOKEN_KEYWORD_SIZEOF,
    TOKEN_KEYWORD_SWITCH,
    TOKEN_KEYWORD_CASE,
    TOKEN_KEYWORD_DEFAULT,
    TOKEN_KEYWORD_TYPEDEF,
    TOKEN_KEYWORD_ENUM,
    TOKEN_KEYWORD_STATIC,
    TOKEN_KEYWORD_CONST,
    TOKEN_COLON,
    TOKEN_AND,     // &&
    TOKEN_OR,      // ||
    TOKEN_NOT,     // !
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,   // %
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_SEMICOLON,
    TOKEN_ASSIGN,
    TOKEN_EQ,
    TOKEN_NE,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_COMMA,
    TOKEN_LBRACKET,  // [
    TOKEN_RBRACKET,  // ]
    TOKEN_AMPERSAND, // & (both address-of and bitwise AND)
    TOKEN_DOT,       // .
    TOKEN_PIPE,      // |
    TOKEN_CARET,     // ^
    TOKEN_TILDE,     // ~
    TOKEN_LSHIFT,    // <<
    TOKEN_RSHIFT,    // >>
    TOKEN_PLUS_ASSIGN,  // +=
    TOKEN_MINUS_ASSIGN, // -=
    TOKEN_STAR_ASSIGN,  // *=
    TOKEN_SLASH_ASSIGN, // /=
    TOKEN_INCREMENT,    // ++
    TOKEN_DECREMENT,    // --
    TOKEN_QUESTION,     // ?
    TOKEN_ELLIPSIS,     // ...
    TOKEN_UNKNOWN
} TokenType;

typedef struct {
    TokenType type;
    char *text;
    int line;
    int column;
    union {
        int int_value;
        char char_value;
        double float_value;
    } value;
} Token;

typedef struct {
    FILE *input;
    int current_char;
    int line;
    int column;
    char *filename;
} Lexer;

Lexer *lexer_create(FILE *input, const char *filename);
void lexer_destroy(Lexer *lexer);
Token *lexer_next_token(Lexer *lexer);
void token_destroy(Token *token);
const char *token_type_to_string(TokenType type);

#endif
