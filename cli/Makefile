BUILD_DIR = $(PROJECT_ROOT)/build
OBJS_DIR = $(BUILD_DIR)/cli
OBJS = $(OBJS_DIR)/main.o 
TARGET = $(BUILD_DIR)/$(PROJECT_NAME)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $^ -o $@

$(OBJS_DIR)/%o: %.cpp
	$(CXX) -c $< -o $@
