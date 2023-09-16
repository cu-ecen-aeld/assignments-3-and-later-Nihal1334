
CFLAG = -Wall -g

ifdef CROSS_COMPILE
    	CC=$(CROSS_COMPILE)gcc
else
	CC=gcc
endif

default: all

all: writer

writer: writer.c
	$(CC) $(CFLAG) -o writer writer.c

clean:
	rm -f *.o writer

