
# 头文件目录
INCLUDE_PATH := include cargparser/include
# 源文件目录
SOURCE_PATH  := src cargparser/src
# 编译生成的目录
BUILD_PATH := build
# 可执行文件
BIN  := server client cfrp

# 所有源文件
ALL_INCLUDE := $(foreach file,$(INCLUDE_PATH),-I$(file))
ALL_SOURCES := $(foreach file, $(SOURCE_PATH), $(wildcard $(file)/*.c))
ALL_OBJ := $(foreach file, $(ALL_SOURCES), $(BUILD_PATH)/obj/$(file).o)

# 命令
CC := gcc $(ALL_INCLUDE) -std=c99

all: build

$(ALL_OBJ) : $(BUILD_PATH)/obj/%.o : % 
	$(CC) -c $^ -o $@

% : mkpath $(ALL_OBJ) %.c config.h
	$(CC) $(ALL_OBJ) $@.c -o $(BUILD_PATH)/bin/$@

%.run: %.c mkpath $(ALL_OBJ)
	$(CC) $(ALL_OBJ) $< -o $(BUILD_PATH)/bin/$(notdir $@)

.PHONY: build
build: config.h $(BIN)
	@echo Build success!
	@echo build path: $(BUILD_PATH)/bin

.PHONY: mkpath
mkpath:
	@mkdir -p $(patsubst %, $(BUILD_PATH)/obj/%,$(SOURCE_PATH)) $(BUILD_PATH)/bin

.PHONY:clean
clean:	
	@rm -rfv ./$(BUILD_PATH)

config.h:
	cp include/config.def.h	config.h