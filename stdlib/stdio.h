#ifndef STDIO_H
#define STDIO_H

// Basic I/O functions
int putchar(int c);
int getchar(void);
int puts(const char *s);

// Simple printf (very limited)
int printf(const char *format, ...);

#endif // STDIO_H