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
#include <queue>

#include "rpc.h"
#include "binder.h"
#include "communication_functions.h"

#define BACKLOG 100

struct serverInfo {
	int sockfd;
	char* serverIP;
	char* serverPort;
};

struct funcStruct {
	char* funcName;
	int* argTypes;
	int argTypesLen;
};


int main() {

	/* Binder Socket (some code gotten from http://beej.us tutorial) */
	int error;
	int binderSocket;
	struct addrinfo hints;
	struct addrinfo *res, *goodres;

	memset(&hints, 0, sizeof hints); // Confirm hints is empty
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // AI_PASSIVE to allow it work for both ipv4 and ipv6

	/* Get the server address information and pass it to the socket function */
	error = getaddrinfo(NULL, "0", &hints, &res);
	if (error) {
	    fprintf(stderr, "binder: getaddrinfo error: %s\n", gai_strerror(error));
	    exit(1);
	}

	/* Find the valid entries and make the socket */
	binderSocket = -1;
	for (goodres = res; goodres != NULL; goodres =goodres->ai_next) {

		// Create the socket
		binderSocket = socket(goodres->ai_family, goodres->ai_socktype, goodres->ai_protocol);
		if (binderSocket<0) {
			perror("binder: error on creating a binder socket");
			continue;
		}

		// Bind the socket
		if (bind(binderSocket, goodres->ai_addr, goodres->ai_addrlen) == -1) {
			close(binderSocket);
			perror("binder: error on binding the binder");
			continue;
		}
		break;
	}
	if (binderSocket<0) { // If we come out of the loop and still not bound to a socket
		fprintf(stderr, "no valid socket was found\n");
	}
	if (goodres == NULL) {
		fprintf(stderr, "failed to bind\n");
	}
	
	if (DEBUG_PRINT_ENABLED)
		std::cout<< "binder: Listening...." <<std::endl;
	/* Free the res linked list structure. Don't need it anymore */
	freeaddrinfo(res);

	/* Listen for connections */
	if (listen(binderSocket, BACKLOG) == -1) {
		fprintf(stderr, "binder: error while trying to listen\n");
	}

	/* Get the Binder's port number and IP */
	struct sockaddr_in binderAddress;
	socklen_t len = sizeof(binderAddress);
	if (getsockname(binderSocket, (struct sockaddr *)&binderAddress, &len) == -1) {
	    perror("error on getsockname");
	}
	else {
		char getBinderHostName[256];
		int r = gethostname(getBinderHostName, 256);
		if (r==-1) {
			perror("error on gethostname");
		}
		printf("BINDER_ADDRESS %s\n",getBinderHostName);
		printf("BINDER_PORT %d\n", ntohs(binderAddress.sin_port));

	}

	/* Select to switch between servers and clients connections requests and processing */
	fd_set master_fd;
	fd_set read_fds;
	int fdmax;
	int newfd;
	struct sockaddr_storage remoteaddr;
	socklen_t addrlen;
	int ret;

	/* Keep track of the number of bytes received so far */
	int nbytes;

	/* Clear master_fd and read_fds */
	FD_ZERO(&master_fd);
	FD_ZERO(&read_fds);

	/* Add the file descriptor of the socket we are listening on to master_fd */
	FD_SET(binderSocket, &master_fd);

	/* At the beginning, the max file descriptor is serverSocket file descriptor */
	fdmax = binderSocket;

	/* Store the incoming message length and type */
	uint32_t messageLength;
	uint32_t messageType;
	int lastFuncAdded=0;

	/* Dictionary and vector to database */
	std::map<std::string, std::queue<std::string> > binderDatabaseStr2;
	// std::map<int, std::vector<std::string> > binderDatabaseStr;
	//std::map<int, std::vector<std::string>::iterator dataBaseIterator;

	int continueRunning = 1;

	/* Store the servers file descriptors */
	std::vector<int> serverFds;

	/* Dictionary to store int to funcArgTypes mapping */
	// std::map<int, funcStruct> funcToMap;

	/* Dictionary to store the server info */
	// std::map<std::string, serverInfo> serverMap;
	
	// std::map<std::string, std::string> serverMapStr;

	/* Binder runs forever, accepting connections and processing data until a termination message */
	while (continueRunning == 1) {

		// At the start of each iteration copy master_fd into read_fds
		read_fds = master_fd;

		// select() blocks until a new connection request or message on current connection
		ret = select(fdmax+1, &read_fds, NULL, NULL, NULL);
		if (ret ==-1) {
			perror("error on select()'s return");
		}

		// Once select returns, loop through the file descriptor list
		for (int i=0; i<=fdmax; i++) {

			if (FD_ISSET(i, &read_fds)) {
				if (i == binderSocket) { // Received a new connection request
					if (DEBUG_PRINT_ENABLED)
						std::cout<<"Binder Accepted new connection...." <<std::endl;

					addrlen = sizeof remoteaddr;
					newfd = accept(binderSocket, (struct sockaddr *)&remoteaddr, &addrlen);
					if (newfd == -1) {
						perror("error when accepting new connection");
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
				else { // Received data from an existing connection
					if (DEBUG_PRINT_ENABLED)
						std::cout<<"Binder Accepted Data...." <<std::endl;
					// First, get the length of the incoming message
					int retL=receiveInt(i, &messageLength, sizeof(messageLength), 0);
					if (retL==-1) {
						close(i);
						FD_CLR(i, &master_fd);
						break;
					}
					if (DEBUG_PRINT_ENABLED)
						std::cout<<"Message length: " <<messageLength<<std::endl;
					// Allocate the appropriate memory and get the message
					// char *message;
					// message = (char*) malloc (messageLength);

					// Next, get the type of the incoming message
					receiveInt(i, &messageType, sizeof(messageType), 0);
					if (DEBUG_PRINT_ENABLED)
						std::cout<<"Message Type: " <<messageType<<std::endl;

					// TERMINATION
					if (messageType == TERMINATE) {
						if (DEBUG_PRINT_ENABLED)
							std::cout<<"Binder Received termination request...." <<std::endl;

						continueRunning = 0;

						// Inform all the servers
						// for (int i=0; i<=fdmax; i++) { // Send to both server and client. Client will ignore it.
						// 	if (FD_ISSET(i, &read_fds) && i != binderSocket) { // Wrong, only send it to the servers on the dictionary
								
						// 		// Send the length
						// 		sendInt(i, &messageLength, sizeof(messageLength), 0);

						// 		// Send the message (Just the type: TERMINATE)
						// 		sendInt(i, &messageType, sizeof(messageType), 0);

						// 	}
						// }
						for (int fd=0; fd<serverFds.size(); fd++) { // Send to the servers

							// Send the length
							sendInt(serverFds.at(fd), &messageLength, sizeof(messageLength), 0);

							// Send the message (Just the type: TERMINATE)
							sendInt(serverFds.at(fd), &messageType, sizeof(messageType), 0);
						}
						// Exit
						break;

					}

					// REGISTRATION OR LOC_REQUEST
					else {

						if (messageType == REGISTER) {

							// Add this server's file descriptor to the list
							int u;
							for (u=0;u<serverFds.size();u++) {
								if (serverFds.at(u)==i) {
									// Already added
									break;
								}
							}
							if (u==serverFds.size()) {
								// Not added yet
								serverFds.push_back(i);
							}
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Binder Received registration request...." <<std::endl;

							// Set the server ip, port, function name and argTypes array 
							
							// Get the serveridentifier
							// messageLength = SERVERIP;
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							char* newServerHostName=(char*)malloc(messageLength);
							receiveMessage(i, newServerHostName, messageLength, 0);
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Message received: "<<newServerHostName<<std::endl;

							// Get the port
							// messageLength = SERVERPORT;
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							char* newServerPort=(char*)malloc(messageLength);
							receiveMessage(i, newServerPort, messageLength, 0);
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Message received: "<<newServerPort<<std::endl;

							// Get the funcName
							// messageLength = FUNCNAMELENGTH;
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							char *funcName = (char*)malloc(messageLength);
							receiveMessage(i, funcName, messageLength, 0);
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Message received: "<<funcName<<std::endl;

							// Get the argTypes
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							int *argTypes = (int*)malloc(messageLength);
							int sizeOfArgTypesArray = messageLength;
							int lengthOfargTypesArray = sizeOfArgTypesArray / sizeof(int);
							int getLength = recv(i, argTypes, messageLength, 0);
							if (getLength!=messageLength) {
								int justInCase=getLength;
								while (justInCase<=messageLength) {
									getLength = recv(i, argTypes+getLength, messageLength-getLength, 0);
									justInCase+=getLength;
								}
							}

							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Message received: "<<argTypes[0]<<std::endl;

							// Get the potential key
							std::string keyFuncArgTypes = getUniqueFunctionKey(funcName, argTypes);
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Key for new registration: "<<keyFuncArgTypes<<std::endl;

							char *serverIden = (char*)malloc(strlen(newServerHostName)+strlen(newServerPort)+2);
							memset(serverIden,0,strlen(newServerHostName)+strlen(newServerPort)+2);
							memcpy(serverIden, newServerHostName, strlen(newServerHostName));
							char* delimiter=";";
							memcpy(serverIden+strlen(newServerHostName), delimiter, 1); // Divide the server ip and port by ';'
							memcpy(serverIden+strlen(newServerHostName)+1, newServerPort, strlen(newServerPort));
							std::string serverLocValue(serverIden);
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"new server value: "<<serverLocValue<<std::endl;

							int added = -1;

							// Check if this funcArgTypes combo exists yet
							std::map<std::string, std::queue<std::string> >::iterator it;
							it = binderDatabaseStr2.find(keyFuncArgTypes);
							if (it == binderDatabaseStr2.end()) { // It does not exist

								std::queue<std::string> newListVec;
								newListVec.push(serverLocValue);
								binderDatabaseStr2.insert( std::pair<std::string, std::queue<std::string> >(keyFuncArgTypes,newListVec) );

								// Update added
								added = 1;
							}
							else { // It exists

								binderDatabaseStr2[keyFuncArgTypes].push(serverLocValue);
								
								// Update added
								added = 1;
							}

							// Respond to the server
							uint32_t responseLength = 4; // Just 4 bytes
							uint32_t responseType;
							uint32_t responseMessage;

							// Send the message length
							sendInt(i, &responseLength, sizeof(responseLength), 0);

							if (added ==1) {
								// Respond with REGISTER_SUCCESS as type and NEW_REGISTRATION as message
								responseType = REGISTER_SUCCESS;
								responseMessage = NEW_REGISTRATION;

								// Send the message Type
								sendInt(i, &responseType, sizeof(responseType), 0);

								// Send the message
								sendInt(i, &responseMessage, sizeof(responseMessage), 0);
							} 
							else {
								// Respond with REGISTER_FAILURE as type and BINDER_UNABLE_TO_REGISTER as message
								responseType = REGISTER_FAILURE;
								responseMessage = BINDER_UNABLE_TO_REGISTER;

								// Send the message Type
								sendInt(i, &responseType, sizeof(responseType), 0);

								// Send the message
								sendInt(i, &responseMessage, sizeof(responseMessage), 0);
								
							}

							// Free
							free(newServerHostName);
							free(newServerPort);
							free(funcName);
							free(argTypes);
							free(serverIden);

						}
						else if (messageType == LOC_REQUEST) {
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Binder Received location request...." <<std::endl;

							// Get the funcName
							// messageLength = FUNCNAMELENGTH;
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							char *funcName = (char*)malloc(messageLength);
							receiveMessage(i, funcName, messageLength, 0);
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Message received: "<<funcName<<std::endl;

							// Get the argTypes
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							int *argTypes = (int*)malloc(messageLength);
							int sizeOfArgTypesArray = messageLength;
							int lengthOfargTypesArray = sizeOfArgTypesArray / sizeof(int);
							int getLength = recv(i, argTypes, messageLength, 0);
							if (getLength!=messageLength) {
								int justInCase=getLength;
								while (justInCase<=messageLength) {
									getLength = recv(i, argTypes+getLength, messageLength-getLength, 0);
									justInCase+=getLength;
								}
							}

							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Message received: "<<argTypes[0]<<std::endl;

							// Get the potential key
							std::string keyFuncArgTypes = getUniqueFunctionKey(funcName, argTypes);
							if (DEBUG_PRINT_ENABLED)
								std::cout<<"Key for new registration: "<<keyFuncArgTypes<<std::endl;

							uint32_t responseLength;
							uint32_t responseType;

							std::map<std::string, std::queue<std::string> >::iterator it;
							it = binderDatabaseStr2.find(keyFuncArgTypes);
							if (it != binderDatabaseStr2.end()) { // It does exist

								std::string serverIdAndPort = binderDatabaseStr2[keyFuncArgTypes].front();
								binderDatabaseStr2[keyFuncArgTypes].pop();
								binderDatabaseStr2[keyFuncArgTypes].push(serverIdAndPort);

								// Get the server ip and port
								std::string delimiter = ";";
								std::string serverIP = serverIdAndPort.substr(0, serverIdAndPort.find(delimiter));
								std::string serverPort = serverIdAndPort.substr(serverIdAndPort.find(delimiter)+1, serverIdAndPort.length());

								if (DEBUG_PRINT_ENABLED) {
									std::cout<<serverIP<<std::endl;
									std::cout<<serverPort<<std::endl;
								}

								// std::string str = "string";
								// char *cstr = new char[str.length() + 1];
								// strcpy(cstr, str.c_str());
								char *serverIPchar = new char[serverIP.length() + 1];
								strcpy(serverIPchar, serverIP.c_str());
								char *serverPortchar = new char[serverPort.length() + 1];
								strcpy(serverPortchar, serverPort.c_str());

								// Send the message Type
								responseLength = sizeof(responseType);
								responseType = LOC_SUCCESS;
								sendInt(i, &responseLength, sizeof(responseLength), 0);
								sendInt(i, &responseType, responseLength, 0);

								// Send Server Identifier
								responseLength = strlen(serverIPchar)+1;
								sendInt(i, &responseLength, sizeof(responseLength), 0);
								sendMessage(i, serverIPchar, responseLength, 0);

								// Send Sever Port
								responseLength = strlen(serverPortchar)+1;
								sendInt(i, &responseLength, sizeof(responseLength), 0);
								sendMessage(i, serverPortchar, responseLength, 0);

								// Reset all the other queues to ensure the server just sent is also not at the front
								// std::map<std::string, std::queue<std::string> > binderDatabaseStr2;
								std::string frontOfQueue;
								for (std::map<std::string, std::queue<std::string> >::iterator it=binderDatabaseStr2.begin(); it!=binderDatabaseStr2.end(); ++it) {
    								// std::cout << it->first << " => " << it->second << '\n';
    								frontOfQueue = binderDatabaseStr2[it->first].front(); 
    								if ( frontOfQueue.compare(serverIdAndPort) == 0) {
    									binderDatabaseStr2[it->first].pop();
										binderDatabaseStr2[it->first].push(serverIdAndPort);
										// std::cout<<"out"<<std::endl;
    								}
								}

							}
							else { // It does not exist
								// Respond with LOC_FAILURE and reasonCode
								responseLength = 4;
								responseType = LOC_FAILURE;
								uint32_t responseMessage = NO_SERVER_CAN_HANDLE_REQUEST;

								// Send the message length
								sendInt(i, &responseLength, sizeof(responseLength), 0);

								// Send the message Type
								sendInt(i, &responseType, sizeof(responseType), 0);

								// Send the message
								sendInt(i, &responseMessage, sizeof(responseMessage), 0);
							}
							// Free
							free(funcName);
							free(argTypes);
							//free(funcArgTypesToFind);
						}

					}	

					// free(message);
				}
			}
		}

	}

	// Close binder socket
	close(binderSocket);
}
