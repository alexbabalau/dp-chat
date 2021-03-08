#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include "netio.h"

int set_addr(struct sockaddr_in *addr, char* name, u_int32_t inaddr, short sin_port){

	addr -> sin_family = AF_INET;

	if(name != NULL){
		struct hostent* host;
		host = gethostbyname(name);
		if(host == NULL)
			return -1;
		addr -> sin_addr.s_addr = *(u_int32_t*) host -> h_addr_list[0];
	}
	else{
		addr -> sin_addr.s_addr = htonl(inaddr);
	}
	addr -> sin_port = htons(sin_port);
	return 0;
}

int stream_read(int sockfd, char* buf, int len){

	int remaining = len;
	while(remaining > 0){
		int number_read = read(sockfd, buf, remaining);
		if(number_read == -1)
			return -1;
		if(number_read == 0)
			break;
		buf += number_read;
		remaining -= number_read;
	}
	return len - remaining;

}

int stream_write(int sockfd, char* buf, int len){

	int remaining = len;
	while(remaining > 0){
		int number_wrote = write(sockfd, buf, remaining);
		if(number_wrote == -1)
			return -1;
		buf += number_wrote;
		remaining -= number_wrote;
	}
	return len - remaining;

}