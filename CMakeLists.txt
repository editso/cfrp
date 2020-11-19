cmake_minimum_required(VERSION 3.17)
project(cfrp)
include_directories(include)

set(    sources
        src/clist.c
        src/cfrp.c
        src/cbuff.c
        src/cmap.c
        src/logger.c
        src/cqueue.c
        src/clib.c
        )

add_executable(
        server
        server.c
        ${sources})

add_executable(
        client
        client.c
        ${sources}
)

add_executable(
        test
        test.c
        ${sources}
)