#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rpc.h"
#include "binder.h"

int main() {
	
	// Server Socket (some code gotten from http://beej.us tutorial)
	int error;
	int binderSocket;
	struct addrinfo hints;
	struct addrinfo *res, *goodres;

	memset(&hints, 0, sizeof hints); // Confirm hints is empty
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // To allow it work for both ipv4 and ipv6

	// Get the server address information and pass it to the socket function
	error = getaddrinfo(NULL, "0", &hints, &res);
	if (error) {
//	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
	    exit(1);
	}

	// Find the valid entries and make the socket
	binderSocket = -1;
	for (goodres = res; goodres != NULL; goodres =goodres->ai_next) {

		// Socket
		binderSocket = socket(goodres->ai_family, goodres->ai_socktype, goodres->ai_protocol);
		if (binderSocket<0) { // i.e. not -1
			// Not found yet!
//			perror("server: socket");
			continue;
		}

		// Bind
		if (::bind(serverSocket, goodres->ai_addr, goodres->ai_addrlen) == -1) {
			close(binderSocket);
//			perror("server: bind");
			continue;
		}
		break;
	}
	if (binderSocket<0) { // If we come out of the loop and still not found
//		fprintf(stderr, "no valid socket was found\n");
	}
	if (goodres == NULL) {
//		fprintf(stderr, "failed to bind\n");
	}

	// Free the res linked list structure. Don't need it anymore
	freeaddrinfo(res);

	// Listen
	if (listen(binderSocket, BACKLOG) == -1) {
//		fprintf(stderr, "error while trying to listen\n");
	}

	// Get the Server's port number and IP
	struct sockaddr_in binderAddress;
	socklen_t len = sizeof(binderAddress);
	if (getsockname(binderSocket, (struct sockaddr *)&binderAddress, &len) == -1) {
//	    perror("getsockname");
	}
	else {
		char namGe[1024];
		int r = gethostname(namGe, 1024);
		if (r==-1) {
//			perror("gethostname");
		}
		cout << "BINDER_ADDRESS "<< namGe << endl;
		printf("BINDER_PORT %d\n", ntohs(binderAddress.sin_port));

	}
}
