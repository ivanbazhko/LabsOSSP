CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-value -std=c11 -pedantic -D_DEFAULT_SOURCE

.PHONY: clean
all: program

program: program.c
	$(CC) $(CFLAGS) program.c -o program

clean:
	rm -rf *.o program