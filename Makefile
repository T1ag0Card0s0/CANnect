VERSION = 0.1.0
PROJECT_NAME = cannect

BUILD_DIR = build
OBJS_DIR = $(BUILD_DIR)/objs

CXX := g++
CXXFLAGS := -std=c++17 -O2 -MMD -MP -Wall -Wextra -Wpedantic -Werror

OBJS = $(OBJS_DIR)/main.o 
TARGET = $(BUILD_DIR)/$(PROJECT_NAME)

CXXFLAGS += -Iinclude

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@

$(OBJS_DIR):
	mkdir -p $@

$(OBJS_DIR)/%.o: src/%.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJS_DIR) $(TARGET)

.PHONY: all clean
