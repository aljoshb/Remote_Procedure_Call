#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "rpc.h"
#include "binder.h"
#include "communication_functions.h"

/* Get the type from the macros */
uint32_t getTypeSize(uint32_t typeGotten) {

	uint32_t typeOfArgs;

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
		if (DEBUG_PRINT_ENABLED)
			printf("Invalid Type\n");
	}

	return typeOfArgs;
}

/* This will be used to args */
int sendAny(int socketfd, void *args, size_t messageLength, int flags, uint32_t typeArg, uint32_t arrayLen) {

	int sendLength;
	if (arrayLen==0) { // args points to a scalar
		int sendLength;
		
		if(typeArg==ARG_CHAR){
			char sendChar;//= *args;//(char)*args;
			memcpy(&sendChar, args, sizeof(char));
			sendLength = send(socketfd, &sendChar, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, &sendChar+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_SHORT) {
			short sendShort;//= (short)*args;
			memcpy(&sendShort, args, sizeof(short));
			sendLength = send(socketfd, &sendShort, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, &sendShort+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_INT) {
			int sendInteger;//= (int)*args;
			memcpy(&sendInteger, args, sizeof(int));
			sendLength = send(socketfd, &sendInteger, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, &sendInteger+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_LONG) {
			long sendLong;//= (long)*args;
			memcpy(&sendLong, args, sizeof(long));
			sendLength = send(socketfd, &sendLong, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, &sendLong+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_DOUBLE) {
			double sendDouble;//= (double)*args;
			memcpy(&sendDouble, args, sizeof(double));
			sendLength = send(socketfd, &sendDouble, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, &sendDouble+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_FLOAT) {
			float sendFloat;//= (float)*args;
			memcpy(&sendFloat, args, sizeof(float));
			sendLength = send(socketfd, &sendFloat, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, &sendFloat+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		
	}
	else { // args points to an array
		int sendLength;
		if(typeArg==ARG_CHAR){
			// char* sendThis= (char*)malloc(messageLength+1);
			char* sendChar = (char*)args;
			sendLength = send(socketfd, sendChar, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, sendChar+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_SHORT) {
			short* sendShort= (short*)args;
			sendLength = send(socketfd, sendShort, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, sendShort+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_INT) {
			int* sendInteger= (int*)args;
			sendLength = send(socketfd, sendInteger, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, sendInteger+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_LONG) {
			long* sendLong= (long*)args;
			sendLength = send(socketfd, &sendLong, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, sendLong+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
		else if(typeArg==ARG_DOUBLE) {
			double* sendDouble= (double*)args;
			sendLength = send(socketfd, sendDouble, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, sendDouble+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}	
		else if(typeArg==ARG_FLOAT) {
			float* sendFloat= (float*)args;
			sendLength = send(socketfd, sendFloat, messageLength, 0);
			/* If full message was not sent in first send attempt */
			if (sendLength!=messageLength) {
				int justInCase=sendLength;
				while (justInCase<=messageLength) {
					sendLength = send(socketfd, sendFloat+sendLength, messageLength-sendLength, 0); // BAD: void pointer arithmetic
					justInCase+=sendLength;
				}
			}
		}
	
	}

	return 0;
}


/* This will be used to receive args */
int receiveAny(int socketfd, void *args, size_t messageLength, int flags, uint32_t typeArg, uint32_t arrayLen) {

	return 0;
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
		if (DEBUG_PRINT_ENABLED)
			printf("Error while trying to receive. Closing socket....\n");

		close(socketfd);
		return -1;
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
		if (DEBUG_PRINT_ENABLED)
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
			if (ERROR_PRINT_ENABLED)
				perror("error on creating socket\n");
			continue;
		}

		// Connect
		if (connect(socketfd, goodres2->ai_addr, goodres2->ai_addrlen) == -1) {
			close(socketfd);
			if (ERROR_PRINT_ENABLED)
				perror("error on creating a new connection\n");
				
			continue;
		}
		break;
	}
	if (socketfd<0) { // If we come out of the loop and still not found
		if (DEBUG_PRINT_ENABLED)
			fprintf(stderr, "no valid socket was found\n");

		return -1;
	}

	/* Free res */
	freeaddrinfo(res2);

	return socketfd;
}

std::string getUniqueFunctionKey(char* funcName, int* argTypes) {
	std::string cppStr = funcName;

	// loop until we hit the 0 terminator for the argTypes array
	while (*argTypes) {
        std::stringstream ss;
        ss << *argTypes;
        cppStr += ss.str();

		// increment the pointer to the next int
		argTypes++;
    }

	return cppStr;
}
