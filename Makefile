CC      := clang
CFLAGS  := -Wall -Wextra -std=c23 -Iinclude -g3 -fsanitize=address,undefined -O1 -fno-common -fshort-enums
LDFLAGS :=
RM      := rm -rf

BUILD_DIR      := build
SRC_DIR        := src
TEST_DIR       := tests
TEST_BUILD_DIR := $(BUILD_DIR)/tests

TARGET      := aleron
TEST_TARGET := $(TEST_DIR)/test_runner

# Collect all sources
ALL_SRCS := $(wildcard $(SRC_DIR)/*.c)
TEST_SRCS := $(wildcard $(TEST_DIR)/*.c)

# Object file listings
APP_OBJS  := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(ALL_SRCS))
TEST_OBJS := $(patsubst $(TEST_DIR)/%.c, $(TEST_BUILD_DIR)/%.o, $(TEST_SRCS))

.PHONY: all test run clean

all: $(TARGET) $(TEST_TARGET)

# Main Application Build
$(TARGET): $(APP_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Unit Test Build
test: $(TEST_TARGET)

$(TEST_TARGET): $(TEST_OBJS)
	$(CC) -Isrc $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR)
	$(CC) -Isrc $(CFLAGS) -c $< -o $@

# Directory Creation
$(BUILD_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

clean:
	$(RM) $(TARGET) $(TEST_TARGET) $(BUILD_DIR) *~ tmp*
