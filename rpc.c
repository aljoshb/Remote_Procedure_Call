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
// struct funcStructServer {
// 	char* funcName;
// 	int* argTypes;
// 	skeleton f;
// };

// std::vector<std::string> listOfRegisteredFuncArgTypesNew;
std::map<std::string, skeleton> listOfRegisteredFuncArgTypesNew;

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
	int hasFailed = 0;

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

	// We don't want to care about the specific length of an array argument when request a function
	int* argTypesToSend = (int*)malloc(sizeOfargTypesArray);
	int totalBytesOfArgs = 0;

		for (int i=0; i<lengthOfargTypesArray; i++) {
			int inputOutInfo = *(argTypes+i) >> 24 & 0xff; // The 1st byte from the left
			int typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
			int lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits (3rd and 4th from left)

			if (lenAtI==0) { // This argument is not an array

				// No need to update argTypes at this location, its the same
				*(argTypesToSend+i) = *(argTypes+i);
			}
			else {

				// Update argTypes at this location
				*(argTypesToSend+i) = (inputOutInfo << 24) | (typeAtI << 16) | ITS_AN_ARRAY;
			}
	}

	uint32_t messageLength;
	uint32_t messageType = LOC_REQUEST;

	// Send the type
	messageLength = sizeof(messageType);
	sendInt(fdClientWithBinder, &messageLength, sizeof(messageLength), 0);
	sendInt(fdClientWithBinder, &messageType, messageLength, 0);
	std::cout<<"Message sent: "<<messageType<<std::endl;

	// Send the funcName
	messageLength = strlen(name)+1;
	sendInt(fdClientWithBinder, &messageLength, sizeof(messageLength), 0); // It's own length
	sendMessage(fdClientWithBinder, name, messageLength, 0);
	std::cout<<"Message sent: "<<name<<std::endl;

	// Send the argTypes
	messageLength = sizeOfargTypesArray;
	sendInt(fdClientWithBinder, &messageLength, sizeof(messageLength), 0);
	// int sendLength = send(fdClientWithBinder, argTypes, sizeOfargTypesArray, 0);
	int sendLength = send(fdClientWithBinder, argTypesToSend, sizeOfargTypesArray, 0);
	/* If full message was not sent in first send attempt */
	if (sendLength!=messageLength) {
		int justInCase=sendLength;
		while (justInCase<=messageLength) {
			sendLength = send(fdClientWithBinder, argTypes+sendLength, messageLength-sendLength, 0);
			justInCase+=sendLength;
		}
	}
	std::cout<<"Message sent: "<<argTypesToSend[0]<<std::endl;

	/* Receive response back from the binder */
	uint32_t binderResponseLen;
	uint32_t binderResponseType;
	uint32_t binderNegativeResponse;

	// Get the type
	receiveInt(fdClientWithBinder, &binderResponseLen, sizeof(binderResponseLen), 0);
	receiveInt(fdClientWithBinder, &binderResponseType, binderResponseLen, 0);
	
	
	if (binderResponseType == LOC_SUCCESS) {

		std::cout<<"LOC_SUCCESS!"<<std::endl;

		// Get the server identifier
		receiveInt(fdClientWithBinder, &binderResponseLen, sizeof(binderResponseLen), 0);
		char* serverIdentifier = (char*)malloc(binderResponseLen);
		receiveMessage(fdClientWithBinder, serverIdentifier, binderResponseLen, 0);
		std::cout<<"Server IP Received: "<<serverIdentifier<<std::endl;

		// Get the server port
		receiveInt(fdClientWithBinder, &binderResponseLen, sizeof(binderResponseLen), 0);
		char* serverPortNum = (char*)malloc(binderResponseLen);
		receiveMessage(fdClientWithBinder, serverPortNum, binderResponseLen, 0);
		std::cout<<"Server Port Received: "<<serverPortNum<<std::endl;


		/* Now contact the server gotten from the binder */
		int fdClientWithServer = connection(serverIdentifier, serverPortNum);
		if (fdClientWithServer<0) {
			fprintf(stderr, "unable to connect with the binder\n");
			return -1;
		}

		// Prepare **args
		uint32_t messageTypeServer = EXECUTE;

		int lengthOfargArray = lengthOfargTypesArray - 1;
		int totalBytesOfArgs = 0;

		// Set the total bytes of the arguments array
		for (int i=0; i<lengthOfargArray; i++) {
			uint32_t lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits
			uint32_t typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
			uint32_t sizeAtI = getTypeSize(typeAtI);
			std::cout<<"Length of args at i: "<<lenAtI<<std::endl;
			std::cout<<"Size of args at i: "<<sizeAtI<<std::endl;
			std::cout<<"Type of args at i: "<<typeAtI<<std::endl;
			if (lenAtI==0) { // This argument is not an array
				totalBytesOfArgs += sizeAtI;
			}
			else {
				totalBytesOfArgs += lenAtI*sizeAtI;
			}
		}

		// Send the type
		messageLength = sizeof(messageTypeServer);
		sendInt(fdClientWithServer, &messageLength, sizeof(messageLength), 0);
		sendInt(fdClientWithServer, &messageTypeServer, messageLength, 0);
		std::cout<<"Message sent to server: "<<messageTypeServer<<std::endl;

		// Send the funcName
		messageLength = strlen(name)+1;
		sendInt(fdClientWithServer, &messageLength, sizeof(messageLength), 0); // It's own length
		sendMessage(fdClientWithServer, name, messageLength, 0);
		std::cout<<"Message sent to server: "<<name<<std::endl;

		// Send the argTypes
		messageLength = sizeOfargTypesArray;
		sendInt(fdClientWithServer, &messageLength, sizeof(messageLength), 0);
		int sendLength = send(fdClientWithServer, argTypes, sizeOfargTypesArray, 0);
		/* If full message was not sent in first send attempt */
		if (sendLength!=messageLength) {
			int justInCase=sendLength;
			while (justInCase<=messageLength) {
				sendLength = send(fdClientWithServer, argTypes+sendLength, messageLength-sendLength, 0);
				justInCase+=sendLength;
			}
		}
		std::cout<<"Message sent to server: "<<argTypes[0]<<std::endl;

		// Send the args. Each element in args one at a time

		// First send the total expected length of the args array to the server
		// uint32_t lenArgs = (uint32_t)lengthOfargArray;
		// sendInt(fdClientWithServer, &lenArgs, sizeof(lenArgs), 0);

		// // Then send each element of **args individually
		// for (int i=0; i<lengthOfargArray; i++) {
		// 	uint32_t lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits
		// 	uint32_t typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
		// 	uint32_t sizeAtI = getTypeSize(typeAtI);

		// 	if (lenAtI==0) { // This argument is not an array

		// 		// Send its length
		// 		messageLength = sizeAtI;
		// 		sendInt(fdClientWithServer, &lenAtI, sizeof(messageLength), 0);

		// 		// Send its Type
		// 		sendInt(fdClientWithServer, &typeAtI, sizeof(typeAtI), 0);

		// 		// Send the actual scalar
		// 		sendAny(fdClientWithServer, *(args+i), messageLength, 0, typeAtI, lenAtI);
				
		// 	}
		// 	else {

		// 		// Send its length
		// 		messageLength = lenAtI*sizeAtI;
		// 		sendInt(fdClientWithServer, &lenAtI, sizeof(messageLength), 0);

		// 		// Send its Type
		// 		sendInt(fdClientWithServer, &typeAtI, sizeof(typeAtI), 0);

		// 		// Send the actual scalar
		// 		sendAny(fdClientWithServer, *(args+i), messageLength, 0, typeAtI, lenAtI);
				
		// 	}
		// }
		//Create message to send to the server received from binder
		unsigned char* messageArgsToServer = (unsigned char*)malloc(totalBytesOfArgs);
		int lastCopied=0;
		for (int i=0; i<lengthOfargArray; i++) {
			uint32_t lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits
			uint32_t typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
			uint32_t sizeAtI = getTypeSize(typeAtI);

			if (lenAtI==0) { // This argument is not an array
				memcpy(messageArgsToServer+lastCopied, *(args+i), sizeAtI);
				lastCopied += sizeAtI;
			}
			else {
				memcpy(messageArgsToServer+lastCopied, *(args+i), lenAtI*sizeAtI);
				lastCopied += lenAtI*sizeAtI;
			}
		}

		messageLength = totalBytesOfArgs;

		// Send args
		sendInt(fdClientWithServer, &messageLength, sizeof(messageLength), 0); // Send length
		int ssr= send(fdClientWithServer, messageArgsToServer, messageLength, 0);
		if (ssr!=messageLength) {
			int justInCase=ssr;
			while (justInCase<=messageLength) {
				ssr= send(fdClientWithServer, messageArgsToServer+ssr, messageLength-ssr, 0);
				justInCase+=ssr;
			}
		}
		std::cout<<"Message size sent: "<<ssr<<std::endl;
		std::cout<<"Actual message size: "<<totalBytesOfArgs<<std::endl;
		std::cout<<"Message sent to server: ";
		for (int v=0;v<totalBytesOfArgs;v++) {
			std::cout<<(int)*(messageArgsToServer+v)<<" ";
		}
		std::cout<<std::endl;

		/* Receive response back from the Server */
		uint32_t serverResponseLen;
		uint32_t serverResponseType;
		// char* serverPositiveResponse = (char*)malloc(messageLenServer);/*Length of char *name+int *argTypes+ void **args */
		uint32_t serverNegativeResponse;

		// Get the type
		receiveInt(fdClientWithServer, &serverResponseLen, sizeof(serverResponseLen), 0);
		receiveInt(fdClientWithServer, &serverResponseType, serverResponseLen, 0);

		// Get the message
		if (serverResponseType == EXECUTE_SUCCESS) {
			// Get the size of args back. Ideally should be same as totalBytesOfArgs
			receiveInt(fdClientWithServer, &serverResponseLen, sizeof(serverResponseLen), 0); // Get length

			// Put it back into **args
			unsigned char* messageArgsFromServer = (unsigned char*)malloc(totalBytesOfArgs);//malloc(totalBytesOfArgs);
			int ss= recv(fdClientWithServer, messageArgsFromServer, serverResponseLen, 0);
			if (ss!=serverResponseLen) {
				int justInCase=ss;
				while (justInCase<=serverResponseLen) {
					ss= recv(fdClientWithServer, messageArgsFromServer+ss, serverResponseLen-ss, 0);
					justInCase+=ss;
				}
			}
			int offset = 0;
			for (int i=0; i<lengthOfargArray; i++) {
				uint32_t lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits
				uint32_t typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
				uint32_t sizeAtI = getTypeSize(typeAtI);

				if (lenAtI==0) { // This argument is not an array
					memcpy(*(args+i), messageArgsFromServer + offset, sizeAtI);
					offset += sizeAtI;
				}
				else {
					memcpy(*(args+i), messageArgsFromServer + offset, lenAtI*sizeAtI);
					offset += lenAtI*sizeAtI;
				}
			}

			free(messageArgsFromServer);
		}

		else if (serverResponseType == EXECUTE_FAILURE) {
			receiveInt(fdClientWithServer, &serverResponseLen, sizeof(serverResponseLen), 0);
			receiveInt(fdClientWithServer, &serverNegativeResponse, sizeof(serverNegativeResponse), 0);
			if (serverNegativeResponse == SERVER_CANNOT_HANDLE_REQUEST) {
				printf("Error during server function execution.\n");
			}
			else if (serverNegativeResponse == SERVER_DOES_NOT_HAVE_RPC) {
				printf("Server does not have remote procedure.\n");
			}
			else if (serverNegativeResponse == SERVER_IS_OVERLOADED) {
				printf("server is currently overloaded, try again later...\n");
			}

			hasFailed = 1;
		}

		free(messageArgsToServer);
		free(serverIdentifier);
		free(serverPortNum);
	}
	else if (binderResponseType == LOC_FAILURE) {

		receiveInt(fdClientWithBinder, &binderNegativeResponse, binderResponseLen, 0);

		if (binderNegativeResponse == NO_SERVER_CAN_HANDLE_REQUEST) {
			printf("no server can handle the request\n");
		}

	}

	else {
		printf("Received nothing!\n");
	}
	
	return hasFailed;
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

	// We don't want to care about the specific length of an array argument when registering a function
	int* argTypesToSend = (int*)malloc(sizeOfargTypesArray);
	int totalBytesOfArgs = 0;

	for (int i=0; i<lengthOfargTypesArray; i++) {
		int inputOutInfo = *(argTypes+i) >> 24 & 0xff; // The 1st byte from the left
		int typeAtI = *(argTypes+i) >> 16 & 0xff; // To get the 2nd byte from the left
		int lenAtI = *(argTypes+i) & 0xffff; // Get only the rightmost 16 bits (3rd and 4th from left)

		if (lenAtI==0) { // This argument is not an array

			// No need to update argTypes at this location, its the same
			*(argTypesToSend+i) = *(argTypes+i);
		}
		else {

			// Update argTypes at this location
			*(argTypesToSend+i) = (inputOutInfo << 24) | (typeAtI << 16) | ITS_AN_ARRAY;
		}
	}

	/* Check if this has been previously registered */
	std::string nameArgTypesCombo = getUniqueFunctionKey(name, argTypesToSend);//argTypes);
	std::map<std::string, skeleton>::iterator it;
	it = listOfRegisteredFuncArgTypesNew.find(nameArgTypesCombo);
	if (it != listOfRegisteredFuncArgTypesNew.end()) { // It is has been registered
		printf("binder previously registered this function and argTypes\n");
		return PREVIOUSLY_REGISTERED;
	}
	// for (int i=0;i<listOfRegisteredFuncArgTypesNew.size();i++) {
	// 	if ((listOfRegisteredFuncArgTypesNew[i]).compare(nameArgTypesCombo) == 0) {
	// 		printf("binder previously registered this function and argTypes\n");
	// 		return PREVIOUSLY_REGISTERED;
	// 	}
	// }

	// Send the type
	messageLength = sizeof(messageType);
	sendInt(fdServerWithBinder, &messageLength, sizeof(messageLength), 0);
	sendInt(fdServerWithBinder, &messageType, messageLength, 0);

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
	// name[strlen(name)] ='\0';
	sendMessage(fdServerWithBinder, name, messageLength, 0);
	std::cout<<"Message sent: "<<name<<std::endl;

	// Send the argTypes
	messageLength = sizeOfargTypesArray;
	sendInt(fdServerWithBinder, &messageLength, sizeof(messageLength), 0);
	// int sendLength = send(fdServerWithBinder, argTypes, sizeOfargTypesArray, 0);
	int sendLength = send(fdServerWithBinder, argTypesToSend, sizeOfargTypesArray, 0);
	/* If full message was not sent in first send attempt */
	if (sendLength!=messageLength) {
		int justInCase=sendLength;
		while (justInCase<=messageLength) {
			// sendLength = send(fdServerWithBinder, argTypes+sendLength, messageLength-sendLength, 0);
			sendLength = send(fdServerWithBinder, argTypesToSend+sendLength, messageLength-sendLength, 0);
			justInCase+=sendLength;
		}
	}
	std::cout<<"Message sent: "<<argTypesToSend[0]<<std::endl;

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
	receiveInt(fdServerWithBinder, &responseMessage, sizeof(responseMessage), 0);

	/* Second Register Step: Associate the server skeleton with the name and list of args */
	if (receiveType == REGISTER_SUCCESS) {
		if (responseMessage == NEW_REGISTRATION) {
			printf("NEW_REGISTRATION\n");

			// Update list
			listOfRegisteredFuncArgTypesNew.insert(std::pair<std::string, skeleton> (nameArgTypesCombo, f));
			std::cout << "Added " << name << ": " << (void *) f << std::endl;
			// listOfRegisteredFuncArgTypesNew.push_back(nameArgTypesCombo);
		}
	}
	else if (receiveType == REGISTER_FAILURE) {
		printf("REGISTER_FAILURE\n");
		if (responseMessage == BINDER_UNABLE_TO_REGISTER) {
			perror("binder unable to register\n");
		}
		return BINDER_UNABLE_TO_REGISTER;
	}


	return 0;
}

int rpcExecute() { /*Jk*/
	const int BACKLOG = 20;

	// listen on the initialized sockets
	if (listen(fdServerWithClient, BACKLOG) == -1) {
		fprintf(stderr, "error while trying to listen\n");
	}
	
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
		printf("New Connection received\n");
		// Once select returns, loop through the file descriptor list
		for (int i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == fdServerWithClient) { // Received a new connection request
					addrlen = sizeof remoteaddr;

					if (i == fdServerWithClient) {
						newfd = accept(fdServerWithClient, (struct sockaddr *)&remoteaddr, &addrlen);
					}
					
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
				else if (i == fdServerWithBinder) { // binder only sends termination messages to server
					// First, get the length of the incoming message
					receiveInt(i, &messageLength, sizeof(messageLength), 0);

					// Next, get the type of the incoming message
					receiveInt(i, &messageType, sizeof(messageType), 0);
					
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
					int result = receiveInt(i, &messageLength, sizeof(messageLength), 0);

					if (result < 0) {
						printf("TESTTTT\n");
						close(i);
						FD_CLR(i, &master_fd);
						break;
					}

					else {
						// Next, get the type of the incoming message
						messageType = 0;
						receiveInt(i, &messageType, sizeof(messageType), 0);
						std::cout << "Received type: " << messageType << std::endl;
						std::cout << "Received length: " << messageLength << std::endl;
					}
					printf("I SHOULD NOT SEE THIS\n");
					// forward request to skeleton
					if (messageType == EXECUTE) {
						// message format from rpcCall
						// length, char* funcName
						receiveInt(i, &messageLength, sizeof(messageLength), 0);
						char funcName[messageLength];
						receiveMessage(i, funcName, messageLength, 0);
						std::cout << "Received: " << funcName << std::endl;
						
						// length, int* argTypes
						receiveInt(i, &messageLength, sizeof(messageLength), 0);
						char argTypesChar[messageLength];
						receiveMessage(i, argTypesChar, messageLength, 0);
						int* argTypes = (int*) argTypesChar;
						std::cout << "Received: " << argTypes[0] << std::endl;
						
						// convert to intermediate form to handle arbitrary length arrays
						int argTypesToSend[messageLength/sizeof(int)];

						int lengthOfargTypesArray = 0;
						int k = 0;

						while (true) {
							std::cout << *(argTypes+k) << " ";
							if (*(argTypes+k) == 0) {
								lengthOfargTypesArray = k+1;
								break;
							}
							k++;
						}
						std::cout << std::endl;

						for (int j = 0; j < lengthOfargTypesArray; j++) {
							int inputOutInfo = *(argTypes+j) >> 24 & 0xff; // The 1st byte from the left
							int typeAtI = *(argTypes+j) >> 16 & 0xff; // To get the 2nd byte from the left
							int lenAtI = *(argTypes+j) & 0xffff; // Get only the rightmost 16 bits (3rd and 4th from left)

							if (lenAtI==0) { // This argument is not an array

								// No need to update argTypes at this location, its the same
								*(argTypesToSend+j) = *(argTypes+j);
							}
							else {
								// Update argTypes at this location
								*(argTypesToSend+j) = (inputOutInfo << 24) | (typeAtI << 16) | ITS_AN_ARRAY;
							}
						}

						// length, void** args
						receiveInt(i, &messageLength, sizeof(messageLength), 0);
						uint32_t argsLength = messageLength;
						char argsChar[argsLength];

						receiveMessage(i, argsChar, argsLength, 0);

						// print out args before function call
						std::cout << funcName << " args before function call: " << std::endl;
						for (int j = 0; j < argsLength; j++) {
							int curr = (unsigned char) argsChar[j];

							// print newline every 8 bytes
							if (j % 8 == 0) std::cout << std::endl;

							// pad 0 if only 1 hex character
							if (curr < 0x10) std::cout << "0";
							std::cout << std::hex << curr;
							std::cout << " ";
						}
						std::cout << std::endl;

						// make void* array
						// each pointer needs to point at the start of memory of each argument
						void* args[lengthOfargTypesArray];
						int offset = 0;
						
						// the first element shares the same address as the beginning of the char array
						args[0] = &argsChar;
						
						// lengthOfargTypesArray - 1 since the last element is 0
						for (int j = 0; j < lengthOfargTypesArray - 1; j++) {
							args[j] = ((char *) &argsChar) + offset;

							std::cout << "void* args[" << j << "]: " << args[j] << std::endl;

							uint32_t lenAtJ = *(argTypes + j) & 0xffff; // Get only the rightmost 16 bits
							uint32_t typeAtJ = *(argTypes + j) >> 16 & 0xff; // To get the 2nd byte from the left
							uint32_t sizeAtJ = getTypeSize(typeAtJ);

							if (lenAtJ == 0) { // This argument is not an array
								offset += sizeAtJ;
							}
							else {
								offset += lenAtJ * sizeAtJ;
							}
						}
						
						std::string key = getUniqueFunctionKey(funcName, argTypesToSend);
						std::cout << "Received request for: " << key << std::endl;
						
						// check that function exists on the server
						uint32_t result = EXECUTE_FAILURE;

						std::map<std::string, skeleton>::iterator it;
						it = listOfRegisteredFuncArgTypesNew.find(key);
						if (it != listOfRegisteredFuncArgTypesNew.end()) {
							skeleton skel = listOfRegisteredFuncArgTypesNew.at(key);
							std::cout << "Found function pointer for: " << key << " : " << (void *) skel << std::endl;
							
							// skeleton returns 0, success
							// skeleton returns non-zero, failure
							result = (*skel) (argTypes, args) == 0 ? EXECUTE_SUCCESS : EXECUTE_FAILURE;
							
							// _______ ____    _____   ____  
							//|__   __/ __ \  |  __ \ / __ \ 
							//   | | | |  | | | |  | | |  | |
							// 	 | | | |  | | | |  | | |  | |
							// 	 | | | |__| | | |__| | |__| |
							// 	 |_|  \____/  |_____/ \____/ 
							// 
							// f2 doesn't work properly
							//								

							// copy the server's local memory to be sent to the client
							offset = 0;
							// lengthOfargTypesArray - 1 since the last element is 0
							for (int j = 0; j < lengthOfargTypesArray - 1; j++) {
								uint32_t lenAtJ = *(argTypes + j) & 0xffff; // Get only the rightmost 16 bits
								uint32_t typeAtJ = *(argTypes + j) >> 16 & 0xff; // To get the 2nd byte from the left
								uint32_t sizeAtJ = getTypeSize(typeAtJ);

								if (lenAtJ == 0) { // This argument is not an array
									memcpy(argsChar + offset, *(args + j), sizeAtJ);

									offset += sizeAtJ;
								}
								else {
									memcpy(argsChar + offset, *(args + j), lenAtJ*sizeAtJ);
									offset += lenAtJ * sizeAtJ;
								}
							}

							// print out args after function call
							std::cout << funcName << " after function call: " << std::endl;
							for (int j = 0; j < argsLength; j++) {
								int curr = (unsigned char) argsChar[j];

								// print newline every 8 bytes
								if (j % 8 == 0) std::cout << std::endl;

								// pad 0 if only 1 hex character
								if (curr < 0x10) std::cout << "0";
								std::cout << std::hex << curr;
								std::cout << " ";
								
							}
							std::cout << std::endl;

							// reply EXECUTE_SUCCESS or EXECUTE_FAILURE depending on result
							messageLength = sizeof(result);
							sendInt(i, &messageLength, sizeof(messageLength), 0);
							sendInt(i, &result, sizeof(result), 0);

							if (result == EXECUTE_SUCCESS) {
								// send args
								sendInt(i, &argsLength, sizeof(argsLength), 0);
								sendMessage(i, argsChar, argsLength, 0);
							}

							else {
								// send reasonCode
								uint32_t reasonCode = SERVER_CANNOT_HANDLE_REQUEST;
								messageLength = sizeof(reasonCode);

								sendInt(i, &messageLength, sizeof(messageLength), 0);
								sendInt(i, &reasonCode, sizeof(reasonCode), 0);
							}
						}

						else {
							messageLength = sizeof(result);
							sendInt(i, &messageLength, sizeof(messageLength), 0);
							sendInt(i, &result, sizeof(result), 0);

							// send reasonCode
							uint32_t reasonCode = SERVER_DOES_NOT_HAVE_RPC;
							messageLength = sizeof(reasonCode);

							sendInt(i, &messageLength, sizeof(messageLength), 0);
							sendInt(i, &reasonCode, sizeof(reasonCode), 0);
						}
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
