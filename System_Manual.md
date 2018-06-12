# Introduction

This system manual will serve as a guide to this project and give further details on the design choices that the developers made during the course of this project.

## Design Choices

The following subsections will give a detailed explanation of the reasoning behind the design choices that were made during this project.


### communication_function.c and communication_function.h

The file communication_function.c (and its header file: communication_function.h) contains some of the key functions that were used during the completion of this project. In this file, the sending and receiving functions were implemented. Specifically, two types of sending and receiving functions were implemented, one for sending and receiving integers (such as the length and types of a message: sendInt() and receiveInt()) and the other for sending and receiving the actual messages stored in char arrays. Also, this file contains a function (connection()) which is used to make a socket connection to a known machine address and port.


### Marshalling/Unmarshalling of Data

To send the arguments from the client to the server, the areas of memory that contained each argument in the `void** args` array were marshalled into a `char` array representing the binary data of all the aguments concatenated together. This is possible from the `void** args` array containing the starting address of each of its arguments, and we can determine how much memory to copy for each element of the `void** args` array from the metadata contained in `int* argTypes`. 

On the server side, the arguments are contained in a contiguous block of memory. Again, from the  sizes of each argument contained within the `int* argTypes` array, we can reconstruct the pointers of each argument to a copy of `void** args` residing on the server by adding an offset of the size of each of the arguments. 

The values of all arguments in the `void** args` array are copied back into contiguous `char` array in the event that a server function allocated dynamic memory and did not operate directly on the `void** args` array.

Finally, when the client receives the contiguous block of memory back in the form of a `char` array, it places each argument received from the server back into its original memory location. This has the end effect as if the client called the function locally.

### Structure Binder Database and Handling of Function Overloading

The binder used a map to store the database. The structure of the map is as follows:

            std::map<std::string, std::queue<std::string> > binderdatabase;

The database stores a mapping of a function/procedure name (as a string) to all the servers that have this function/procedure. The function name serves as the key of the database. Specifically, to handle function overloading, the key is not only just the function name (e.g. "f0"), rather, the key is a concatenation of the function name and the argTypes array converted into a string. That is, the whole argsTypes array (which is an int array) is converted into a string (std::string) which is then combined which the function name to give a unique key. As a result, functions with the same name but with different argument types will be easily differentiated.

As stated, the database maps a function (i.e. key="funcName"+string(argTypes)) to a list of all the servers that have this function. The list of servers is stored in a queue of strings. Each string in the queue represents the location of a server in the form of "serverIp"+";"+"serverPort". That is, the concatenation of the server's location and port. This way, a client request can be easily served by seperating the serverIp and serverPort at the semicolon delimeter.

Below is a sample of what entries will look like in the database:


                key             |           Value

            "f034789921567"          {"server1ip;server1port", "server5ip;server5port", "server4ip;server4port"}
            "f1347899215"            {"server1ip;server1port", "server2ip;server2port",  "server3ip;server3port"}


### Managing Round-Robin Scheduling

As stated above, the binder database stores a mapping of the functions to the servers that have such a functions (i.e. a mapping of each server that can handle the request). The list of servers is stored as a queue of strings. A queue by design only allows removal of elements from the front and addition of elements to the back. This is inherently round-robin in nature. That is, when a client makes a request for the location of a server that can service particular function, the location of the server at the front of the queue (for that particular function) is sent to the client. Next, this server at the front of the queue is popped and push back into the queue. 

Below is a simulation of how the round-robin approach works:

            1. Client A makes a request for function "f0" with argTypes (converted to a string) of "34789921567"
            2. "f034789921567" has the 3 servers that can service this request as shown in the table above
            3. "sever1ip" and "server1port" is sent to Client A
            4. The front of the queue for function "f034789921567" (i.e. "server1ip;server1port") is popped and pushed to the back of the queue
            5. At the end of the process the database now has the form:

                key           |            Value

            "f034789921567"           {"server5ip;server5port", "server4ip;server4port", "server1ip;server1port"}
            "f1347899215"             {"server2ip;server2port", "server1ip;server1port", "server3ip;server3port"} 

That is, server1 has been moved to the back of the list of servers that have function "f034789921567". This way, we guarantee that server 1 will not be called again to service a request for function "f034789921567" until all the other servers ("server5ip;server5port" & "server4ip;server4port") that can service a request for "f034789921567" have also been called.

Furthermore, to ensure that the server location that was just sent to Client A (server1) does not get called to service another function (funtion "f1347899215") in the next consecutive client request, we iterate through the database to check to make sure server1 is not at the front of all the other functions. If we find that server1 is at the front of all the other functions, we pop is and push it to the back of the queue for such a function.


### Termination procedure

In addition to the database that the binder keeps, the binder also keeps track of the socket file descriptors of all the servers (in a vector):

            std::vector<int> serverFds. 

That is, when a server sends a REGISTER request to the binder, the binder adds the file descritor of this server to the 'serverFds'. Therefore, when a client sends a TERMINATION request to the binder, the binder loops through 'serverFds' and sends a TERMINATION message to each server on the list, after which the binder itself terminates. Each server in turn, verifies that the TERMINATION request was actually received from the binder and then it terminates.


### rpc Library Database and Registering the Same Function Twice

The rpc library (as seen from the server) keeps track all the functions that the server has succesfully registered with the binder in the form of a map of the function name (key="funcName"+string(argTypes)) to the function skeleton:

            std::map<std::string, skeleton> listOfRegisteredFuncArgTypesNew

The rpc library will add a function this database only after it has received a REGISTER_SUCCESS message from the binder. If for some reason a REGISTER_FAILURE message is received, the function is not added to the database. As a result, if the server tries to re-register a function that it has previously registered, the rpc library catches this and returns a positive warning of PREVIOUSLY_REGISTERED back to the server. This takes some load off the binder in the form of re-registering previously registered functions from the same server.


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
            #define EXECUTE           7
            #define EXECUTE_SUCCESS   8
            #define EXECUTE_FAILURE   9
            #define TERMINATE         10


## Error Codes, Warnings and Follow up messages

Below are the error messages and codes that this system follows:

            #define BINDER_NOT_FOUND                -2
            #define BINDER_UNABLE_TO_REGISTER       -3
            #define NO_SERVER_CAN_HANDLE_REQUEST    -4
            #define SERVER_CANNOT_HANDLE_REQUEST    -5
            #define SERVER_IS_OVERLOADED            -6

The rpc library returns BINDER_NOT_FOUND to the server or client in the event that the connection to the binder failed or no binder with such an ip and port could be found. The binder responds to server with BINDER_UNABLE_TO_REGISTER in the event that the binder was unable to register the function that the server requested as a result of binder overload or other factors beyound the binder's control. The binder responds with NO_SERVER_CAN_HANDLE_REQUEST in the event there is no server in its database that can service the function that the client is requesting. The server responds to the client with SERVER_CANNOT_HANDLE_REQUEST in the event that the client contacted it for a function that it actually does not have. The server responds to the client with SERVER_IS_OVERLOADED in the event that the server is overloaded or too busy to service the client's request.

Below are the follow up messages and warning messages and codes that this system follows:


            #define NEW_REGISTRATION             11
            #define PREVIOUSLY_REGISTERED        12

The binder responds to the server with a follow up of NEW_REGISTRATION as part of the REGISTER_SUCCESS message that it sends to the server in the event of a successful registration. The rpc library returns a warning of PREVIOUSLY_REGISTERED to the server in the event that the server tries to re-register a function it has already sucdessfully registered in the past.

