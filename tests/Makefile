ROOT_DIR = ..
BUILD_DIR = build
OBJS_DIR = $(BUILD_DIR)/objs
BINS_DIR = $(BUILD_DIR)
COVERAGE_DIR = $(BUILD_DIR)/coverage
COVERAGE_INFO = $(COVERAGE_DIR)/coverage.info

PROJECT_NAME ?= cannect
LIB := $(ROOT_DIR)/build/lib/lib$(PROJECT_NAME).a

CXX := g++
BASE_CXXFLAGS := -std=c++17 -MMD -MP -Wall -Wextra -Wpedantic -Werror
BASE_CXXFLAGS += -I$(ROOT_DIR)/include

CXXFLAGS := $(BASE_CXXFLAGS) -O2
LDFLAGS := -pthread

COVERAGE_CXXFLAGS := $(BASE_CXXFLAGS) -O0 --coverage
COVERAGE_LDFLAGS := --coverage -pthread

ROOT_COVERAGE_CXXFLAGS := -std=c++17 -O0 --coverage -MMD -MP -Wall -Wextra -Wpedantic -Werror -Iinclude -DLOG_LEVEL=3

TEST_SRCS := $(wildcard *.cpp)
TEST_OBJS := $(patsubst %.cpp,$(OBJS_DIR)/%.o,$(TEST_SRCS))
TEST_BINS := $(patsubst %.cpp,$(BINS_DIR)/%,$(TEST_SRCS))

all: $(TEST_BINS)

$(BINS_DIR)/%: $(OBJS_DIR)/%.o $(LIB) | $(BINS_DIR)
	$(CXX) $(CXXFLAGS) $< $(LIB) -o $@ $(LDFLAGS)

$(OBJS_DIR)/%.o: %.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJS_DIR) $(BINS_DIR) $(COVERAGE_DIR):
	mkdir -p $@

-include $(wildcard $(OBJS_DIR)/*.d)

run: all
	@for bin in $(TEST_BINS); do \
		echo ""; \
		echo ">>> Running $$bin"; \
		./$$bin; \
	done

coverage_clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C $(ROOT_DIR) clean

coverage_build:
	$(MAKE) -C $(ROOT_DIR) CXXFLAGS='$(ROOT_COVERAGE_CXXFLAGS)'
	$(MAKE) clean
	$(MAKE) CXXFLAGS='$(COVERAGE_CXXFLAGS)' LDFLAGS='$(COVERAGE_LDFLAGS)' all

coverage_run: coverage_build
	@for bin in $(TEST_BINS); do \
		echo ""; \
		echo ">>> Running $$bin"; \
		./$$bin; \
	done

coverage_html: coverage_run | $(COVERAGE_DIR)
	lcov --capture --directory $(ROOT_DIR)/build --directory $(BUILD_DIR) --output-file $(COVERAGE_INFO) --ignore-errors empty
	lcov --remove $(COVERAGE_INFO) '/usr/*' '*/tests/*' --output-file $(COVERAGE_INFO)
	genhtml $(COVERAGE_INFO) --output-directory $(COVERAGE_DIR)/html
	@echo ""
	@echo "Coverage report:"
	@echo "  $(COVERAGE_DIR)/html/index.html"

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run clean coverage_clean coverage_build coverage_run coverage_html