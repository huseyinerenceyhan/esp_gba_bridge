#include "printdebug.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <tonc_nocash.h>



void print(const char *msg, ...) {
    char buffer[80];
    va_list args;
    va_start(args, msg);
    vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end(args);
    nocash_puts(buffer);
}