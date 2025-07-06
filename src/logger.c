#include "logger.h"
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static FILE *log_file = NULL;
static LogLevel current_level = LOG_INFO;
static const char *level_strings[] = {
    "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

static const char *level_colors[] = {
    "\x1b[37m",  // TRACE - White
    "\x1b[36m",  // DEBUG - Cyan
    "\x1b[32m",  // INFO  - Green
    "\x1b[33m",  // WARN  - Yellow
    "\x1b[31m",  // ERROR - Red
    "\x1b[35m"   // FATAL - Magenta
};

void log_init(const char *filename, LogLevel min_level) {
    current_level = min_level;
    
    if (filename) {
        log_file = fopen(filename, "a");
        if (!log_file) {
            fprintf(stderr, "Failed to open log file: %s\n", filename);
        }
    }
}

void log_set_level(LogLevel level) {
    if (level >= LOG_TRACE && level <= LOG_FATAL) {
        current_level = level;
    }
}

void log_message(LogLevel level, const char *file, int line, const char *fmt, ...) {
    if (level < current_level) {
        return;
    }
    
    time_t now;
    time(&now);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';  // Remove newline
    
    const char *filename = strrchr(file, '/');
    if (filename) {
        filename++;
    } else {
        filename = file;
    }
    
    va_list args;
    
    // Log to stderr (with colors if terminal)
    if (isatty(fileno(stderr))) {
        fprintf(stderr, "%s[%s] [%s] %s:%d - ", 
                level_colors[level], time_str, level_strings[level], filename, line);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\x1b[0m\n");  // Reset color
    } else {
        fprintf(stderr, "[%s] [%s] %s:%d - ", 
                time_str, level_strings[level], filename, line);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
    
    // Log to file if available
    if (log_file) {
        fprintf(log_file, "[%s] [%s] %s:%d - ", 
                time_str, level_strings[level], filename, line);
        va_start(args, fmt);
        vfprintf(log_file, fmt, args);
        va_end(args);
        fprintf(log_file, "\n");
        fflush(log_file);
    }
    
    if (level == LOG_FATAL) {
        log_cleanup();
        exit(1);
    }
}

void log_cleanup(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
