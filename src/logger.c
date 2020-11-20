#include "logger.h"
#include "stdio.h"

/**
 * 简单输出...
*/
extern void log_out(const char* message, const int level, const char *file,const char *func, const int line, ...){    
    va_list arg;
    va_start(arg, line);
    char buff[1024];
    vsnprintf(buff, sizeof(buff), message, arg);
    va_end(arg);
    printf("%s:%d[%s]: %s\n", file, line, func, buff);
}