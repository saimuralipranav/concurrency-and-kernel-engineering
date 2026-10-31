CC = gcc
CFLAGS = -Wall -Wextra -pthread -O3

all: bellman_ford

bellman_ford: bellman_ford.c
	$(CC) $(CFLAGS) -o bellman_ford bellman_ford.c

clean:
	rm -f bellman_ford

