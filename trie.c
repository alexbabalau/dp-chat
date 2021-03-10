#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "trie.h"

void fillPassword(struct Trie* node, char* password){
	int passLen = strlen(password);
	node -> password = malloc(sizeof(char) * (passLen + 1));
	if(node -> password == NULL){
		printf("Malloc error\n");
		exit(-1);
	}
	strcpy(node -> password, password);
}

struct Trie* newNode(){
	struct Trie* node = malloc(sizeof(struct Trie));
	if(node == NULL){
		printf("Error at malloc\n");
		exit(-1);
	} 
	node -> isUser = 0;
	node -> password = NULL;
	return node;
}

int insertNewUser(struct Trie* node, char* username, char* password){
	if(*username == '\0'){
		if(node -> isUser){
			return 0;
		}
		node -> isUser = 1;
		fillPassword(node, password);
		return 1;
	}

	char letter = *username - 'a';
	if(node -> next[letter] == NULL){
		node -> next[letter] = newNode();
	}
	insertNewUser(node -> next[letter], username + 1, password);
}

int checkCredentials(struct Trie* node, char* username, char* password){
	if(node == NULL)
		return -2;
	if(*username == '\0'){
		if(node -> isUser == 0){
			return 0;
		}
		if(strcmp(node -> password, password) == 0){
			return 1;
		}
		else
			return 0;
	}
	return checkCredentials(node -> next[*username - 'a'], username + 1, password);
}



