project("rpr")
include_directories(include)

add_executable(rpr server.cpp include/logging.h include/rpr.h src/logging.cpp src/rpr.cpp)