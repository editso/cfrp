/*
 * @Author: your name
 * @Date: 1970-01-01 08:00:00
 * @LastEditTime: 2021-01-29 10:31:22
 * @LastEditors: Please set LastEditors
 * @Description: In User Settings Edit
 * @FilePath: /cfrp/include/logger.h
 */
#ifndef __LOGGER_H__
#define __LOGGER_H__
#include <stdarg.h>

enum{
    DEBUG,
    INFO,
    WARN,
    ERROR
};



// error
#define LOG_ERROR(message, ...) log_out(message, ERROR, __FILE__, __func__, __LINE__, ##__VA_ARGS__)

// info
#define LOG_INFO(message, ...) log_out(message, INFO, __FILE__, __func__, __LINE__, ##__VA_ARGS__)

//  debug
#define LOG_DEBUG(message, ...) log_out(message, DEBUG, __FILE__, __func__, __LINE__, ##__VA_ARGS__)

// warning
#define LOG_WARN(message, ...) log_out(message, WARN, __FILE__, __func__, __LINE__, ##__VA_ARGS__)

extern void log_out(const char* message, const int level, const char *file,const char *func, const int line, ...);

#endif