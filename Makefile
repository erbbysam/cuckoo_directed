#http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/
CC=gcc -g
CFLAGS=-L/usr/lib -lssl -lcrypto -I. -Wall -std=c99
DEPS = cuckoo.h
OBJ = cuckoo.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

cuckoo: $(OBJ)
	gcc -g -o $@ $^ $(CFLAGS)