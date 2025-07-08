#ifndef STDLIB_H
#define STDLIB_H

// Memory allocation
void *malloc(int size);
void free(void *ptr);

// Process control
void exit(int status);

// Conversions
int atoi(const char *str);

#endif // STDLIB_H