# tool macros
CC := gcc
CCFLAGS := -O2
DBGFLAGS := -Wall -Werror -g
LIBS := -L./lib/ -lm -lpthread -ldl -lrt -lsoundio -lasound -lpulse -lpulse-simple -lpush -lcomm -lusb-1.0 

# path macros
BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
DBG_PATH := debug

# compile macros
TARGET_NAME := synth
ifeq ($(OS),Windows_NT)
	TARGET_NAME := $(addsuffix .exe,$(TARGET_NAME))
endif

TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)

# discover all .c files recursively
SRC := $(shell find $(SRC_PATH) -type f -name '*.c')

# convert src path → obj path (mirror structure)
OBJ := $(patsubst $(SRC_PATH)/%.c, $(OBJ_PATH)/%.o, $(SRC))
OBJ_DEBUG := $(patsubst $(SRC_PATH)/%.c, $(DBG_PATH)/%.o, $(SRC))

# default
default: makedir all

# build target
$(TARGET): $(OBJ)
	$(CC) $(CCFLAGS) -o $@ $(OBJ) $(LIBS)

$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CC) $(CCFLAGS) $(DBGFLAGS) -o $@ $(OBJ_DEBUG) $(LIBS)

# generic compile rules (with auto mkdir)
$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CCFLAGS) -c $< -o $@

$(DBG_PATH)/%.o: $(SRC_PATH)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CCFLAGS) $(DBGFLAGS) -c $< -o $@

# phony targets
.PHONY: makedir clean distclean all debug

makedir:
	mkdir -p $(BIN_PATH) $(OBJ_PATH) $(DBG_PATH)

all: $(TARGET)
debug: $(TARGET_DEBUG)

clean:
	rm -rf $(OBJ_PATH) $(DBG_PATH) $(BIN_PATH)/$(TARGET_NAME)

distclean:
	rm -rf $(OBJ_PATH) $(DBG_PATH) $(BIN_PATH)
