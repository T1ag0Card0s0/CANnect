CXX ?= g++
AR ?= ar

ROOT_DIR := ..
BUILD_DIR := $(ROOT_DIR)/build
TEST_BUILD_DIR := $(BUILD_DIR)/tests

LIB_TARGET := $(BUILD_DIR)/libcannect.a

INCLUDES := -I$(ROOT_DIR)/include -Itests_api

GCOV_FLAGS := --coverage
CXXFLAGS ?= -O0 -g -Wall -Wextra $(INCLUDES)
SRC_COVERAGE_FLAGS := $(addprefix -I,$(ROOT_DIR)/src) $(GCOV_FLAGS)
LDFLAGS  ?=

TEST_SRCS := core/test_cansocket.cpp
TEST_API_SRC := tests_api/tests_api.cpp

TEST_BINS := $(patsubst %.cpp,$(TEST_BUILD_DIR)/%,$(TEST_SRCS))

all: $(TEST_BINS)

$(TEST_BUILD_DIR):
	@mkdir -p $@

$(TEST_BUILD_DIR)/%: %.cpp $(TEST_API_SRC) | $(TEST_BUILD_DIR) $(LIB_TARGET)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(GCOV_FLAGS) $< $(TEST_API_SRC) $(LIB_TARGET) $(LDFLAGS) -o $@

test: all run

run: $(TEST_BINS)
	@set -e; \
	for t in $(TEST_BINS); do \
		echo "==> $$t"; \
		$$t; \
	done

list:
	@echo "$(TEST_BINS)"

clean:
	rm -rf $(TEST_BUILD_DIR)
	rm -rf ../build/tests
	rm -rf *.gcda *.gcno *.info coverage.info* coverage-report

.PHONY: all test run clean list
