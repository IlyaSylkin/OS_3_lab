# Makefile (упрощенный)
CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGETS = parent child

all: $(TARGETS)

parent: parent.c process.c
	$(CC) $(CFLAGS) -o parent parent.c process.c -lrt -pthread

child: child.c process.c
	$(CC) $(CFLAGS) -o child child.c process.c -lrt -pthread

clean:
	rm -f $(TARGETS) *.o

.PHONY: all clean