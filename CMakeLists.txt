cmake_minimum_required(VERSION 3.17)
project(cfrp)
include_directories(include)



add_executable(server
        server.c
        src/clist.c
        src/cfrp.c
        src/cbuff.c
        src/cmap.c
        src/logger.c
        src/cqueue.c)

