all: phew.c
	gcc -ansi -pedantic-errors -D_XOPEN_SOURCE -Wall -Werror phew.c -o phew
	strip phew
ex: all
	phew
