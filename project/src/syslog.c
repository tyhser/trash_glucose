#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "usart.h"
#include "syslog.h"
static const char *print_level_table[] = { "D", "I", "W", "E" };

#define change_level_to_string(level) \
  ((level) - PRINT_LEVEL_DEBUG <= PRINT_LEVEL_ERROR) ? print_level_table[level] : "debug"

static void vprint_module_log(const char *func,
                       int line,
                       print_level_t level,
                       const char *message,
                       va_list list)
{
        printf("[%s/ F: %s L: %d]: ",
               change_level_to_string(level),
               func,
               line);
    vprintf(message, list);
    printf("\n");
}

void print_module_log(const char *func,
                      int line,
                      print_level_t level,
                      const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    vprint_module_log(func, line, level, message, ap);
    va_end(ap);
}

void log_error(const char *func, int line, const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    vprint_module_log(func, line, PRINT_LEVEL_ERROR, message, ap);
    va_end(ap);
}

void log_warning(const char *func, int line, const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    vprint_module_log(func, line, PRINT_LEVEL_WARNING, message, ap);
    va_end(ap);
}

void log_info(const char *func, int line, const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    vprint_module_log(func, line, PRINT_LEVEL_INFO, message, ap);
    va_end(ap);
}

void log_debug(const char *func, int line, const char *message, ...)
{
    va_list ap;
    va_start(ap, message);
    vprint_module_log(func, line, PRINT_LEVEL_DEBUG, message, ap);
    va_end(ap);
}

void hex_dump(const char *name, const char *data, int length)
{
    int index = 0;
    printf("%s:\n", name);
    for (index = 0; index < length; index++) {
        printf("%02X", (int)(data[index]));
        if ((index + 1) % 16 == 0) {
            printf("\n");
            continue;
        }
        if (index + 1 != length) {
            printf(" ");
        }
    }
    if (0 != index && 0 != index % 16) {
        printf("\n");//add one more blank line
    }
}
