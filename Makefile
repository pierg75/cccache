CC = gcc 
CFLAGS = -Wall 

cccache: cccache.c data.o
	$(CC) $(CFLAGS) -o cccache cccache.c data.o

data.o: data.c data.h
	$(CC) $(CFLAGS) -c data.c

clean:
	rm -rf cccache data.o
