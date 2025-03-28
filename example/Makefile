CC=clang
CFLAGS=-Wall -Wextra -Werror -g -I../src/
LDFLAGS= -L../build/ -ljson

main: main.c ../build/libjson.a
	$(CC) $(CFLAGS) $(LDFLAGS) main.c -o main
