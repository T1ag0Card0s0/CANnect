VERSION = 0.1.0
PROJECT_NAME = cannect
BUILD_DIR = build
OBJS_DIR = $(BUILD_DIR)/objs
LIB_DIR = $(BUILD_DIR)/lib
LIB = $(LIB_DIR)/lib$(PROJECT_NAME).a

CXX := g++
# Log level 0=Debug ... 4=Off
CXXFLAGS := -std=c++17 -O2 -MMD -MP -Wall -Wextra -Wpedantic -Werror -Iinclude -DLOG_LEVEL=0

TARGET = $(BUILD_DIR)/$(PROJECT_NAME)

LIB_SRCS := $(filter-out src/main.cpp, $(wildcard src/*.cpp))
LIB_OBJS := $(patsubst src/%.cpp, $(OBJS_DIR)/%.o, $(LIB_SRCS))
MAIN_OBJ := $(OBJS_DIR)/main.o $(OBJS_DIR)/CanDispatcher.o $(OBJS_DIR)/Logger.o $(OBJS_DIR)/SocketCanInterface.o $(OBJS_DIR)/Cannect.o

all: $(TARGET)

$(LIB): $(LIB_OBJS) | $(LIB_DIR)
	ar rcs $@ $^

$(TARGET): $(MAIN_OBJ) $(LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJS_DIR)/%.o: src/%.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR) $(OBJS_DIR) $(LIB_DIR):
	mkdir -p $@

-include $(wildcard $(OBJS_DIR)/*.d)

clean:
	rm -rf $(BUILD_DIR)

test: all
	$(MAKE) -C tests run

export LIB
export PROJECT_NAME

.PHONY: all clean test

