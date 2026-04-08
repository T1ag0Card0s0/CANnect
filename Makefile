VERSION = 0.1.0
PROJECT_NAME = cannect

BUILD_DIR = build
OBJS_DIR = $(BUILD_DIR)/objs
LIB_DIR = $(BUILD_DIR)/lib
LIB = $(LIB_DIR)/lib$(PROJECT_NAME).a

RELEASE_DIR = $(BUILD_DIR)/release
RELEASE_NAME = $(PROJECT_NAME)-$(VERSION)
RELEASE_ROOT = $(RELEASE_DIR)/$(RELEASE_NAME)
RELEASE_PKG = $(BUILD_DIR)/$(RELEASE_NAME).tar.gz

CXX := g++
AR := ar
RM := rm -f

# Log level 0=Debug ... 4=Off
CXXFLAGS := -std=c++17 -O2 -MMD -MP -Wall -Wextra -Wpedantic -Werror -Iinclude -DLOG_LEVEL=3

TARGET = $(BUILD_DIR)/$(PROJECT_NAME)

LIB_OBJS := \
	$(OBJS_DIR)/CanDispatcher.o \
	$(OBJS_DIR)/Logger.o \
	$(OBJS_DIR)/SocketCanInterface.o \
	$(OBJS_DIR)/Cannect.o \
	$(OBJS_DIR)/cants/CanTsProtocol.o \
	$(OBJS_DIR)/cants/SetBlockManager.o \
	$(OBJS_DIR)/cants/GetBlockManager.o

APP_OBJS := \
	$(OBJS_DIR)/app/main.o \
	$(OBJS_DIR)/app/Cli.o \
	$(OBJS_DIR)/app/Handlers.o

DEPS := $(LIB_OBJS:.o=.d) $(APP_OBJS:.o=.d)

all: $(TARGET)

$(LIB): $(LIB_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(TARGET): $(APP_OBJS) $(LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJS_DIR)/%.o: src/%.cpp | $(OBJS_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR) $(OBJS_DIR) $(LIB_DIR) $(RELEASE_DIR):
	@mkdir -p $@

-include $(DEPS)

release: $(RELEASE_PKG)
	@echo "Release package ready: $(RELEASE_PKG)"

$(RELEASE_PKG): $(LIB) | $(RELEASE_DIR)
	@echo "Staging release $(RELEASE_NAME)..."
	$(RM) $(RELEASE_PKG)
	rm -rf $(RELEASE_ROOT)
	mkdir -p $(RELEASE_ROOT)/lib
	mkdir -p $(RELEASE_ROOT)/demo/app
	mkdir -p $(RELEASE_ROOT)/include/cannect

	cp -r include $(RELEASE_ROOT)/
	cp $(LIB) $(RELEASE_ROOT)/lib/
	cp src/app/main.cpp $(RELEASE_ROOT)/demo/app/
	cp src/app/Cli.cpp $(RELEASE_ROOT)/demo/app/
	cp src/app/Handlers.cpp $(RELEASE_ROOT)/demo/app/
	cp src/app/Cli.hpp $(RELEASE_ROOT)/demo/app/
	cp src/app/Handlers.hpp $(RELEASE_ROOT)/demo/app/
	cp LICENSE $(RELEASE_ROOT)/LICENSE
	cp README.md $(RELEASE_ROOT)/README.md

	@printf '%s\n' \
		'CXX ?= g++' \
		'CXXFLAGS ?= -std=c++17 -O2 -I../include' \
		'' \
		'TARGET = demo' \
		'SRC = app/main.cpp app/Cli.cpp app/Handlers.cpp' \
		'LIB = ../lib/lib$(PROJECT_NAME).a' \
		'' \
		'all: $$(TARGET)' \
		'' \
		'$$(TARGET): $$(SRC) $$(LIB)' \
		'	$$(CXX) $$(CXXFLAGS) $$(SRC) $$(LIB) -o $$(TARGET)' \
		'' \
		'clean:' \
		'	rm -f $$(TARGET)' \
		'' \
		'.PHONY: all clean' \
	> $(RELEASE_ROOT)/demo/Makefile

	tar -czf $(RELEASE_PKG) -C $(RELEASE_DIR) $(RELEASE_NAME)
	@echo "SHA256: $$(sha256sum $(RELEASE_PKG))"

clean:
	rm -rf $(BUILD_DIR)
	$(MAKE) -C tests clean

test:
	$(MAKE) -C tests run

coverage:
	$(MAKE) -C tests coverage_html

export LIB
export PROJECT_NAME

.PHONY: all clean test release
