CC= gcc
CFLAGS= -g -Wall

all: binder rpc.a

binder: binder.c
	$(CC) -o $@ $^

rpc.o: rpc.c
	$(CC) -c -o $@ $^

rpc.a: rpc.o
	ar rcs $@ $^

clean:
	rm -f *.o *.a binder
