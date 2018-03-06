
#ifdef __cplusplus
extern "C" {
#endif

int sendInt(int socketfd, uint32_t *numToSend, size_t length, int flags);
int receiveInt(int socketfd, uint32_t *numToReceive, size_t length, int flags);
int sendMessage(int socketfd, void *messageToSend, size_t length, int flags);
int receiveMessage(int socketfd, void *messageToReceive, size_t length, int flags);
int connection(const char *destIP, const char *destPort);

#ifdef __cplusplus
}
#endif