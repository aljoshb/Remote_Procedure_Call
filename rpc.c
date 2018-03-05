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

/* Server socket file descriptors with client and binder */
int fdServerWithClient = -1;
int fdServerWithBinder = -1;

/* Binder location info */
const char *binderIP;
const char *binderPort;

/* Server location info */
char getServerHostName[1024];
uint32_t serverPort;

int rpcInit() { /*Josh*/
	
	/* First Init: Server socket for accepting connections from clients */

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

		printf("SERVER_ADDRESS %s\n",getServerHostName);
		printf("SERVER_PORT %d\n", serverPort);
	}

	/* Second Init: Server socket connection for communication with binder*/

	/* Get the binder info from env variables */
	binderIP = getenv("BINDER_ADDRESS");
	binderPort = getenv("BINDER_PORT");

	/* Set up socket and connect to binder */
	int error2;
	struct addrinfo hints2;
	struct addrinfo *res2, *goodres2;

	memset(&hints2, 0, sizeof hints2);
	hints2.ai_family = AF_UNSPEC;
	hints2.ai_socktype = SOCK_STREAM;
	hints2.ai_flags = AI_PASSIVE;

	/* Get the server address information and pass it to the socket function */
	error = getaddrinfo(binderIP, binderPort, &hints2, &res2);
	if (error2) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
		exit(1);
	}
	/* Find the valid entries and make the socket */
	for (goodres2 = res2; goodres2 != NULL; goodres2 =goodres2->ai_next) {

		// Socket
		fdServerWithBinder = socket(goodres2->ai_family, goodres2->ai_socktype, goodres2->ai_protocol);
		if (fdServerWithBinder<0) { // i.e. not -1
			// Not found yet!
			perror("error on creating socket to connect to the binder");
			continue;
		}

		// Connect
		if (connect(fdServerWithBinder, goodres2->ai_addr, goodres2->ai_addrlen) == -1) {
			close(fdServerWithBinder);
			perror("error on connecting to the binder");
			continue;
		}
		break;
	}
	if (fdServerWithBinder<0) { // If we come out of the loop and still not found
		fprintf(stderr, "no valid socket was found\n");
		return -1;
	}

	/* Free res2 */
	freeaddrinfo(res2);

	return 0;
}
int rpcCall(char* name, int* argTypes, void** args) { /*Josh*/

	return 0;
}
int rpcRegister(char* name, int* argTypes, skeleton f) { /*Josh*/
	
	/* Call the binder and inform it that I have the function procedure called 'name' */


	/* Associate the server skeleton with the name and list of args */


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
