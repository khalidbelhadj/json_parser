CC = clang
CFLAGS = -Wall -Wextra -Werror
FILES = src/main.c src/tokeniser.c src/parser.c src/utils.c

build:
	$(CC) $(CFLAGS) $(FILES) -o bin/main

run: build
	./bin/main

clean:
	rm -rf bin main.dSYM