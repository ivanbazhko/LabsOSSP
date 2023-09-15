CC = gcc
CFLAGS = -W -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable

.PHONY: clean
all: dirwalk

dirwalk: dirwalk.c
	$(CC) $(CFLAGS) dirwalk.c -o dirwalk
	
clean:
	rm -rf *.o dirwalk