# Introduction

The purpose of this project is to implement a crude version of Remote Procedure Call (RPC). Refer to the System manual in this repo for a detailed breakdown of this project and the reasons behind the design decisions.

## Running the application 

To run this application, clone this repo, cd into the directory and follow this procedure in your terminal:

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

## Collaborators

- Joshua Alawode 
- JK Liu

## Reference

Some of the code used in this project was inspired by and taken from [Beej's Guide to Network Programming Using Internet Sockets](http://beej.us).
