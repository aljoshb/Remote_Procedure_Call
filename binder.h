
#ifdef __cplusplus
extern "C" {
#endif

/* Struct for storing the server info */
struct serverIdentifier {
	char *serverIP;
	char *serverPort;
};

/* Message types */

#define REGISTER          1
#define REGISTER_SUCCESS  2
#define REGISTER_FAILURE  3
#define LOC_REQUEST       4
#define LOC_SUCCESS       5
#define LOC_FAILURE       6
#define EXECUTE  		  7
#define EXECUTE_SUCCESS   8
#define EXECUTE_FAILURE   9
#define TERMINATE		  10

/* Errors */

#define BINDER_NOT_FOUND  -2

/* Warnings/Follow-up */

#define NEW_REGISTRATION   11
#define PREVIOUSLY_REGISTERED 12

/* Max length of function name, server ip and port */
#define FUNCNAMELENGTH 65
#define SERVERIP 256
#define SERVERPORT 4

#ifdef __cplusplus
}
#endif