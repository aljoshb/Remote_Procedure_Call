#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#include "rpc.h"
#include "binder.h"
#include "communication_functions.h"

/* Server socket file descriptors with client and binder */
int fdServerWithClient = -1;
int fdServerWithBinder = -1;

/* Server location info */
char getServerHostName[1024];
uint32_t serverPort;

int rpcInit() { /*Josh*/
	
	/* First Init Step: Server socket for accepting connections from clients */

	int error;
	struct addrinfo hints;
	struct addrinfo *res, *goodres;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	/* Get the server address information and pass it to the socket function */
	error = getaddrinfo(NULL, "0", &hints, &res);
	if (error) {
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
	    exit(1);
	}

	/* Find the valid entries and make the socket */
	for (goodres = res; goodres != NULL; goodres =goodres->ai_next) {

		// Create the socket
		fdServerWithClient = socket(goodres->ai_family, goodres->ai_socktype, goodres->ai_protocol);
		if (fdServerWithClient<0) {
			perror("error on creating a server socket");
			continue;
		}

		// Bind the socket
		if (bind(fdServerWithClient, goodres->ai_addr, goodres->ai_addrlen) == -1) {
			close(fdServerWithClient);
			perror("error on binding the server");
			continue;
		}
		break;
	}
	if (fdServerWithClient<0) { // If we come out of the loop and still not bound to a socket
		fprintf(stderr, "no valid socket was found\n");
		return -1;
	}
	if (goodres == NULL) {
		fprintf(stderr, "failed to bind\n");
		return -1;
	}

	/* Free res */
	freeaddrinfo(res);

	/* Set the server location info */
	struct sockaddr_in serverAddress;
	socklen_t len = sizeof(serverAddress);
	if (getsockname(fdServerWithClient, (struct sockaddr *)&serverAddress, &len) == -1) {
	    perror("error on getsockname");
	}
	else {
		int r = gethostname(getServerHostName, 1024);
		if (r==-1) {
			perror("error on gethostname");
		}
		serverPort = ntohs(serverAddress.sin_port);

		// printf("SERVER_ADDRESS %s\n",getServerHostName);
		// printf("SERVER_PORT %d\n", serverPort);
	}

	/* Second Init Step: Server socket connection for communication with binder*/

	/* Get the binder info from env variables */
	const char *binderIP = getenv("BINDER_ADDRESS");
	const char *binderPort = getenv("BINDER_PORT");

	fdServerWithBinder = connection(binderIP, binderPort);
	if (fdServerWithBinder<0) { // If we come out of the loop and still not found
		fprintf(stderr, "no valid socket was found\n");
		return -1;
	}

	return 0;
}

int rpcCall(char* name, int* argTypes, void** args) { /*Josh*/

	/* Create a connection to the binder */

	const char *binderIP = getenv("BINDER_ADDRESS");
	const char *binderPort = getenv("BINDER_PORT");

	/* Set up socket and connect to binder */
	int fdClientWithBinder = connection(binderIP, binderPort);
	if (fdClientWithBinder<0) {
		fprintf(stderr, "unable to connect with the binder\n");
		return -1;
	}

	/* Contact the binder to get the location of a server that has 'name' */

	uint32_t messageLenBinder;
	uint32_t messageTypeBinder = LOC_REQUEST;

	// Send the message length
	sendInt(fdClientWithBinder, &messageLenBinder, sizeof(messageLenBinder), 0);
	
	// Send the message type
	sendInt(fdClientWithBinder, &messageTypeBinder, sizeof(messageTypeBinder), 0);

	// Send the actual message

	/* Receive response back from the binder */

	// Get the length

	// Get the message type

	// Set the expected server info

	// Then create a socket connection to the server using the ip and port info gotten from the binder
	int fdClientWithServer; // =connection(serverIP, serverPort);
	if (fdClientWithServer<0) {
		fprintf(stderr, "unable to connect with the binder\n");
		return -1;
	}

	// Then close the socket with the binder
	close(fdClientWithBinder);

	/* Now contact the server gotten from the binder */

	uint32_t messageLenServer;
	uint32_t messageTypeServer = EXECUTE;

	// Send the length
	sendInt(fdClientWithServer, &messageLenServer, sizeof(messageLenServer), 0);

	// Send the message type
	sendInt(fdClientWithServer, &messageTypeServer, sizeof(messageTypeServer), 0);

	// Send the message

	/* Receive response back from the Server */

	// Get the length

	// Get the message type

	// Get the message

	
	return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) { /*Josh*/
	
	/* First Register Step: Call the binder and inform it that I have the function procedure called 'name' */
	
	uint32_t messageLength;
	uint32_t messageType = REGISTER;

	// Set the message length
		// sizeof(array) does not work on arrays passed into a function,
		// so how do I get the length argTypes, then the length of the message?

	// Send the length
	sendInt(fdServerWithBinder, &messageLength, sizeof(messageLength), 0);

	// Send the type
	sendInt(fdServerWithBinder, &messageType, sizeof(messageType), 0);

	// Send the message
		// How to concat the name and argTypes array into one message?


	/* Get the response back from the binder */ 
	uint32_t receiveLength;
	uint32_t receiveType;

	// Get the length
	receiveInt(fdServerWithBinder, &receiveLength, sizeof(receiveLength), 0);

	// Get the message type
	receiveInt(fdServerWithBinder, &receiveType, sizeof(receiveType), 0);

	// Get the message

	/* Second Register Step: Associate the server skeleton with the name and list of args */


	return 0;
}
int rpcExecute() { /*Jk*/
	// main server loop
	
	int messageType = 0;

	// handle termination
	if (messageType == TERMINATE) {
		const char *hostname = getenv("BINDER_ADDRESS");
		const char *portStr = getenv("BINDER_PORT");
		
		// check if termination from binder
		

		// binder requested termination
		return 0;
	}

	// forward request to skeleton
	else if (messageType == EXECUTE) {
	
		// return results to client
	}

	return 0;
}
int rpcTerminate() { /*Jk*/
	const char *hostname = getenv("BINDER_ADDRESS");
	const char *portStr = getenv("BINDER_PORT");

	// client passes request to binder

	return 0;
}
// length/type/message
