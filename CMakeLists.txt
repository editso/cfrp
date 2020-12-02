cmake_minimum_required(VERSION 3.17)
project(cfrp)
include_directories(include)

file(GLOB SOURCES src/*.c)

add_executable(server server.c ${SOURCES})

add_executable(client client.c ${SOURCES})

add_executable(cfrp cfrp.c ${SOURCES})

add_executable(test test.c ${SOURCES})