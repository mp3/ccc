#include "lexer.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

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

static void lexer_skip_line_directive(Lexer *lexer) {
    // Skip preprocessor line directives: # linenum "filename"
    if (lexer->current_char == '#') {
        // Save current position
        int saved_line = lexer->line;
        int saved_column = lexer->column;
        
        // Skip #
        lexer_advance(lexer);
        
        // Skip whitespace
        while (lexer->current_char == ' ' || lexer->current_char == '\t') {
            lexer_advance(lexer);
        }
        
        // Check if this is a line directive (starts with a digit)
        if (lexer->current_char >= '0' && lexer->current_char <= '9') {
            // Skip the entire line
            while (lexer->current_char != '\n' && lexer->current_char != EOF) {
                lexer_advance(lexer);
            }
            if (lexer->current_char == '\n') {
                lexer_advance(lexer);
            }
        } else {
            // Not a line directive, restore position
            // This is a hack - we'd need to unread characters properly
            // For now, we'll just continue and let the lexer handle it as unknown
            lexer->line = saved_line;
            lexer->column = saved_column;
            lexer->current_char = '#';
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
    bool is_float = false;
    
    // Read integer part
    while (isdigit(lexer->current_char) && i < 255) {
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }
    
    // Check for decimal point
    if (lexer->current_char == '.' && i < 254) {
        // Peek ahead to ensure it's a float (not member access)
        int next_char = fgetc(lexer->input);
        ungetc(next_char, lexer->input);
        
        if (isdigit(next_char)) {
            is_float = true;
            buffer[i++] = lexer->current_char;
            lexer_advance(lexer);
            
            // Read fractional part
            while (isdigit(lexer->current_char) && i < 255) {
                buffer[i++] = lexer->current_char;
                lexer_advance(lexer);
            }
        }
    }
    
    // Check for exponent (e or E)
    if ((lexer->current_char == 'e' || lexer->current_char == 'E') && i < 253) {
        is_float = true;
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
        
        // Optional sign
        if ((lexer->current_char == '+' || lexer->current_char == '-') && i < 254) {
            buffer[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
        
        // Exponent digits
        while (isdigit(lexer->current_char) && i < 255) {
            buffer[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
    }
    
    // Check for float suffix (f or F)
    if ((lexer->current_char == 'f' || lexer->current_char == 'F') && i < 255) {
        is_float = true;
        buffer[i++] = lexer->current_char;
        lexer_advance(lexer);
    }
    
    buffer[i] = '\0';
    
    if (is_float) {
        Token *token = create_token(TOKEN_FLOAT_LITERAL, buffer, start_line, start_column);
        token->value.float_value = strtod(buffer, NULL);
        LOG_TRACE("Lexed float: %f", token->value.float_value);
        return token;
    } else {
        Token *token = create_token(TOKEN_INT_LITERAL, buffer, start_line, start_column);
        token->value.int_value = atoi(buffer);
        LOG_TRACE("Lexed integer: %d", token->value.int_value);
        return token;
    }
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
    else if (strcmp(buffer, "else") == 0) type = TOKEN_KEYWORD_ELSE;
    else if (strcmp(buffer, "while") == 0) type = TOKEN_KEYWORD_WHILE;
    else if (strcmp(buffer, "do") == 0) type = TOKEN_KEYWORD_DO;
    else if (strcmp(buffer, "for") == 0) type = TOKEN_KEYWORD_FOR;
    else if (strcmp(buffer, "break") == 0) type = TOKEN_KEYWORD_BREAK;
    else if (strcmp(buffer, "continue") == 0) type = TOKEN_KEYWORD_CONTINUE;
    else if (strcmp(buffer, "return") == 0) type = TOKEN_KEYWORD_RETURN;
    else if (strcmp(buffer, "int") == 0) type = TOKEN_KEYWORD_INT;
    else if (strcmp(buffer, "char") == 0) type = TOKEN_KEYWORD_CHAR;
    else if (strcmp(buffer, "float") == 0) type = TOKEN_KEYWORD_FLOAT;
    else if (strcmp(buffer, "double") == 0) type = TOKEN_KEYWORD_DOUBLE;
    else if (strcmp(buffer, "void") == 0) type = TOKEN_KEYWORD_VOID;
    else if (strcmp(buffer, "struct") == 0) type = TOKEN_KEYWORD_STRUCT;
    else if (strcmp(buffer, "union") == 0) type = TOKEN_KEYWORD_UNION;
    else if (strcmp(buffer, "sizeof") == 0) type = TOKEN_KEYWORD_SIZEOF;
    else if (strcmp(buffer, "switch") == 0) type = TOKEN_KEYWORD_SWITCH;
    else if (strcmp(buffer, "case") == 0) type = TOKEN_KEYWORD_CASE;
    else if (strcmp(buffer, "default") == 0) type = TOKEN_KEYWORD_DEFAULT;
    else if (strcmp(buffer, "typedef") == 0) type = TOKEN_KEYWORD_TYPEDEF;
    else if (strcmp(buffer, "enum") == 0) type = TOKEN_KEYWORD_ENUM;
    else if (strcmp(buffer, "static") == 0) type = TOKEN_KEYWORD_STATIC;
    else if (strcmp(buffer, "const") == 0) type = TOKEN_KEYWORD_CONST;
    else if (strcmp(buffer, "extern") == 0) type = TOKEN_KEYWORD_EXTERN;
    
    Token *token = create_token(type, buffer, start_line, start_column);
    LOG_TRACE("Lexed %s: %s", token_type_to_string(type), buffer);
    return token;
}

static Token *lexer_read_char_literal(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    char buffer[4] = {'\'', '\0', '\0', '\0'};
    
    lexer_advance(lexer); // skip opening '
    
    if (lexer->current_char == '\\') {
        // Handle escape sequences
        lexer_advance(lexer);
        char escaped;
        switch (lexer->current_char) {
            case 'n': escaped = '\n'; break;
            case 't': escaped = '\t'; break;
            case 'r': escaped = '\r'; break;
            case '\\': escaped = '\\'; break;
            case '\'': escaped = '\''; break;
            case '0': escaped = '\0'; break;
            default:
                LOG_WARN("Unknown escape sequence: \\%c", lexer->current_char);
                escaped = lexer->current_char;
        }
        buffer[1] = '\\';
        buffer[2] = lexer->current_char;
        lexer_advance(lexer);
        
        Token *token = create_token(TOKEN_CHAR_LITERAL, buffer, start_line, start_column);
        token->value.char_value = escaped;
        
        if (lexer->current_char == '\'') {
            lexer_advance(lexer);
        } else {
            LOG_WARN("Missing closing quote for character literal");
        }
        return token;
    } else if (lexer->current_char != '\'' && lexer->current_char != '\n' && lexer->current_char != EOF) {
        char ch = lexer->current_char;
        buffer[1] = ch;
        buffer[2] = '\'';
        lexer_advance(lexer);
        
        Token *token = create_token(TOKEN_CHAR_LITERAL, buffer, start_line, start_column);
        token->value.char_value = ch;
        
        if (lexer->current_char == '\'') {
            lexer_advance(lexer);
        } else {
            LOG_WARN("Missing closing quote for character literal");
        }
        return token;
    }
    
    LOG_WARN("Empty character literal");
    return create_token(TOKEN_UNKNOWN, "'", start_line, start_column);
}

static Token *lexer_read_string_literal(Lexer *lexer) {
    int start_line = lexer->line;
    int start_column = lexer->column;
    char buffer[1024];
    int i = 0;
    
    buffer[i++] = '"';
    lexer_advance(lexer); // skip opening "
    
    while (lexer->current_char != '"' && lexer->current_char != '\n' && 
           lexer->current_char != EOF && i < 1022) {
        if (lexer->current_char == '\\') {
            buffer[i++] = '\\';
            lexer_advance(lexer);
            if (lexer->current_char != EOF && lexer->current_char != '\n') {
                buffer[i++] = lexer->current_char;
                lexer_advance(lexer);
            }
        } else {
            buffer[i++] = lexer->current_char;
            lexer_advance(lexer);
        }
    }
    
    if (lexer->current_char == '"') {
        buffer[i++] = '"';
        lexer_advance(lexer);
    } else {
        LOG_WARN("Unterminated string literal");
    }
    
    buffer[i] = '\0';
    return create_token(TOKEN_STRING_LITERAL, buffer, start_line, start_column);
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
    lexer_skip_line_directive(lexer);
    lexer_skip_whitespace(lexer);
    
    // LOG_TRACE("lexer_next_token: current char='%c' (0x%02X) at %d:%d", 
    //           lexer->current_char, (unsigned char)lexer->current_char, lexer->line, lexer->column);
    
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
    
    if (lexer->current_char == '\'') {
        return lexer_read_char_literal(lexer);
    }
    
    if (lexer->current_char == '"') {
        return lexer_read_string_literal(lexer);
    }
    
    char ch = lexer->current_char;
    lexer_advance(lexer);
    
    switch (ch) {
        case '+':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_PLUS_ASSIGN, "+=", line, column);
            } else if (lexer->current_char == '+') {
                lexer_advance(lexer);
                return create_token(TOKEN_INCREMENT, "++", line, column);
            }
            return create_token(TOKEN_PLUS, "+", line, column);
        case '-':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_MINUS_ASSIGN, "-=", line, column);
            } else if (lexer->current_char == '-') {
                lexer_advance(lexer);
                return create_token(TOKEN_DECREMENT, "--", line, column);
            }
            return create_token(TOKEN_MINUS, "-", line, column);
        case '*':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_STAR_ASSIGN, "*=", line, column);
            }
            return create_token(TOKEN_STAR, "*", line, column);
        case '/':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_SLASH_ASSIGN, "/=", line, column);
            }
            return create_token(TOKEN_SLASH, "/", line, column);
        case '%': return create_token(TOKEN_PERCENT, "%", line, column);
        case '(': return create_token(TOKEN_LPAREN, "(", line, column);
        case ')': return create_token(TOKEN_RPAREN, ")", line, column);
        case '{': return create_token(TOKEN_LBRACE, "{", line, column);
        case '}': return create_token(TOKEN_RBRACE, "}", line, column);
        case ';': return create_token(TOKEN_SEMICOLON, ";", line, column);
        case ',': return create_token(TOKEN_COMMA, ",", line, column);
        case '[': return create_token(TOKEN_LBRACKET, "[", line, column);
        case ']': return create_token(TOKEN_RBRACKET, "]", line, column);
        case '&':
            if (lexer->current_char == '&') {
                lexer_advance(lexer);
                return create_token(TOKEN_AND, "&&", line, column);
            }
            return create_token(TOKEN_AMPERSAND, "&", line, column);
        case '.':
            // Check for ellipsis (...)
            if (lexer->current_char == '.') {
                lexer_advance(lexer); // consume second dot
                if (lexer->current_char == '.') {
                    lexer_advance(lexer); // consume third dot  
                    return create_token(TOKEN_ELLIPSIS, "...", line, column);
                } else {
                    // Not ellipsis, return single dot and backup
                    ungetc(lexer->current_char, lexer->input);
                    lexer->current_char = '.';
                    lexer->column--;
                    return create_token(TOKEN_DOT, ".", line, column);
                }
            }
            return create_token(TOKEN_DOT, ".", line, column);
        case ':': return create_token(TOKEN_COLON, ":", line, column);
        case '=':
            // LOG_TRACE("Found '=' at %d:%d, next char is '%c'", line, column, lexer->current_char);
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
            return create_token(TOKEN_NOT, "!", line, column);
        case '<':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_LE, "<=", line, column);
            } else if (lexer->current_char == '<') {
                lexer_advance(lexer);
                return create_token(TOKEN_LSHIFT, "<<", line, column);
            }
            return create_token(TOKEN_LT, "<", line, column);
        case '>':
            if (lexer->current_char == '=') {
                lexer_advance(lexer);
                return create_token(TOKEN_GE, ">=", line, column);
            } else if (lexer->current_char == '>') {
                lexer_advance(lexer);
                return create_token(TOKEN_RSHIFT, ">>", line, column);
            }
            return create_token(TOKEN_GT, ">", line, column);
        case '|':
            if (lexer->current_char == '|') {
                lexer_advance(lexer);
                return create_token(TOKEN_OR, "||", line, column);
            }
            return create_token(TOKEN_PIPE, "|", line, column);
        case '^':
            return create_token(TOKEN_CARET, "^", line, column);
        case '~':
            return create_token(TOKEN_TILDE, "~", line, column);
        case '?':
            return create_token(TOKEN_QUESTION, "?", line, column);
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
        "EOF", "INT_LITERAL", "FLOAT_LITERAL", "CHAR_LITERAL", "STRING_LITERAL", "IDENTIFIER", 
        "IF", "ELSE", "WHILE", "DO", "FOR", "BREAK", "CONTINUE", "RETURN", "INT", "CHAR", "FLOAT", "DOUBLE", "VOID", "STRUCT", "UNION", "SIZEOF",
        "SWITCH", "CASE", "DEFAULT", "TYPEDEF", "ENUM", "STATIC", "CONST", "EXTERN", "COLON", "AND", "OR", "NOT",
        "PLUS", "MINUS", "STAR", "SLASH", "PERCENT", "LPAREN", "RPAREN", "LBRACE", "RBRACE",
        "SEMICOLON", "ASSIGN", "EQ", "NE", "LT", "GT", "LE", "GE", "COMMA", 
        "LBRACKET", "RBRACKET", "AMPERSAND", "DOT", "PIPE", "CARET", "TILDE", 
        "LSHIFT", "RSHIFT", "PLUS_ASSIGN", "MINUS_ASSIGN", "STAR_ASSIGN", "SLASH_ASSIGN", 
        "INCREMENT", "DECREMENT", "QUESTION", "ELLIPSIS", "UNKNOWN"
    };
    return names[type];
}
