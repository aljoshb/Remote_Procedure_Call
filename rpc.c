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
#include <vector>
#include <map>
#include <cstring>
#include <string>
#include <sstream>
#include <iostream>

#include "rpc.h"
#include "binder.h"
#include "communication_functions.h"

/* Server socket file descriptors with the client and binder */
int fdServerWithClient = -1;
int fdServerWithBinder = -1;

/* Server location info */
char getServerHostName[SERVERIP];
char* serverPort = (char*)malloc(SERVERPORT);

/* Client socket file descriptors with the binder */
int fdClientWithBinder = -1;

/* Store the registered function and its skeleton */
std::vector<struct funcStructServer> listOfRegisteredFuncArgTypesNew;

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
	std::cout<<"Server Listening...." <<std::endl;
	/* Free res */
	freeaddrinfo(res);

	/* Set the server location info */
	struct sockaddr_in serverAddress;
	socklen_t len = sizeof(serverAddress);
	if (getsockname(fdServerWithClient, (struct sockaddr *)&serverAddress, &len) == -1) {
	    perror("error on getsockname");
	}
	else {
		int r = gethostname(getServerHostName, SERVERIP);
		if (r==-1) {
			perror("error on gethostname");
		}
		//serverPort = ntohs(serverAddress.sin_port);
		memset(serverPort, 0, SERVERPORT);
		sprintf(serverPort, "%d", ntohs(serverAddress.sin_port));
		printf("SERVER_ADDRESS %s\n",getServerHostName);
		printf("SERVER_PORT %s\n", serverPort);
	}

	/* Second Init Step: Server socket connection for communication with binder*/

	/* Get the binder info from env variables */
	const char *binderIP = getenv("BINDER_ADDRESS");
	const char *binderPort = getenv("BINDER_PORT");

	fdServerWithBinder = connection(binderIP, binderPort);
	std::cout<<"Binder file descriptor created: "<<fdServerWithBinder<<std::endl;
	if (fdServerWithBinder<0) { // If we come out of the loop and still not found
		fprintf(stderr, "no valid socket was found\n");
		return -1;
	}

	return 0;
}

int rpcCall(char* name, int* argTypes, void** args) { /*Josh*/

	/* Create a connection to the binder */
	if (fdClientWithBinder == -1) { // A connection hasn't been created yet. Avoid creating multiple connections to the same binder.
		const char *binderIP = getenv("BINDER_ADDRESS");
		const char *binderPort = getenv("BINDER_PORT");
		fdClientWithBinder = connection(binderIP, binderPort);
	}
	if (fdClientWithBinder<0) {
		fprintf(stderr, "unable to connect with the binder\n");
		return -1;
	}

	/* Contact the binder to get the location of a server that has 'name' and argTypes */
	uint32_t messageLenBinder;
	uint32_t messageTypeBinder = LOC_REQUEST;

	// Set the message length
	int i=0;
	int sizeOfargTypesArray;
	int lengthOfargTypesArray;
	while (true) {
		if (*(argTypes+i) == 0) {
			lengthOfargTypesArray = i+1;
			break;
		}
		i++;
	}
	sizeOfargTypesArray = lengthOfargTypesArray * sizeof(int);
	messageLenBinder = FUNCNAMELENGTH + sizeOfargTypesArray;

	// Prepare the message
	char* message = (char*)malloc(messageLenBinder);
	memset(message, 0, FUNCNAMELENGTH); // Pad with zeroes first
	memcpy(message, name, strlen(name));
	memcpy(message+FUNCNAMELENGTH, argTypes, sizeOfargTypesArray);

	// Send the message length
	sendInt(fdClientWithBinder, &messageLenBinder, sizeof(messageLenBinder), 0);
	
	// Send the message type
	sendInt(fdClientWithBinder, &messageTypeBinder, sizeof(messageTypeBinder), 0);
	// Send the actual message
	sendMessage(fdClientWithBinder, message, messageLenBinder, 0);

	// Free message
	free(message);

	/* Receive response back from the binder */
	uint32_t binderResponseLen;
	uint32_t binderResponseType;
	char* binderPositiveResponse = (char*)malloc(SERVERIP+SERVERPORT);
	uint32_t binderNegativeResponse;

	// Get the length
	receiveInt(fdClientWithBinder, &binderResponseLen, sizeof(binderResponseLen), 0);

	// Get the message type
	receiveInt(fdClientWithBinder, &binderResponseType, sizeof(binderResponseType), 0);

	// Get the message
	if (binderResponseType == LOC_SUCCESS) {
		receiveMessage(fdClientWithBinder, binderPositiveResponse, binderResponseLen, 0);

		/* Now contact the server gotten from the binder */
		char* serverIP = (char*)malloc(SERVERIP);
		char* serverPortLoc = (char*)malloc(SERVERPORT);
		memcpy(serverIP, binderPositiveResponse, SERVERIP);
		memcpy(serverPortLoc, binderPositiveResponse+SERVERIP, SERVERPORT);

		int fdClientWithServer = connection(serverIP, serverPortLoc);
		if (fdClientWithServer<0) {
			fprintf(stderr, "unable to connect with the binder\n");
			return -1;
		}
		uint32_t messageLenServer;
		uint32_t messageTypeServer = EXECUTE;
		int lengthOfargArray = lengthOfargTypesArray - 1;
		int totalBytesOfArgs = 0;

		// Set the total bytes of the arguments array
		for (int i=0; i<lengthOfargArray; i++) {
			int lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits
			int typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
			int sizeAtI = getTypeSize(typeAtI);

			if (lenAtI==0) { // This argument is not an array
				totalBytesOfArgs += sizeAtI;
			}
			else {
				totalBytesOfArgs += lenAtI*sizeAtI;
			}
		}

		// Create message to send to the server received from binder. EXECUTE, char* name, int* argTypes, void** args
		char* messageArgsToServer = (char*)malloc(totalBytesOfArgs);
		int lastCopied=0;
		for (int i=0; i<lengthOfargArray; i++) {
			int lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits
			int typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
			int sizeAtI = getTypeSize(typeAtI);

			if (lenAtI==0) { // This argument is not an array
				memcpy(messageArgsToServer+lastCopied, *(args+i), sizeAtI);
				lastCopied = sizeAtI;
			}
			else {
				memcpy(messageArgsToServer+lastCopied, *(args+i), lenAtI*sizeAtI);
				lastCopied = lenAtI*sizeAtI;
			}
		}

		// Copy the whole message
		messageLenServer = FUNCNAMELENGTH + sizeOfargTypesArray + totalBytesOfArgs;
		char* messageToServer = (char*)malloc(messageLenServer);
		memset(message, 0, FUNCNAMELENGTH); // Pad with zeroes first
		memcpy(messageToServer, name, strlen(name));
		memcpy(messageToServer+FUNCNAMELENGTH, argTypes, sizeOfargTypesArray);
		memcpy(messageToServer+FUNCNAMELENGTH+sizeOfargTypesArray, messageArgsToServer, totalBytesOfArgs);

		// Send the length
		sendInt(fdClientWithServer, &messageLenServer, sizeof(messageLenServer), 0);

		// Send the message type
		sendInt(fdClientWithServer, &messageTypeServer, sizeof(messageTypeServer), 0);

		// Send the message
		sendMessage(fdClientWithServer, messageToServer, messageLenServer, 0);

		// Free
		free(messageToServer);
		free(messageArgsToServer);


		/* Receive response back from the Server */
		uint32_t serverResponseLen;
		uint32_t serverResponseType;
		char* serverPositiveResponse = (char*)malloc(messageLenServer);/*Length of char *name+int *argTypes+ void **args */
		uint32_t serverNegativeResponse;

		// Get the length
		receiveInt(fdClientWithServer, &serverResponseLen, sizeof(serverResponseLen), 0);

		// Get the message type
		receiveInt(fdClientWithServer, &serverResponseType, sizeof(serverResponseType), 0);

		// Get the message
		if (serverResponseType == EXECUTE_SUCCESS) {

			receiveMessage(fdClientWithServer, serverPositiveResponse, serverResponseLen, 0);

			// Put it back into **args
			int copiedSoFar=0;
			for (int i=0; i<lengthOfargArray; i++) {
				int lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits
				int typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
				int sizeAtI = getTypeSize(typeAtI);

				if (lenAtI==0) { // This argument is not an array

					memcpy(*(args+i), serverPositiveResponse+FUNCNAMELENGTH+sizeOfargTypesArray+copiedSoFar, sizeAtI);
					copiedSoFar += sizeAtI;
				}
				else {

					memcpy(*(args+i), serverPositiveResponse+FUNCNAMELENGTH+sizeOfargTypesArray+copiedSoFar, lenAtI*sizeAtI);
					copiedSoFar += lenAtI*sizeAtI;
				}
			}

		}
		else if (serverResponseType == EXECUTE_FAILURE) {

			receiveInt(fdClientWithServer, &serverNegativeResponse, serverResponseLen, 0);

			if (serverNegativeResponse == SERVER_CANNOT_HANDLE_REQUEST) {
				printf("server does not have this procedure\n");
			}
			else if (serverNegativeResponse == SERVER_IS_OVERLOADED) {
				printf("server is currently overloaded, try again later...\n");
			}
		}

		free(serverPositiveResponse);

	}
	else if (binderResponseType == LOC_FAILURE) {

		receiveInt(fdClientWithBinder, &binderNegativeResponse, binderResponseLen, 0);

		if (binderNegativeResponse == NO_SERVER_CAN_HANDLE_REQUEST) {
			printf("no server can handle the request\n");
		}

	}

	free(binderPositiveResponse);
	
	return 0;
}

int rpcRegister(char* name, int* argTypes, skeleton f) { /*Josh*/
	
	/* First Register Step: Call the binder and inform it that I have the function procedure called 'name' */
	uint32_t messageLength;
	uint32_t messageType = REGISTER;

	// Set the message length
	int i=0;
	int sizeOfargTypesArray;
	int lengthOfargTypesArray;
	while (true) {
		if (*(argTypes+i) == 0) {
			lengthOfargTypesArray = i+1;
			break;
		}
		i++;
	}

	sizeOfargTypesArray = lengthOfargTypesArray * sizeof(int);

	// Send the type
	messageLength = sizeof(messageType);
	sendInt(fdServerWithBinder, &messageLength, sizeof(messageLength), 0);
	sendInt(fdServerWithBinder, &messageType, sizeof(messageType), 0);

	// Send the serveridentifier
	messageLength = strlen(getServerHostName)+1;
	sendInt(fdServerWithBinder, &messageLength, sizeof(messageLength), 0); // It's own length
	getServerHostName[strlen(getServerHostName)] ='\0';
	sendMessage(fdServerWithBinder, getServerHostName, messageLength, 0);
	std::cout<<"Message sent: "<<getServerHostName<<std::endl;

	// Send the port
	messageLength = strlen(serverPort)+1;
	sendInt(fdServerWithBinder, &messageLength, sizeof(messageLength), 0); // It's own length
	serverPort[strlen(serverPort)] ='\0';
	sendMessage(fdServerWithBinder, serverPort, messageLength, 0);
	std::cout<<"Message sent: "<<serverPort<<std::endl;

	// Send the funcName
	messageLength = strlen(name)+1;
	sendInt(fdServerWithBinder, &messageLength, sizeof(messageLength), 0); // It's own length
	name[strlen(name)] ='\0';
	sendMessage(fdServerWithBinder, name, messageLength, 0);
	std::cout<<"Message sent: "<<name<<std::endl;

	// Send the argTypes
	messageLength = sizeOfargTypesArray;
	sendInt(fdServerWithBinder, &messageLength, sizeof(messageLength), 0);
	int sendLength = send(fdServerWithBinder, argTypes, sizeOfargTypesArray, 0);
	/* If full message was not sent in first send attempt */
	if (sendLength!=messageLength) {
		int justInCase=sendLength;
		while (justInCase<=messageLength) {
			sendLength = send(fdServerWithBinder, argTypes+sendLength, messageLength-sendLength, 0);
			justInCase+=sendLength;
		}
	}
	std::cout<<"Message sent: "<<argTypes[0]<<std::endl;


	std::cout<<"Registration message sent to the binder"<<std::endl;
	/* Get the response back from the binder */ 
	uint32_t receiveLength;
	uint32_t receiveType;
	uint32_t responseMessage;

	// Get the length
	receiveInt(fdServerWithBinder, &receiveLength, sizeof(receiveLength), 0);

	// Get the message type
	receiveInt(fdServerWithBinder, &receiveType, sizeof(receiveType), 0);

	// Get the message
	receiveInt(fdServerWithBinder, &responseMessage, receiveLength, 0);

	/* Second Register Step: Associate the server skeleton with the name and list of args */
	if (receiveType == REGISTER_SUCCESS) {
		printf("HEREEEE\n");
		if (responseMessage == PREVIOUSLY_REGISTERED) {
			printf("PREVIOUSLY_REGISTERED\n");
			printf("binder previously registered this function and argTypes\n");
		}
		else if (responseMessage == NEW_REGISTRATION) {
			printf("NEW_REGISTRATION\n");

			struct funcStructServer newReg;
			newReg.funcName = name;
			newReg.argTypes = argTypes;
			newReg.f = f;

			listOfRegisteredFuncArgTypesNew[listOfRegisteredFuncArgTypesNew.size()];
		}
	}
	else if (receiveType == REGISTER_FAILURE) {
		printf("REGISTER_FAILURE\n");
		if (responseMessage == BINDER_UNABLE_TO_REGISTER) {
			perror("binder unable to register\n");
		}
	}


	return 0;
}

int rpcExecute() { /*Jk*/
	const int BACKLOG = 20;

	// listen on the initialized sockets
	if (listen(fdServerWithClient, BACKLOG) == -1) {
		fprintf(stderr, "error while trying to listen\n");
	}
	
	// if (listen(fdServerWithBinder, BACKLOG) == -1) { //---------Josh
	// 	fprintf(stderr, "error while trying to listen\n");
	// }

	// set up for select
	fd_set master_fd;
	fd_set read_fds;
	int fdmax;
	int newfd;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;
	int ret;

	/* Keep track of the number of bytes received so far */
	int nbytes;

	FD_ZERO(&master_fd);
	FD_ZERO(&read_fds);

	// add sockets to the listened set
	FD_SET(fdServerWithClient, &master_fd);
	FD_SET(fdServerWithBinder, &master_fd);

	// max file descriptor is the largest of the socket fds
	fdmax = fdServerWithClient > fdServerWithBinder ? fdServerWithClient : fdServerWithBinder;

	/* Store the incoming message length and type */
	uint32_t messageLength;
	uint32_t messageType;

	int isRunning = 1;

	// main server loop
	while (isRunning) {
		// At the start of each iteration copy master_fd into read_fds
		read_fds = master_fd;

		// select() blocks until a new connection request or message on current connection
		ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
		if (ret ==-1) {
			perror("error on select()'s return\n");
		}

		// Once select returns, loop through the file descriptor list
		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == fdServerWithClient/* || i == fdServerWithBinder*/) { // Received a new connection request //---------Josh
					addrlen = sizeof remoteaddr;

					if (i == fdServerWithClient) {
						newfd = accept(fdServerWithClient, (struct sockaddr *)&remoteaddr, &addrlen);
					}

					// else if (i == fdServerWithBinder) { //---------Josh
					// 	newfd = accept(fdServerWithBinder, (struct sockaddr *)&remoteaddr, &addrlen);
					// }
					
					if (newfd == -1) {
						perror("error when accepting new connection\n");
					}

					else {
						// Add the new client or server file descriptor to the list
						FD_SET(newfd, &master_fd);
						if (newfd>fdmax) {
							// Then its the new fdmax
							fdmax = newfd;
						}
					}
				}
				else if (i == fdServerWithBinder) { // Must be a termination request//---------Josh
					// handle termination
					if (messageType == TERMINATE) {
						// check if termination from binder
						const char *hostname = getenv("BINDER_ADDRESS");
						const char *portStr = getenv("BINDER_PORT");
						
						isRunning = 0;
						break;
					}
				}

				else { // Received data from an existing connection
					// First, get the length of the incoming message
					receiveInt(i, &messageLength, sizeof(messageLength), 0);

					// Next, get the type of the incoming message
					receiveInt(i, &messageType, sizeof(messageType), 0);

					// // handle termination ---------Josh
					// if (messageType == TERMINATE) {
					// 	// check if termination from binder
					// 	const char *hostname = getenv("BINDER_ADDRESS");
					// 	const char *portStr = getenv("BINDER_PORT");
						
					// 	isRunning = 0;
					// 	break;
					// }

					// forward request to skeleton
					/*else */if (messageType == EXECUTE) { //---------Josh
						// Allocate the appropriate memory and get the message
						char *message;
						message = (char*) malloc(messageLength);

						nbytes = recv(i, message, messageLength, 0);
						if (nbytes>=1 && nbytes<messageLength) { // If full message not received
							int justInCase=nbytes;
							while (justInCase<=messageLength) {
								nbytes = recv(i, message+nbytes, messageLength, 0);
								justInCase+=nbytes;
							}
						}

						// If error or no data, close socket with this client or server
						if (nbytes <= 0) {
							close(i);
							FD_CLR(i, &master_fd);
						}
						else {

						}
						free(message);
					}
				}
			}
		}
	}
		
	return 0;
}

int rpcTerminate() { /*Jk*/

	/* Create a connection to the binder */
	if (fdClientWithBinder == -1) { //---------Josh
		const char *binderIP = getenv("BINDER_ADDRESS");
		const char *binderPort = getenv("BINDER_PORT");
		fdClientWithBinder = connection(binderIP, binderPort);
	}
	if (fdClientWithBinder < 0) {
		fprintf(stderr, "Unable to connect to binder\n");
		return -1;
	}

	uint32_t messageLenBinder;
	uint32_t messageTypeBinder = TERMINATE;

	// Send the message length
	sendInt(fdClientWithBinder, &messageLenBinder, sizeof(messageLenBinder), 0);
	
	// Send the message type
	sendInt(fdClientWithBinder, &messageTypeBinder, sizeof(messageTypeBinder), 0);

	return 0;
}
// length/type/message
