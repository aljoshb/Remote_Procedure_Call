#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "rpc.h"
#include "binder.h"

int fdClientAccept = -1;
int fdBinder = -1;

int rpcInit() { /*Josh*/
	
	return 0;
}
int rpcCall(char* name, int* argTypes, void** args) { /*Josh*/

	return 0;
}
int rpcRegister(char* name, int* argTypes, skeleton f) { /*Josh*/
	
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
