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
	    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(error));
	    exit(1);
	}

	/* Find the valid entries and make the socket */
	binderSocket = -1;
	for (goodres = res; goodres != NULL; goodres =goodres->ai_next) {

		// Create the socket
		binderSocket = socket(goodres->ai_family, goodres->ai_socktype, goodres->ai_protocol);
		if (binderSocket<0) {
			perror("error on creating a binder socket");
			continue;
		}

		// Bind the socket
		if (bind(binderSocket, goodres->ai_addr, goodres->ai_addrlen) == -1) {
			close(binderSocket);
			perror("error on binding the binder");
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
	std::cout<<"Binder Listening...." <<std::endl;
	/* Free the res linked list structure. Don't need it anymore */
	freeaddrinfo(res);

	/* Listen for connections */
	if (listen(binderSocket, BACKLOG) == -1) {
		fprintf(stderr, "error while trying to listen\n");
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
	std::map<std::string, std::vector<std::string> > binderDatabaseStr2;
	std::map<int, std::vector<std::string> > binderDatabaseStr;
	//std::map<int, std::vector<std::string>::iterator dataBaseIterator;

	int continueRunning = 1;

	/* Dictionary to store int to funcArgTypes mapping */
	std::map<int, funcStruct> funcToMap;

	/* Dictionary to store the server info */
	std::map<std::string, serverInfo> serverMap;
	
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
					std::cout<<"Binder Accepted Data...." <<std::endl;
					// First, get the length of the incoming message
					receiveInt(i, &messageLength, sizeof(messageLength), 0);
					std::cout<<"Message length: " <<messageLength<<std::endl;
					// Allocate the appropriate memory and get the message
					char *message;
					message = (char*) malloc (messageLength);

					// Next, get the type of the incoming message
					receiveInt(i, &messageType, sizeof(messageType), 0);
					std::cout<<"Message Type: " <<messageType<<std::endl;

					// TERMINATION
					if (messageType == TERMINATE) {
						std::cout<<"Binder Received termination request...." <<std::endl;
						continueRunning = 0;

						// Inform all the servers
						for (int i=0; i<=fdmax; i++) { // Send to both server and client. Client will ignore it.
							if (FD_ISSET(i, &read_fds) && i != binderSocket) { // Wrong, only send it to the servers on the dictionary
								
								// Send the length
								sendInt(i, &messageLength, sizeof(messageLength), 0);

								// Send the message (Just the type: TERMINATE)
								sendInt(i, &messageType, sizeof(messageType), 0);

							}
						}
						// Exit
						break;

					}

					// REGISTRATION OR LOC_REQUEST
					else {
						std::cout<<"Either REGISTER or LOC_REQUEST received"<<std::endl;

						if (messageType == REGISTER) {
							std::cout<<"Binder Received registration request...." <<std::endl;

							// Set the server ip, port, function name and argTypes array 
							
							// Get the serveridentifier
							// messageLength = SERVERIP;
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							char* newServerHostName=(char*)malloc(messageLength);
							receiveMessage(i, newServerHostName, messageLength, 0);
							std::cout<<"Message received: "<<newServerHostName<<std::endl;

							// Get the port
							// messageLength = SERVERPORT;
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							char* newServerPort=(char*)malloc(messageLength);
							receiveMessage(i, newServerPort, messageLength, 0);
							std::cout<<"Message received: "<<newServerPort<<std::endl;

							// Get the funcName
							// messageLength = FUNCNAMELENGTH;
							receiveInt(i, &messageLength, sizeof(messageLength), 0);
							char *funcName = (char*)malloc(messageLength);
							receiveMessage(i, funcName, messageLength, 0);
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


							std::cout<<"Message received: "<<argTypes[0]<<std::endl;

							// Create the funcName and argTypes pair for the dictionary
							// char *funNameAndArgTypes = (char*)malloc(FUNCNAMELENGTH+sizeOfArgTypesArray);
							// memset(funNameAndArgTypes, -1, FUNCNAMELENGTH+sizeOfArgTypesArray);
							// std::cout<<funNameAndArgTypes<<std::endl;
							// std::cout<<strlen(funNameAndArgTypes)<<std::endl;
							// memcpy(funNameAndArgTypes, funcName, strlen(funcName));
							// memcpy(funNameAndArgTypes+strlen(funcName), argTypes, sizeOfArgTypesArray);
							// std::cout<<funNameAndArgTypes<<std::endl;
							// std::cout<<strlen(funNameAndArgTypes)<<std::endl;

							char *serverIden = (char*)malloc(strlen(newServerHostName)+strlen(newServerPort)+1);
							memcpy(serverIden, newServerHostName, strlen(newServerHostName));
							char* delimiter=";";
							memcpy(serverIden+strlen(newServerHostName), delimiter, 1); // Divide the server ip and port by ';'
							memcpy(serverIden+strlen(newServerHostName)+1, newServerPort, strlen(newServerPort));
							std::string serverLocValue(serverIden);
							std::cout<<"new server value: "<<serverLocValue<<std::endl;
							
							serverInfo newserverInfo;// = (struct serverInfo*)malloc(sizeof (struct serverInfo));
							newserverInfo.sockfd = i;
							newserverInfo.serverIP = newServerHostName;
							newserverInfo.serverPort = newServerPort;
							std::map<std::string, serverInfo>::iterator it;
							it = serverMap.find(serverLocValue);
							if (it == serverMap.end()) {
								serverMap.insert ( std::pair<std::string, serverInfo>(serverLocValue,newserverInfo) );
							}
							
							
							int added = -1;
							int previouslyAdded = -1;

							int key;
							int found=-1;
							
							for (int k=0;k<funcToMap.size();k++) {
								int count=0;
								if (funcToMap.count(k+1)>0 && 
									strcmp (funcToMap[k+1].funcName, funcName)==0 && 
									funcToMap[k+1].argTypesLen ==  lengthOfargTypesArray) {

									for (int h=0; h<funcToMap[k+1].argTypesLen; h++) {
										if (funcToMap[k+1].argTypes[h]==argTypes[h]) {
											count +=1;
										}
										else {
											count-=1;
										}
									}
								}

								if (count == funcToMap[k+1].argTypesLen) {
									key = k+1;
									found = 1;
									break;
								}
								
							}

							// --- BAD FROM HERE....
							if (found ==1 && binderDatabaseStr.count(key)>0) { // A server already registered this funcArgTypes

								// It exists. Check if this server has already been added to the list for this specific funcArgTypes combo
								for (int g=0;g<binderDatabaseStr[key].size();g++) {
									if ( (binderDatabaseStr[key].at(g)).compare(serverLocValue) == 0 ) { // Already added

										// Update it
										//binderDatabaseStr[key].at(g) = serverLocValue;
										previouslyAdded = 1;

										break;

									}
								}
								if (previouslyAdded==-1) { // Not yet added
									binderDatabaseStr[key].push_back(serverLocValue);
								}

								// Update added
								added = 1;
							}
							// else { // Not yet registered
							// 	funcStruct newFunStruct;// = (struct funcStruct*)malloc(sizeof (struct funcStruct));
							// 	newFunStruct.funcName = funcName;
							// 	newFunStruct.argTypes = argTypes;
							// 	newFunStruct.argTypesLen = lengthOfargTypesArray;
							// 	key = funcToMap.size();
							// 	funcToMap.insert ( std::pair<int, funcStruct>(key,newFunStruct) );
							// 	//std::map<int, funcStruct> funcToMap;
							// 	//funcToMap[key] = newFunStruct; /// WROOOOOONNNNNGGG

							// 	// It doesn't exist, add it
							// 	std::vector <std::string> addThisValue;
							// 	addThisValue.push_back(serverLocValue);
							// 	binderDatabaseStr[key]= addThisValue;
								
							// 	// Update added
							// 	added = 1;
							// }
							// --- ....TO HERE!

							// Respond to the server
							uint32_t responseLength = 4; // Just 4 bytes
							uint32_t responseType;
							uint32_t responseMessage;

							// Send the message length
							sendInt(i, &responseLength, sizeof(responseLength), 0);
							
							//previouslyAdded = 1; // REMOVE THIS WHEN FIXED
							if (previouslyAdded == 1 ) {
								// Respond with REGISTER_SUCCESS as type and PREVIOUSLY_REGISTERED as message
								responseType = REGISTER_SUCCESS;
								responseMessage = PREVIOUSLY_REGISTERED;

								// Send the message Type
								sendInt(i, &responseType, sizeof(responseType), 0);

								// Send the message
								sendInt(i, &responseMessage, sizeof(responseMessage), 0);


							}
							else if (added ==1) {
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
							std::cout<<"Binder Received location request...." <<std::endl;
							// Set the function name and argTypes array
							char *funcName = (char*)malloc(FUNCNAMELENGTH);
							int *argTypes = (int*)malloc(messageLength-FUNCNAMELENGTH);
							int lengthOfargTypesArray = messageLength-FUNCNAMELENGTH;
							
							memcpy(funcName, message, FUNCNAMELENGTH);
							memcpy(argTypes, message+FUNCNAMELENGTH, lengthOfargTypesArray);

							// Find the ip address and port number of a server that can service the client's request
							char *funcArgTypesToFind = (char*)malloc(messageLength);
							memcpy(funcArgTypesToFind, message, messageLength);
							std::string funcArgTypesToFindKey(funcArgTypesToFind);

							uint32_t responseLength;
							uint32_t responseType;
							int key;

							// Check that such a func and argTypes combo exists
							if (binderDatabaseStr.count(key)>0) {
								// Respond with the server info
								responseLength = SERVERIP+SERVERPORT;
								responseType = LOC_SUCCESS;
								std::string serverThatCanHandle;
								char *serverInfo = (char*)malloc(SERVERIP+SERVERPORT);

								if (binderDatabaseStr[key].size()==1) {
									
									// Only one server can handle it
									serverThatCanHandle = binderDatabaseStr[key][0];
									strcpy(serverInfo, serverThatCanHandle.c_str());

									// Send the message length
									sendInt(i, &responseLength, sizeof(responseLength), 0);

									// Send the message Type
									sendInt(i, &responseType, sizeof(responseType), 0);

									// Send the message
									sendMessage(i, serverInfo, responseLength, 0);
								}

								else {
									// Multiple servers can handle it
									// Do round robin to find the appropriate server

									// For now, do it trivially and send the first server. 
									// Yet to implement round robin approach
									serverThatCanHandle = binderDatabaseStr[key][0];
									strcpy(serverInfo, serverThatCanHandle.c_str());

									// Send the message length
									sendInt(i, &responseLength, sizeof(responseLength), 0);

									// Send the message Type
									sendInt(i, &responseType, sizeof(responseType), 0);

									// Send the message
									sendMessage(i, serverInfo, responseLength, 0);
								}
								
								free(serverInfo);

							}
							else {
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
							free(funcArgTypesToFind);
						}

						// If error or no data, close socket with this client or server
						if (nbytes <= 0) {
							close(i);
							FD_CLR(i, &master_fd);
						}
						else {

						}
					}	

					free(message);
				}
			}
		}

		// if (messageType == TERMINATE) {
		// 	// Break again
		// 	break;
		// }

	}

	// Close binder socket
	close(binderSocket);
}
