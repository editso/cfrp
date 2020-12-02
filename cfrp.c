//
// Created by zy on 2020/11/30.
//
#include "cfrp.h"
#include "cargparser.h"


int main(int argc, char **argv){
    cargparser_args args;
    cargparser parser = {argv[0], 0, 0, 0, 0, 0, "cfrp"};
    parse_main(0, &args, argc, argv);
    cargparser_call(&args);
    return 0;
}
