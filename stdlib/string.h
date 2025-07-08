#ifndef STRING_H
#define STRING_H

// String operations
int strlen(const char *s);
char *strcpy(char *dest, const char *src);
int strcmp(const char *s1, const char *s2);
char *strcat(char *dest, const char *src);

// Memory operations
void *memcpy(void *dest, const void *src, int n);
void *memset(void *s, int c, int n);

#endif // STRING_H