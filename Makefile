CC = gcc
CFLAGS = -Wall -pedantic -g -O0
RM = rm

all: bccsh ep1

bccsh: bccsh.c
	$(CC) $(CFLAGS) bccsh.c -o bccsh -lreadline

ep1: ep1.c
	$(CC) $(CFLAGS) ep1.c -o ep1 -lpthread

clean:
	$(RM) *.o ep1 bccsh
