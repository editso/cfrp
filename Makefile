INCLUDE_PATH = include
SOURCE_PATH = src
OBJ_PATH = object
BIN_PATH = bin

ALL_CPP = $(wildcard $(SOURCE_PATH)/*.cpp)
ALL_C = $(wildcard $(SOURCE_PATH)/*.c)
ALL_H = $(wildcard $(INCLUDE_PATH)/*.h)
ALL_OBJ = $(patsubst $(SOURCE_PATH)/%.cpp, $(OBJ_PATH)/%.o, $(ALL_CPP))


CPP = g++ -I$(INCLUDE_PATH)
CC = gcc -I$(INCLUDE_PATH)

all: mkpath $(ALL_OBJ)


$(ALL_OBJ): $(OBJ_PATH)/%.o : $(SOURCE_PATH)/%.cpp
	$(CPP) -c $^ -o $@

mkpath:
	mkdir -p $(OBJ_PATH) $(BIN_PATH)

%:all %.cpp config.h
	$(CPP) $@.cpp $(ALL_OBJ) -o $(BIN_PATH)/$@ 

config.h:
	cp $(INCLUDE_PATH)/config.def.h config.h

.PHONY:clean
clean:
	rm -rf *.out $(OBJ_PATH) $(BIN_PATH)

