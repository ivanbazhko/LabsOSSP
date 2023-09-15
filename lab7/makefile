CC = gcc
CFLAGS = -W -Wall -Wno-unused-parameter -Wno-unused-variable -Wno-unused-value -std=c11 -pedantic -D_DEFAULT_SOURCE

.PHONY: clean
all: program1 program2

program1: program1.c
	$(CC) $(CFLAGS) program1.c -o program1

program2: program2.c
	$(CC) $(CFLAGS) program2.c -o program2

clean:
	rm -rf *.o program1 program2