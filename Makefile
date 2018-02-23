CC= gcc
CFLAGS= -g -Wall

all: binder rpc

binder: binder.c
	$(CC) -o $@ $^

rpc: rpc.c
	$(CC) -o $@ $^

clean:
	rm -f *.o binder rpc
