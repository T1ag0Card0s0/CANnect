ROOT_DIR = ..
BUILD_DIR = build
OBJS_DIR = $(BUILD_DIR)/objs
BINS_DIR = $(BUILD_DIR)

PROJECT_NAME ?= cannect
LIB := $(ROOT_DIR)/build/lib/lib$(PROJECT_NAME).a

CXX := g++
CXXFLAGS := -std=c++17 -O2 -MMD -MP -Wall -Wextra -Wpedantic -Werror
CXXFLAGS += -I$(ROOT_DIR)/include

TEST_SRCS := $(wildcard *.cpp)
TEST_OBJS := $(patsubst %.cpp, $(OBJS_DIR)/%.o, $(TEST_SRCS))
TEST_BINS := $(patsubst %.cpp, $(BINS_DIR)/%, $(TEST_SRCS))

all: $(TEST_BINS)

$(BINS_DIR)/%: $(OBJS_DIR)/%.o | $(BINS_DIR)
	$(CXX) $(CXXFLAGS) $< $(LIB) -o $@ -pthread

$(OBJS_DIR)/%.o: %.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJS_DIR) $(BINS_DIR):
	mkdir -p $@

-include $(wildcard $(OBJS_DIR)/*.d)

run: all
	@for bin in $(TEST_BINS); do \
		echo ""; \
		echo ">>> Running $$bin"; \
		$$bin; \
	done

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean


