#ifndef __LOGGING_H__
#define __LOGGING_H__
#include <iostream>
#include <string>
using namespace std;

typedef enum log_level{
    INFO,
    DEBUG,
    ERROR,
    WARNING
}log_level;


class Log{ 
    public:
        virtual void error(const string &message) = 0;
        virtual void info(const string &message) = 0;
        virtual void warn(const string &message) = 0;
        virtual void debug(const string &message) = 0;
};

extern Log* get_log(string, log_level);


#define LOG_INIT() get_log(__FILE__, DEBUG)


#define logger get_log(__FILE__, DEBUG)


#endif