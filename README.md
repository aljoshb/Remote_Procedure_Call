# CS 454/654 Assignment 3

## Introduction

This assignment was completed to meet the requirements of CS454/654. The purpose of this assignment is to implement a crude version of Remote Procedure Call (RPC).

## Running the application 

To run this application, follow this procedure in the command line (terminal):

				$ make
				$ g++ -L. client.o -lrpc -o client
				$ g++ -L. server functions.o server function skels.o server.o -lrpc -o server
				$ ./binder

Next, manually set the binder address and binder port using the out gotten as follows:

				$ export BINDER_ADDRESS=<binder port value gotten>
				$ export BINDER_PORT=<binder port value gotten>

Finally, run the client and server files as follows:

				$ ./server
				$ ./client

## Group Members

- `jb2alawo` Joshua Alawode 
- `j327liu` JK Liu

## Reference

Some of the code used in this project was inspired by and taken from [Beej's Guide to Network Programming Using Internet Sockets](http://beej.us).
