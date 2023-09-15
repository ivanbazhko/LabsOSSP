CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -std=c11 -pedantic

.PHONY: clean
all: program

program: program.c
	$(CC) $(CFLAGS) program.c -o program

clean:
	rm -rf *.o program