CC = clang
CFLAGS = -Wall -Wextra -Werror
SRC_DIR=src
BUILD_DIR=build
SRC_FILES=$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES=$(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

build/libjson.a: $(OBJ_FILES)
	ar -rcs $@ $^

fmt:
	clang-format -i */**.c */**.h

clean:
	rm -rf build/*