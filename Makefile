INCLUDE_PATH = include

SOURCE_PATH = src

OBJ_PATH = build/object

BIN_PATH = build/bin

ALL_CPP = $(wildcard $(SOURCE_PATH)/*.cpp)
ALL_C = $(wildcard $(SOURCE_PATH)/*.c)
ALL_H = $(wildcard $(INCLUDE_PATH)/*.h)
ALL_OBJ = $(patsubst $(SOURCE_PATH)/%.c, $(OBJ_PATH)/%.o, $(ALL_C))

CPP = g++ -I$(INCLUDE_PATH)
CC = gcc -I$(INCLUDE_PATH) -std=c99 -g

.PHONY:clean

all: mkpath $(ALL_OBJ)

$(ALL_OBJ): $(OBJ_PATH)/%.o : $(SOURCE_PATH)/%.c
	$(CC) -c $^ -o $@

mkpath:
	mkdir -p $(OBJ_PATH) $(BIN_PATH)

%:all %.c config.h
	$(CC) $@.c $(ALL_OBJ) -o $(BIN_PATH)/$@

config.h:
	cp $(INCLUDE_PATH)/config.def.h config.h

clean:
	rm -rf *.out $(OBJ_PATH) $(BIN_PATH)

