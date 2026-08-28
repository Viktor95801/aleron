CC        := clang
CFLAGS    := -Wall -Wextra -std=c23 -Iinclude -g3 -fsanitize=address,undefined -O1 -fno-common -fshort-enums
LDFLAGS   :=
RM        := rm -rf

TARGET      := aleron
TEST_TARGET := test_runner

BUILD_DIR      := build
SRC_DIR        := src
TEST_DIR       := tests
TEST_BUILD_DIR := $(BUILD_DIR)/tests

# Collect all sources under src/
ALL_SRCS       := $(wildcard $(SRC_DIR)/*.c)
# Exclude main.c for unit test inclusion
LIB_SRCS       := $(filter-out $(SRC_DIR)/main.c, $(ALL_SRCS))
TEST_SRCS      := $(wildcard $(TEST_DIR)/*.c)

# Object file listings
APP_OBJS       := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(ALL_SRCS))
LIB_OBJS       := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LIB_SRCS))
TEST_OBJS      := $(patsubst $(TEST_DIR)/%.c, $(TEST_BUILD_DIR)/%.o, $(TEST_SRCS))

.PHONY: all test test_integration run clean

all: $(TARGET)

# Main Application Build
$(TARGET): $(APP_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Unit Test Build
test: $(TEST_TARGET)
	@./test.sh $(ARGS)

$(TEST_TARGET): $(LIB_OBJS) $(TEST_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TEST_BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(TEST_BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Directory Creation
$(BUILD_DIR) $(TEST_BUILD_DIR):
	mkdir -p $@

test_integration: $(TARGET)
	@LSAN_OPTIONS=suppressions=suppressions.txt ./test_integration.sh

run: $(TARGET)
	@./run.sh $(ARGS)

clean:
	$(RM) $(TARGET) $(TEST_TARGET) $(BUILD_DIR) *~ tmp*
