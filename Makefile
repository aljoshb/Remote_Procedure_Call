CC= gcc
CFLAGS= -g -Wall

all: binder librpc.a

binder: binder.c
	$(CC) -o $@ $^

rpc.o: rpc.c
	$(CC) -c -o $@ $^

librpc.a: rpc.o
	ar rcs $@ $^

clean:
	rm -f *.o *.a binder
