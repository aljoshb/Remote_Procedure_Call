CC= gcc
CFLAGS= -g -Wall

all: binder librpc.a

binder: binder.c
	$(CC) -o $@ $^

rpc.o: rpc.c
	$(CC) -c -o $@ $^

librpc.a: rpc.o
	ar rcs $@ $^

test:
	g++ -c -o client.o client.c
	g++ -c -o server_functions.o server_functions.c
	g++ -c -o server_function_skels.o server_function_skels.c
	g++ -c -o server.o server.c
	g++ -L. client.o -lrpc -o client
	g++ -L. server_functions.o server_function_skels.o server.o -lrpc -o server

clean:
	rm -f *.o *.a binder
