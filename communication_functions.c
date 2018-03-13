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

/* Get the type from the macros */
int getTypeSize(int typeGotten) {

	int typeOfArgs;

	switch(typeGotten) {

		case ARG_CHAR:
			typeOfArgs = sizeof(char);
			break;

		case ARG_SHORT:
			typeOfArgs = sizeof(short);
			break;

		case ARG_INT:
			typeOfArgs = sizeof(int);
			break;

		case ARG_LONG:
			typeOfArgs = sizeof(long);
			break;

		case ARG_DOUBLE:
			typeOfArgs = sizeof(double);
			break;

		case ARG_FLOAT:
			typeOfArgs = sizeof(float);
			break;
	  
		default:
			printf("Invalid Type\n");
	}

	return typeOfArgs;
}

/* This will be used to send the length and message Types */
int sendInt(int socketfd, uint32_t *numToSend, size_t length, int flags) {

	/* First send attempt */
	int sendLength = send(socketfd, numToSend, length, 0);

	/* If full message was not sent in first send attempt */
	if (sendLength!=length) {
		int justInCase=sendLength;
		while (justInCase<=length) {
			sendLength = send(socketfd, numToSend+sendLength, length-sendLength, 0);
			justInCase+=sendLength;
		}
	}

	return 0;
}

/* This will be used to receive the length and message Types */
int receiveInt(int socketfd, uint32_t *numToReceive, size_t length, int flags) {

	/* First receive attempt */
	int nbytes = recv(socketfd, numToReceive, length, 0);

	/* Encountered an error while trying to receive */
	if (nbytes<=0) {
		printf("Error while trying to receive. Closing socket....\n");
		// close(socketfd);
	}
	/* If full message was not received in first receive attempt */
	else if (nbytes<length) {
		int justInCase=nbytes;
		while (justInCase<=length) {
			nbytes = recv(socketfd, numToReceive+nbytes, length-nbytes, 0);
			justInCase+=nbytes;
		}
	}

	return 0;
}

/* This will be used to send the actual message */
int sendMessage(int socketfd, char *messageToSend, size_t length, int flags) {

	/* First send attempt */
	int sendLength = send(socketfd, messageToSend, length, 0);

	/* If full message was not sent in first send attempt */
	if (sendLength!=length) {
		int justInCase=sendLength;
		while (justInCase<=length) {
			sendLength = send(socketfd, messageToSend+sendLength, length-sendLength, 0);
			justInCase+=sendLength;
		}
	}

	return 0;
}

/* This will be used to receive the actual message */
int receiveMessage(int socketfd, char *messageToReceive, size_t length, int flags) {

	/* First receive attempt */
	int nbytes = recv(socketfd, messageToReceive, length, 0);

	/* Encountered an error while trying to receive */
	if (nbytes<=0) {
		printf("Error while trying to receive. Closing socket....\n");
		// close(socketfd);
	}
	/* If full message was not received in first receive attempt */
	else if (nbytes<length) {
		int justInCase=nbytes;
		while (justInCase<=length) {
			nbytes = recv(socketfd, messageToReceive+nbytes, length-nbytes, 0);
			justInCase+=nbytes;
		}
	}
	
	return 0;
}

/* This will be used to create a socket connection */
int connection(const char *destIP, const char *destPort) {

	int socketfd;
	int error2;
	struct addrinfo hints2;
	struct addrinfo *res2, *goodres2;

	memset(&hints2, 0, sizeof hints2);
	hints2.ai_family = AF_UNSPEC;
	hints2.ai_socktype = SOCK_STREAM;
	hints2.ai_flags = AI_PASSIVE;

	/* Get the server address information and pass it to the socket function */
	error2 = getaddrinfo(destIP, destPort, &hints2, &res2);
	if (error2) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error2));
		exit(1);
	}
	/* Find the valid entries and make the socket */
	for (goodres2 = res2; goodres2 != NULL; goodres2 =goodres2->ai_next) {

		// Socket
		socketfd = socket(goodres2->ai_family, goodres2->ai_socktype, goodres2->ai_protocol);
		if (socketfd<0) { // i.e. not -1
			// Not found yet!
			perror("error on creating socket\n");
			continue;
		}

		// Connect
		if (connect(socketfd, goodres2->ai_addr, goodres2->ai_addrlen) == -1) {
			close(socketfd);
			perror("error on creating a new connection\n");
			continue;
		}
		break;
	}
	if (socketfd<0) { // If we come out of the loop and still not found
		fprintf(stderr, "no valid socket was found\n");
		return -1;
	}

	/* Free res */
	freeaddrinfo(res2);

	return socketfd;
}





