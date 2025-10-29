#
# Makefile for lab 2
#

CFLAGS = -ggdb3 -Wall -pedantic -g -fstack-protector-all -fsanitize=address
shell56: shell.c parser.c
	gcc shell.c parser.c -o shell $(CFLAGS)

clean:
	rm -f *.o shell
