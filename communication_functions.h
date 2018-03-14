
#ifdef __cplusplus
extern "C" {
#endif

uint32_t getTypeSize(uint32_t typeGotten);
int sendAny(int socketfd, void *args, size_t messageLength, int flags, uint32_t typeArg, uint32_t arrayLen);
int receiveAny(int socketfd, void *args, size_t messageLength, int flags, uint32_t typeArg, uint32_t arrayLen);
int sendInt(int socketfd, uint32_t *numToSend, size_t length, int flags);
int receiveInt(int socketfd, uint32_t *numToReceive, size_t length, int flags);
int sendMessage(int socketfd, char *messageToSend, size_t length, int flags);
int receiveMessage(int socketfd, char *messageToReceive, size_t length, int flags);
int connection(const char *destIP, const char *destPort);
std::string getUniqueFunctionKey(char* funcName, int* argTypes);

#ifdef __cplusplus
}
#endif