#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

int set_addr(struct sockaddr_in *addr, char* name, u_int32_t inaddr, short sin_port);
int stream_read(int sockfd, char* buf, int len);
int stream_write(int sockfd, char* buf, int len);