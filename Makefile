CC        := clang
CFLAGS    := -Wall -Wextra -std=c23 -Iinclude -g3 -fsanitize=address,undefined -O1 -fno-common -fshort-enums
LDFLAGS   :=
RM        := rm -rf

TARGET    := aleron
BUILD_DIR := build
SRC_DIR   := src

SRCS      := $(wildcard $(SRC_DIR)/*.c)
OBJS      := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

.PHONY: all test run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

test: $(TARGET)
	@LSAN_OPTIONS=suppressions=suppressions.txt ./test.sh

run: $(TARGET)
	@./run.sh $(ARGS)

clean:
	$(RM) $(TARGET) $(BUILD_DIR) *~ tmp*
