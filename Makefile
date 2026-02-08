export VERSION = 0.0.1
LIB_TARGET := libcannect.a
EXE_TARGET := cannect
BUILD_DIR := build
DIST_DIR := $(BUILD_DIR)/dist
SRC_DIR := src
SRCS := Cannect.cpp \
		core/SocketCanTransport.cpp \
		core/CanFrame.cpp core/CanDispatcher.cpp \
		core/CanListener.cpp \
		core/CanSender.cpp \
		core/cants/CanTsProtocol.cpp \
		cli/ArgumentParser.cpp \
		cli/Logger.cpp \
		cli/CanLogger.cpp 

OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

PREFIX ?= $(HOME)/.local/bin

CXX := g++
CXXFLAGS := -Iinclude -DTARGET=\"$(EXE_TARGET)\" -DVERSION=\"$(VERSION)\" -Wall -Wextra -Wpedantic -MMD -MP -O2 -std=c++17
LDFLAGS :=
AR := ar
ARFLAGS := rcs

CLANG_FORMAT ?= clang-format
FORMAT_FILES := $(shell find . -name "*.cpp" -o -name "*.hpp")

all: $(EXE_TARGET) 

$(EXE_TARGET): $(LIB_TARGET)
	$(CXX) $(CXXFLAGS) -c $(SRC_DIR)/main.cpp -o $(BUILD_DIR)/main.o 
	$(CXX) $(BUILD_DIR)/main.o $(BUILD_DIR)/$^ $(LDFLAGS) -o $(BUILD_DIR)/$@

$(LIB_TARGET): $(OBJS)
	$(AR) $(ARFLAGS) $(BUILD_DIR)/$@ $^

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

format:
	$(CLANG_FORMAT) -i $(FORMAT_FILES)

release: all
	rm -rf "$(BUILD_DIR)/$(EXE_TARGET)-$(VERSION)"
	mkdir -p "$(BUILD_DIR)/$(EXE_TARGET)-$(VERSION)"/{inc,lib,bin}
	cp -r include/* "$(BUILD_DIR)/$(EXE_TARGET)-$(VERSION)/inc/"
	cp "$(BUILD_DIR)/$(LIB_TARGET)" "$(BUILD_DIR)/$(EXE_TARGET)-$(VERSION)/lib/"
	cp "$(BUILD_DIR)/$(EXE_TARGET)" "$(BUILD_DIR)/$(EXE_TARGET)-$(VERSION)/bin/"
	cp "README.md" "$(BUILD_DIR)/$(EXE_TARGET)-$(VERSION)"
	tar -C "$(BUILD_DIR)" \
	    -czvf "$(BUILD_DIR)/$(EXE_TARGET)-$(VERSION).tar.gz" \
	    "$(EXE_TARGET)-$(VERSION)"


clean:
	rm -rf $(BUILD_DIR)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)
	install -m 755 $(BUILD_DIR)/$(EXE_TARGET) $(DESTDIR)$(PREFIX)/$(EXE_TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/$(EXE_TARGET)

test: $(LIB_TARGET)
	$(MAKE) -C tests run

coverage: CXXFLAGS+=--coverage
coverage: LDFLAGS+=--coverage
coverage: test
	lcov --capture --directory $(BUILD_DIR) --output-file $(BUILD_DIR)/coverage.info --rc lcov_branch_coverage=1 --ignore-errors inconsistent
	lcov --remove $(BUILD_DIR)/coverage.info '*/tests/*' '*/include/*' --output-file $(BUILD_DIR)/coverage.info.cleaned --rc lcov_branch_coverage=1
	genhtml $(BUILD_DIR)/coverage.info.cleaned --output-directory $(BUILD_DIR)/coverage-report --rc lcov_branch_coverage=1
	@echo "Coverage report generated in build/coverage-report/index.html"

-include $(DEPS)

.PHONY: all clean format install uninstall test coverage

