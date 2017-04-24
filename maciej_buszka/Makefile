CC = gcc
CFLAGS = -std=gnu11 -Wall -Wextra

all: router

router: router.o connect.o entry.o table.o

clean:
	rm -rf *.o

distclean: clean
	rm -rf router
