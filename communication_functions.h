
#ifdef __cplusplus
extern "C" {
#endif

// message printing toggles
#define DEBUG_PRINT_ENABLED 0
#define ERROR_PRINT_ENABLED 0

/* Message types */
#define REGISTER 			1
#define REGISTER_SUCCESS	2
#define REGISTER_FAILURE	3
#define LOC_REQUEST     	4
#define LOC_SUCCESS     	5
#define LOC_FAILURE       	6
#define EXECUTE  		  	7
#define EXECUTE_SUCCESS   	8
#define EXECUTE_FAILURE   	9
#define TERMINATE		  	10

/* Errors/Reasoncode */
#define BINDER_NOT_FOUND  				-2
#define BINDER_UNABLE_TO_REGISTER 		-3
#define NO_SERVER_CAN_HANDLE_REQUEST 	-4
#define SERVER_CANNOT_HANDLE_REQUEST 	-5
#define SERVER_IS_OVERLOADED		 	-6
#define SERVER_DOES_NOT_HAVE_RPC		-7

/* Warnings/Follow-up Messages from Binder */
#define NEW_REGISTRATION   				11
#define PREVIOUSLY_REGISTERED 			12

/* Max length of function name, server ip and port */
#define FUNCNAMELENGTH 	65
#define SERVERIP 		256
#define SERVERPORT 		5

/* Type info */
#define ARG_CHAR 		1
#define ARG_SHORT 		2
#define ARG_INT 		3
#define ARG_LONG 		4
#define ARG_DOUBLE 		5
#define ARG_FLOAT 		6

/* Input/Output nature */
#define ARG_INPUT 		31
#define ARG_OUTPUT 		30

/* The right most two bytes of only array arguments in argTypes will have this value */
#define ITS_AN_ARRAY	48

uint32_t getTypeSize(uint32_t typeGotten);
int sendInt(int socketfd, uint32_t *numToSend, size_t length, int flags);
int receiveInt(int socketfd, uint32_t *numToReceive, size_t length, int flags);
int sendMessage(int socketfd, char *messageToSend, size_t length, int flags);
int receiveMessage(int socketfd, char *messageToReceive, size_t length, int flags);
int connection(const char *destIP, const char *destPort);
std::string getUniqueFunctionKey(char* funcName, int* argTypes);

#ifdef __cplusplus
}
#endif