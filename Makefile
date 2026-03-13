VERSION = 0.1.0
PROJECT_NAME = cannect
BUILD_DIR = build
OBJS_DIR = $(BUILD_DIR)/objs
LIB_DIR = $(BUILD_DIR)/lib
LIB = $(LIB_DIR)/lib$(PROJECT_NAME).a
RELEASE_DIR = $(BUILD_DIR)/release
RELEASE_NAME = $(PROJECT_NAME)-$(VERSION)
RELEASE_PKG = $(BUILD_DIR)/$(RELEASE_NAME).tar.gz

CXX := g++
# Log level 0=Debug ... 4=Off
CXXFLAGS := -std=c++17 -O2 -MMD -MP -Wall -Wextra -Wpedantic -Werror -Iinclude -DLOG_LEVEL=3

TARGET = $(BUILD_DIR)/$(PROJECT_NAME)

LIB_OBJS := $(OBJS_DIR)/CanDispatcher.o $(OBJS_DIR)/Logger.o $(OBJS_DIR)/SocketCanInterface.o $(OBJS_DIR)/Cannect.o $(OBJS_DIR)/CanTsProtocol.o
MAIN_OBJ := $(OBJS_DIR)/main.o 
all: $(TARGET)

$(LIB): $(LIB_OBJS) | $(LIB_DIR)
	ar rcs $@ $^

$(TARGET): $(MAIN_OBJ) $(LIB) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJS_DIR)/%.o: src/%.cpp | $(OBJS_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR) $(OBJS_DIR) $(LIB_DIR) $(RELEASE_DIR):
	mkdir -p $@

-include $(wildcard $(OBJS_DIR)/*.d)

release: $(RELEASE_PKG)
	@echo "Release package ready: $(RELEASE_PKG)"
 
$(RELEASE_PKG): $(LIB) | $(RELEASE_DIR)
	@echo "Staging release $(RELEASE_NAME)..."
	$(RM) -rf $(RELEASE_DIR)/$(RELEASE_NAME)
	mkdir -p $(RELEASE_DIR)/$(RELEASE_NAME)/lib
	cp -r include $(RELEASE_DIR)/$(RELEASE_NAME)/include
	cp $(LIB) $(RELEASE_DIR)/$(RELEASE_NAME)/lib/
	cp LICENSE $(RELEASE_DIR)/$(RELEASE_NAME)/LICENSE
	cp README.md $(RELEASE_DIR)/$(RELEASE_NAME)/README.md
	tar -czf $(RELEASE_PKG) -C $(RELEASE_DIR) $(RELEASE_NAME)
	@echo "SHA256: $$(sha256sum $(RELEASE_PKG))"

clean:
	rm -rf $(BUILD_DIR)

test: all
	$(MAKE) -C tests run

export LIB
export PROJECT_NAME

.PHONY: all clean test

