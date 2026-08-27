CC       := clang
CFLAGS   := -Wall -Wextra -std=c23 -Iinclude -g -fsanitize=address,undefined,leak -O1 -fno-common
LDFLAGS  :=
RM       := rm -rf

TARGET   := aleron
BUILD_DIR := build
SRC_DIR  := src

# Automatically find all .c files in src/
SRCS     := $(wildcard $(SRC_DIR)/*.c)

# Map src/foo.c -> build/foo.o
OBJS     := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all test clean

all: $(TARGET)

# Link the executable from object files in build/
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Rule to compile each .c file into build/%.o
# The $(BUILD_DIR) prerequisite creates the directory automatically if it doesn't exist
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Order-only prerequisite to ensure the build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TARGET)
	@./test.sh

clean:
	$(RM) $(TARGET) $(BUILD_DIR) *~ tmp*
