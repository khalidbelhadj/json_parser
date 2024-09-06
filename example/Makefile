CC=clang
CFLAGS=-Wall -Wextra -Werror -g -I../src/
LDFLAGS= -L../build/ -ljson -L../lib/c_utils/build/ -lutils

main: main.c ../build/libjson.a
	$(CC) $(CFLAGS) $(LDFLAGS) main.c -o main
