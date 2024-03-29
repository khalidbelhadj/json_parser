CC = clang
CFLAGS = -Wall -Wextra -Werror

main: main.c ../build/libjson.a
	$(CC) $(CFLAGS) main.c -o main -I../src/ -L../build/ -ljson
