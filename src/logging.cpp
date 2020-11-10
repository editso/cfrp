#include "logging.h"
#include <iostream>
#include <error.h>
#include <errno.h>
#include <string.h>
using namespace std;


class SimpleLogging: public Log{
    log_level level;
    string name;

    public:
        SimpleLogging(string &name, log_level level){
            this->name = name;
            this->level = level;
        }

        void log(const string &message, log_level level){
            cout <<
            this->name <<
            "-" <<
            message << endl;
        }

        void error(const string &message) override {
            log("[ERROR]: " + message + ", errno: " + to_string(errno) + ", msg: " + strerror(errno), ERROR);
        }

        void info(const string &message) override {
            log("[INFO]: " + message, INFO);
        }

        void warn(const string &message) override {
            log("[WARNING]: " + message, WARNING);
        }

        void debug(const string &message) override {
            log("[DEBUG]: "+message, DEBUG);
        }
};


Log* get_log(string name, log_level level){
    return new SimpleLogging(name, level);
}
