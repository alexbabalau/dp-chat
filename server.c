#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>

#include "netio.h"

#define SERVER_PORT 5555
#define SERVER_ADDRESS "0.0.0.0"
#define NUMBER_OF_USERS 4

char* usernames[] = {"alex", "daria", "miriam", "david"};
char* passwords[] = {"alex", "daria", "miriam", "david"};

char currentUser[1005];
int isAuthenticated[105];

struct sockaddr_in localAddress, remoteAddress;

int* connections[105];
char users[105][105];
int active[105], cntActive;
//char reply[1005];
int cntConnection;
pthread_mutex_t mutex;

enum check_result{NO_COMMAND, REGULAR_COMMAND, QUIT_COMMAND};

int authenticate(char* username, char* password){
	for(int i = 0; i < NUMBER_OF_USERS; i++){
		if(strcmp(username, usernames[i]) == 0){
			if(strcmp(password, passwords[i]) == 0){
				//isAuthenticated = 1;
				strcpy(currentUser, username);
				return 1;
			}
			else
				return 0;
		}
	}
	return 0;
}

void executeAuthCommand(char* command, int connectionIndex, char* reply){
	if(isAuthenticated[connectionIndex] == 1){
		sprintf(reply, "Already authenticated\n");
		return;
	}
	char* token;
	token = strtok(command, " \n");
	int cnt = 0;
	char username[105], password[105];
	while(token != NULL){
		cnt++;
		if(cnt == 2){
			strcpy(username, token);
		}
		if(cnt == 3){
			strcpy(password, token);
		}
		if(cnt > 3){
			sprintf(reply, "Too many arguments for auth command. The syntax is /auth username password\n");
			return;
		}
		token = strtok(NULL, " \n");
	}
	if(authenticate(username, password)){
		isAuthenticated[connectionIndex] = 1;
		sprintf(reply, "Authenticated as %s\n", username);
		strcpy(users[connectionIndex], username);
	}
	else{
		sprintf(reply, "User or password incorrect\n");
	}
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
	if(set_addr(&localAddress, NULL, INADDR_ANY, SERVER_PORT) == -1){
		printf("Error at set_addr");
		exit(-1);
	}
}

void bindSocket(int sockfd){
	if(bind(sockfd, (struct sockaddr*)&localAddress, sizeof(localAddress)) == -1){
		printf("Error at bind\n");
		perror("");
		exit(-1);
	}
}

void listenToClients(int sockfd){
	if(listen(sockfd, 5) == -1){
		printf("Error at listen\n");
		exit(-1);
	}
}

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
			return cnt + 1;
		}
		cnt++;
	}
	buf[cnt] = '\0';
	return cnt;
}

void stream_write_to_fd(int fd, char* message, int len){
	int nwr = stream_write(fd, message, len);
	if(nwr < len){
		printf("Error at writing in socket\n");
		exit(-1);
	}
}

void sendWelcome(int fd, char* message){
	sprintf(message, "Welcome to the chat\nUse /auth username password to authenticate yourself\nUse /quit to exit\n");
	stream_write_to_fd(fd, message, strlen(message));
}


void sendMessageToClients(char* reply){
	
	int replyLen = strlen(reply);
	pthread_mutex_lock(&mutex);
	
	for(int i = 1; i <= cntActive; i++){
		stream_write_to_fd(*connections[active[i]], reply, replyLen);
	}
	pthread_mutex_unlock(&mutex);
}

void swap(int* a, int* b){
	int aux = *a;
	*a = *b;
	*b = aux;
}

void endConnection(int conn, int* connfd, char* reply){

	if(isAuthenticated[conn]){
		sprintf(reply, "%s closed the chat\n", users[conn]);
		sendMessageToClients(reply);
	}
	pthread_mutex_lock(&mutex);
	int pos;
	for(int i = 1; i <= cntActive; i++){
		if(active[i] == conn){
			swap(&active[i], &active[cntActive]);
		}
	}
	--cntActive;
	pthread_mutex_unlock(&mutex);

	close(*connfd);
	free(connfd);
}

enum check_result checkForCommand(char* message, int conn, int* connfd, char* reply){
	if(strncmp("/auth ", message, 6) == 0){
		executeAuthCommand(message, conn, reply);
		stream_write_to_fd(*connfd, reply, strlen(reply));
		if(isAuthenticated[conn]){
			sprintf(reply, "%s just entered in the chat!\n", users[conn]);
			sendMessageToClients(reply);
		}
		return REGULAR_COMMAND;
	}
	if(strncmp("/quit\n", message, 6) == 0){

		return QUIT_COMMAND;
	}

	return NO_COMMAND;
}

int checkAuthentication(int conn, int* connfd, char* reply){
	if(!isAuthenticated[conn]){
		sprintf(reply, "Not authenticated. Please use the command /auth username password\n");
		stream_write_to_fd(*connfd, reply, strlen(reply));
		return 0;
	}
	return 1;
}

int addConnection(int* connfd){
	int conn;
	pthread_mutex_lock(&mutex);
	connections[++cntConnection] = connfd;
	active[++cntActive] = cntConnection;
	conn = cntConnection;
	pthread_mutex_unlock(&mutex);
	return conn;
}

void fillReplyWithUserMessage(int conn, char* reply, char* message){
	sprintf(reply, "%s says:%s", users[conn], message);
}


void processChatMessage(int conn, int* connfd, char* reply, char* message){
	if(!checkAuthentication(conn, connfd, reply))
		return;
	fillReplyWithUserMessage(conn, reply, message);
	sendMessageToClients(reply);
}

void* handleClient(void* arg){
	int* connfd = (int*) arg;
	int i, conn;

	conn = addConnection(connfd);

	char line[1005], reply[1005];
	sendWelcome(*connfd, reply);
	
	while(1){
		int len = readLine(*connfd, line, 1000);
		if(len == 0)
			break;
		enum check_result result = checkForCommand(line, conn, connfd, reply);
		if(result == QUIT_COMMAND)
			break;
		if(result == NO_COMMAND){
			processChatMessage(conn, connfd, reply, line);	
		}
	}
	
	endConnection(conn, connfd, reply);
	return NULL;

}

void runServer(int sockfd){
	int remoteLen = sizeof(remoteAddress);

	pthread_t thread;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, 1);
	pthread_mutex_init(&mutex, NULL);
	//int remoteLen;

	while(1){
		int* connfd = (int*) malloc(sizeof(int));
		if(connfd == NULL){
			printf("malloc error");
			exit(-1);
		}

		*connfd = accept(sockfd, (struct sockaddr*) &remoteAddress, &remoteLen);
		if(*connfd == -1){
			printf("Accept error\n");
			exit(-1);
		}

		if(0 != pthread_create(&thread, &attr, handleClient, (void *)connfd)){
			printf("Thread creation error\n");
			exit(-1);
		}
	}
}

int main(){

	int sockfd = createSocket();

	setLocalAddress();

	bindSocket(sockfd);

	listenToClients(sockfd);

	runServer(sockfd);

	return 0;
}