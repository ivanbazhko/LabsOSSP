CC = gcc
CFLAGS = -W -Wall -std=c11 -pedantic -pthread -Wno-unused-parameter -Wno-unused-variable
.PHONY : clean

all : program generate view

program: program.c makefile
	$(CC) $(CFLAGS) program.c -o program

generate: generate.c makefile
	$(CC) $(CFLAGS) generate.c -o generate

view: view.c makefile
	$(CC) $(CFLAGS) view.c -o view

clean:
	rm -rf *.o program generate view file
