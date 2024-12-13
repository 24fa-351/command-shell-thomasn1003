# Makefile for xsh shell program

all: xsh

xsh: commandshell.c
	gcc -Wall -Wextra -std=c99 -O2 -o xsh commandshell.c

clean:
	rm -f xsh