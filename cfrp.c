//
// Created by zy on 2020/11/30.
//
#include "cfrp.h"
#include "cargparser.h"


extern void default_cmd(cargparser_args *args);

extern void server_cmd(cargparser_args *args);

extern void client_cmd(cargparser_args *args);

int main(int argc, char **argv){
    cargparser_args args;
    struct option options[] = {
        {.name = "port", .keyword = {"-p","--port"}, .description = "端口号", .require = 1, .defaults=8080},
        {.name = "host", .keyword = {"-h","--host"}, .description = "主机地址", .require = 1, .defaults="0.0.0.0"},
        {.name = "mapping-port", .keyword = {"-mp","--mapping-port"}, .description = "映射端口", .require = 1, .defaults=8080},
        {.name = "mapping-host", .keyword = {"-mh","--mapping-host"}, .description = "映射主机", .require = 1, .defaults="0.0.0.0"},
    };
    cargparser sub[] = {
        {argv[0], 0, 0, 0, 0, server_cmd, "cfrp server"},
        {argv[0], 0, 0, 0, 0, client_cmd, "cfrp client"},
    };
    cargparser parser = {argv[0], options, len(options), sub, len(sub), default_cmd, "cfrp proxy"};
    parse_main(0, &args, argc, argv);
    cargparser_call(&args);
    return 0;
}

extern void default_cmd(cargparser_args *args){
    
}

extern void server_cmd(cargparser_args *args){

}

extern void client_cmd(cargparser_args *args){
    
}

