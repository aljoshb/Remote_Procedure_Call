CC= g++
CFLAGS= -g -Wall

all: binder librpc.a

binder: binder.c
	$(CC) -o $@ $^ communication_functions.c

communication_functions.o: communication_functions.c
	$(CC) -c -o $@ $^

rpc.o: rpc.c
	$(CC) -c -o $@ $^

librpc.a: rpc.o communication_functions.o
	ar rcs $@ $^

toy:
	g++ -o toy.o toy.c
	./toy.o

test: all
	g++ -c -o client.o client.c
	g++ -c -o server_functions.o server_functions.c
	g++ -c -o server_function_skels.o server_function_skels.c
	g++ -c -o server.o server.c
	g++ -L. client.o -lrpc -o client
	g++ -L. server_functions.o server_function_skels.o server.o -lrpc -o server

clean:
	rm -f *.o *.a binder
