#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>

#include "netio.h"

#define SERVER_PORT 5555
#define SERVER_ADDRESS "127.0.0.1"

struct sockaddr_in localAddress, remoteAddress;

int readLine(int connfd, char* buf, int maxlen){
	int total = 0, cnt = 0;
	while(cnt < maxlen){
		int nread = read(connfd, buf + cnt, 1);
		
		if(nread == -1){
			printf("Read error\n");
			exit(-1);
		}
		if(nread == 0){
			buf[cnt] = '\0';
			return cnt;
		}
		if(buf[cnt] == '\n'){
			buf[cnt + 1] = '\0';
			return cnt;
		}
		cnt++;
	}
	buf[cnt] = '\0';
	return cnt;
}

int createSocket(){

	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	if(sockfd == -1){
		printf("Error at socket creation\n");
		exit(-1);
	}
	return sockfd;
}

void setLocalAddress(){
	if(set_addr(&localAddress, NULL, INADDR_ANY, 0) == -1){
		printf("Error at set_addr");
		exit(-1);
	}
}

void setRemoteAddress(){
	if(set_addr(&remoteAddress, SERVER_ADDRESS, 0, SERVER_PORT) == -1){
		printf("Error at set_addr");
		exit(-1);
	}
}

void bindSocket(int sockfd){
	if(bind(sockfd, (struct sockaddr*)&localAddress, sizeof(localAddress)) == -1){
		printf("Error at bind\n");
		exit(-1);
	}
}

void connectToServer(int sockfd){
	if(-1 == connect(sockfd, (struct sockaddr*)&remoteAddress, sizeof(remoteAddress))){
		printf("Error at connect\n");
		exit(-1);
	}
}

void* sendMessages(void* arg){
	int sockfd = *((int*)arg);
	char buf[1005];
	printf("Enter your message or command: ");
	fflush(stdout);
	while(readLine(1, buf, 1000) > 0){
		if(stream_write(sockfd, buf, strlen(buf)) < 0){
			printf("Stream write error\n");
			exit(-1);
		}
		printf("Enter your message or command: ");
		fflush(stdout);
	}
	return NULL;
}

void terminateChildProcess(pid_t pid){
	if(kill(pid, 15) == -1){
		printf("Error at kill\n");
		exit(-1);
	}
}

void waitForChildren(){
	int status = 0;
	while(wait(&status) > 0);
}

int main(){


	int fd = open("chat.txt", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);

	if(fd == -1){
		printf("Error at opening file\n");
		exit(-1);
	}

	pid_t pid = fork();

	

	if(pid == -1){
		printf("Error at fork\n");
		exit(-1);
	}

	if(pid == 0){
		execlp("xterm", "xterm", "-hold", "-e", "tail", "-f", "chat.txt", NULL);
		printf("Error at exec\n");
		exit(-1);
	}
	pid_t chatpid = pid;
	int sockfd = createSocket();

	setLocalAddress();

	bindSocket(sockfd);

	setRemoteAddress();

	connectToServer(sockfd);
	//pthread_attr_t attr;
	//pthread_t thread;
	//pthread_attr_init(&attr);
	//pthread_attr_setdetachstate(&attr, 1);
	pid = fork();
	if(pid == -1){
		printf("Error at fork\n");
		exit(-1);
	}
	if(pid == 0){
		sendMessages(&sockfd);
		exit(0);
	}
	pid_t messagePid = pid;
	char buf[1005];
	int nread = readLine(sockfd, buf, 1000);
	//printf("%d\n", nread);
	while(nread > 0){
		int bufLen = strlen(buf);
		if(write(fd, buf, bufLen) < bufLen){
			printf("Write Error\n");
			exit(-1);
		}
		nread = readLine(sockfd, buf, 1000);
	}
	terminateChildProcess(chatpid);
	terminateChildProcess(messagePid);
	waitForChildren();
	close(sockfd);
	close(fd);
	return 0;
}