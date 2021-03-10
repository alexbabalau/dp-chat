#define ALPHA 26

struct Trie{

	int isUser;
	char* password;
	struct Trie* next[ALPHA]; 
};

int insertNewUser(struct Trie* node, char* username, char* password);
int checkCredentials(struct Trie* node, char* username, char* password);