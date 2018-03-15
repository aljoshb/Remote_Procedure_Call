# Introduction

This system manual will serve as a guide to this project and give further details on the design choices that the developers made during the course of this project.

## Design Choices

### communication_function.c and communication_function.h

The file communication_function.c (and its header file: communication_function.h) contains some of the key functions that were used during the completion of this assignment. In this file, the sending and receiving functions were implemented. Specifically, two types of sending and receiving functions were implemented, one for sending and receiving integers (such as the length and types of a message: sendInt() and receiveInt()) and the other for sending and receiving the actual messages stored in char arrays. Also, this file contains a function (connection()) which is used to make a socket connection to a known machine address and port.

### Marshalling/Unmarshalling of Data

### Structure Binder Database and Handling of Function Overloading

The binder used a map to store the database. The structure of the map is as follows:

			std::map<std::string, std::queue<std::string> > binderdatabase;

The database stores a mapping of a function/procedure name (as a string) to all the servers that have this function/procedure. The function name serves as the key of the database. Specifically, to handle function overloading, the key is not only just the function name (e.g. "f0"), rather, the key is a concatenation of the function name and the argTypes array converted into a string. That is, the whole argsTypes array (which is an int array) is converted into a string (std::string) which is then combined which the function name to give a unique key. As a result, functions with the same name but with different argument types will be easily differentiated.

As stated, the database maps a function (i.e. key="funcName"+string(argTypes)) to a list of all the servers that have this function. The list of servers is stored in a queue of strings. Each string in the queue represents the location of a server in the form of "serverIp"+";"+"serverPort". That is, the concatenation of the server's location and port. This way, a client request can be easily served by seperating the serverIp and serverPort at the semicolon delimeter.

Below is a sample of what entries will look like in the database:


			key 			|       		Value

			"f034789921567"		{"server1ip;server1port", "server5ip;server5port", "server4ip;server4port"}
			"f1347899215"		{"server1ip;server1port", "server2ip;server2port",  "server3ip;server3port"}

### Registering a Function

### Managing Round-Robin Scheduling

As stated above, the binder database stores a mapping of the functions to the servers that have such a functions (i.e. a mapping of each server that can handle the request). The list of servers is stored as a queue of strings. A queue by design only allows removal of elements from the front and addition of elements to the back. This is inherently round-robin in nature. That is, when a client makes a request for the location of a server that can service particular function, the location of the server at the front of the queue (for that particular function) is sent to the client. Next, this server at the front of the queue is popped and push back into the queue. 

Below is a simulation of how the round-robin approach works:

			1. Client A makes a request for function "f0" with argTypes (converted to a string) of "34789921567"
			2. "f034789921567" has the 3 servers that can service this request as shown in the table above
			3. "sever1ip" and "server1port" is sent to Client A
			4. The front of the queue for function "f034789921567" (i.e. "server1ip;server1port") is popped and pushed to the back of the queue
			5. At the end of the process the database now has the form:

			key 			|       		Value

			"f034789921567"		{"server5ip;server5port", "server4ip;server4port", "server1ip;server1port"}
			"f1347899215"		{"server2ip;server2port", "server1ip;server1port", "server3ip;server3port"} 

That is, server1 has been moved to the back of the list of servers that have function "f034789921567". This way, we guarantee that server 1 will not be called again to service a request for function "f034789921567" until all the other servers ("server5ip;server5port" & "server4ip;server4port") that can service a request for "f034789921567" have also been called.

### Termination procedure




### rpc Library Database and States Stored


### Message Format

Messages are exchanged in a three step format. First the 'length' of the incoming message is sent (in 4 bytes), then the 'type' of the message is sent (in 4 bytes) and finally the actual message is sent (in length bytes). This three step format applies to all message types except the termination message. The termination is sent in a two step format. First the 'length' of the incoming message is sent (in 4 bytes) and then the 'type' of the message is sent (in 4 bytes).

With regards to the termination message, once the binder receives termination message from a client, it sends the message to all the file descriptor sockets it is 'selecting' on. The servers handles this message by verfiying the message was indeed from the binder and after, it terminates.

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


## Error Codes


## Functions Not Implemented

All required functions were implemented.


## Advanced Enhancement

There were no advanced enhancements implemented for this project.
