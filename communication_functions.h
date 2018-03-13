
#ifdef __cplusplus
extern "C" {
#endif

int getTypeSize(int typeGotten);
int sendInt(int socketfd, uint32_t *numToSend, size_t length, int flags);
int receiveInt(int socketfd, uint32_t *numToReceive, size_t length, int flags);
int sendMessage(int socketfd, char *messageToSend, size_t length, int flags);
int receiveMessage(int socketfd, char *messageToReceive, size_t length, int flags);
int connection(const char *destIP, const char *destPort);
std::string getUniqueFunctionKey(char* funcName, int* argTypes);

#ifdef __cplusplus
}
#endif