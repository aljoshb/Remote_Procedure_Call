# Introduction

This is system manual will serve as a guide to this project and give further details on the design choices that the developers made during the course of this project.

## Design Choices

### Message Format

Messages are exchanged in a three step format. First the 'length' of the incoming message is sent (in 4 bytes), then the 'type' of the message is sent (in 4 bytes) and finally the actual message is sent (in length bytes). This three step format applies to all message types except the termination message. The termination is sent in a two step format. First the 'length' of the incoming message is sent (in 4 bytes) and then the 'type' of the message is sent (in 4 bytes).

With regards to the termination message, once the binder receives termination message from a client, it sends the message to all the file descriptor sockets it is 'selecting' on. This will include both clients and servers. The servers handles this message by verfiying the message was indeed from the binder and after, it terminates. The clients on the other hand will ignore the message.

These are the message type codes (all represented in integers):

			#define REGISTER          1
			#define REGISTER_SUCCESS  2
			#define REGISTER_FAILURE  3
			#define LOC_REQUEST       4
			#define LOC_SUCCESS       5
			#define LOC_FAILURE       6
			#define EXECUTE  		  7
			#define EXECUTE_SUCCESS   8
			#define EXECUTE_FAILURE   9
			#define TERMINATE		  10


### Specific to librpc.a

### Specific binder.c



## Error Codes


## Functions Not Implemented


## Advanced Enhancement
