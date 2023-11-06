CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -g

all: aesdsocket

aesdsocket: aesdsocket.o
	$(CC) $^ -o $@

aesdsocket.o: aesdsocket.c
	$(CC) -c $^ $(CFLAGS) -o $@ 

clean:
	rm -f *.o aesdsocket
