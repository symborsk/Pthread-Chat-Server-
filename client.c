#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <signal.h>

#define	 MY_PORT  2222
#define  MaxUsernameLength 20
#define  MaxUsernames 20
#define  BufferSize 256

typedef struct users{
	int len;
	unsigned char username[MaxUsernameLength];
} user;
/* ---------------------------------------------------------------------
 This is a sample client program for the number server. The client and
 the server need not run on the same machine.				 
 --------------------------------------------------------------------- */
user* listUsers[MaxUsernameLength];

void * recieveHandler(void* socket);
void handShake(int s);
uint16_t lengthOfString(unsigned char* userName);
void sendUsername(int s);
void printUsernames();
void readMultipleUsernames(int s, int numberOfUsers);
void readAndAddUserName(int s);
void readAndRemoveUserName(int s);
void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes);
void printUsername();
void userAdded(char* username, int size);
void userRemoved(char* userName, int size);
void chat();
void sendBytes(int sock, unsigned char* buff, uint16_t numBytes);

struct timeval timer;

int main()
{
	int	s, first_confirmation, second_confirmation;
	struct	sockaddr_in	server;
	struct	hostent		*host;
	
	timer.tv_sec = 5;
	timer.tv_usec = 0;

	host = gethostbyname ("129.128.41.79");

	if (host == NULL) {
		perror ("Client: cannot get host description");
		exit (1);
	}

	s = socket(AF_INET, SOCK_STREAM, 0);

	if (s < 0) {
		perror ("Client: cannot open socket");
		exit (1);
	}

	bzero (&server, sizeof (server));
	bcopy (host->h_addr, & (server.sin_addr), host->h_length);
	server.sin_family = host->h_addrtype;
	server.sin_port = htons (MY_PORT);

	if (connect (s, (struct sockaddr*) & server, sizeof (server))) {
		perror ("Client: cannot connect to server");
		exit (1);
	}
	
	//Handshake includeds readinf of specific usernames as well as confirmation code
	handShake(s);
	sendUsername(s);

	pthread_t thread;
	//After the confirmation start a seperate thread for listening
	int ret = pthread_create(&thread, NULL, &recieveHandler, (void *)s);
	if(ret){
		printf("Error Creating Thread %d", ret);
	}

	chat(s);	
}

void * recieveHandler(void* socket){
	
	int newSocket = (int)socket;
	uint16_t code;
	unsigned char buff[BufferSize];
	memset(buff, '\0', BufferSize);
	while(1){
		
		recievedBytes(newSocket, buff, 1);
		code = (uint16_t)buff[0];
		
		//clear the buffer after we use it
		memset(buff, '\0', BufferSize);

		if(code == 0x00){
			
			//Recieving user message
			uint16_t networkSize;
			recievedBytes(newSocket, buff, 2);

			//This simply just allows us to cast two bytes to 2 byte ints
			networkSize = buff[1] << 8 | buff[0];
			uint16_t size = ntohs(networkSize);
		
			memset(buff, '\0', BufferSize);

			recievedBytes(newSocket, buff, size);
			printf("%s\n",buff);
			
			memset(buff, '\0', BufferSize);
		}
		else if(code == 0x01){
			
			readAndAddUserName(newSocket);
		}
		else if(code == 0x02){
			
			readAndRemoveUserName(newSocket);
		}

		else{
			printf("ServerMessage: %u\n", code);
			printf("Server sent uknown code");

		}
	}

}



void handShake(int s){
	unsigned char confirmation_one;
	unsigned char confirmation_two;
	
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0)
	 	error("setsockopt failed\n");
	
	// We cannot assume we will recieve 1 or 2 bytes so we need to account for those cases
	int bytesRecievied = recv(s, &confirmation_one, sizeof (confirmation_one), 0);
	
	if(bytesRecievied <= 0){
		perror("Server hung up or client timeout:");
	}

	bytesRecievied = recv(s, &confirmation_two, sizeof (confirmation_two), 0);

	if(bytesRecievied <= 0){
		perror("Server hung up or client timeout:");
	}
	
	uint16_t first_confirmation = (uint16_t)confirmation_one;
	uint16_t second_confirmation = (uint16_t)confirmation_two;
	if( first_confirmation != 0xCF || second_confirmation != 0xA7){
		perror ("Client: Did not recieve correct server authentication");
		exit(1);
	}

	unsigned char numberOfUsers;
	recv(s, &numberOfUsers, sizeof(numberOfUsers), 0);
	
	uint16_t intUsers  = (uint16_t)numberOfUsers;
	readMultipleUsernames(s, intUsers);

	timer.tv_sec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0){	
	 	perror("setsockopt failed second time\n");
	}
	
	printf("Connection Complete\n");
}

void readMultipleUsernames(int s, int numberOfUsers){
	
	int i;
	for( i = 0;  i < numberOfUsers ; i++ ){
		readAndAddUserName(s);
	}
}

void readAndAddUserName(int s){
		
		unsigned char buff[BufferSize];
		memset(buff, '\0', BufferSize);
		
		recievedBytes(s, buff, 1);

		uint16_t size = (uint16_t)buff[0];
		memset(buff, '\0', BufferSize);

		recievedBytes(s, buff, size);
		
		userAdded(buff, size);
}

void readAndRemoveUserName(int s){

		unsigned char buff[BufferSize];
		memset(buff, '\0', BufferSize);
		
		recievedBytes(s, buff, 1);

		uint16_t size = (uint16_t)buff[0];
		memset(buff, '\0', BufferSize);

		recievedBytes(s, buff, size);
		
		userRemoved(buff, size);
}

uint16_t lengthOfString(unsigned char* userName){
	
	int i;
	for(i = 0; i < BufferSize; i++){
		
		if(userName[i] == '\0' || userName[i] == '\n' )
			break;
	}

	return i;
}

void sendUsername(int s){

	unsigned char buff[MaxUsernameLength];
	printf("Enter username: \n>>");
	fgets(buff, MaxUsernameLength, stdin);
	
	//Put in the length at the start
	uint16_t sizeUser = lengthOfString(buff);
	unsigned char username[sizeUser+1];
	username[0] = (unsigned char)sizeUser;
	
	//Put in the username chars
	memcpy(&username[1], (void *)buff, sizeUser);
	 
	sendBytes(s, username, sizeof(username));
}

void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes){

	printf("Current file descriptor%d\n", sock);
	int numberToRead = numBytes;
	unsigned char * spotInBuffer = buff;
	while(numberToRead > 0){
		
		int readBytes = recv(sock, spotInBuffer, numberToRead, 0);
		if(readBytes <= 0){
			perror("recieve from to socket failed: ");
			close(sock);
			exit(1);			
		}

		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += readBytes;
		numberToRead -= readBytes;
	}
}

void sendBytes(int sock, unsigned char* buff, uint16_t numBytes){

	int numberToWrite = numBytes;
	unsigned char* spotInBuffer = buff;

 	while(numberToWrite > 0){
		int sentBytes = send(sock, spotInBuffer, numberToWrite, 0);

		if(sentBytes <= 0){
			perror("Socket close or server timeout");
			close(sock);
			exit(1);			
		}

		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += sentBytes;
		numberToWrite -= sentBytes;
	}
}

void chat(int socket){

	for(;;){
		
		unsigned char buff[BufferSize];
		// memset(buff, '\0', BufferSize);
		
		printf(">> ");
		fgets(buff, BufferSize, stdin);
		
		if(strncmp(buff, ".quit", 5) == 0){
			printf("exiting chatroom\n");
			close(socket);
			exit(1);
		}

		if(strncmp(buff, ".usernames", 10) == 0){
			printUsernames();
			continue;
		}
		
		uint16_t size = lengthOfString(buff);
		printf("Size of message: %d\n", size);
		uint16_t networkSize = htons(size);

		printf("Sending out the message: %s\n", buff);
		
		sendBytes(socket, (unsigned char *)&networkSize, 2);
		sendBytes(socket, buff, size);

		memset(buff, '\0', BufferSize);
		// Give the message time to send
		sleep(1);
	}
}

void userAdded(char* username, int size){
	int i;
	
	user * User = (user *)malloc(sizeof(user));
	User->len = size;
	memcpy(&(User->username), username, size);
	
	for(i = 0; i < MaxUsernames ; i++){
	
		//Look through our list and find the first free spot
		if(listUsers[i] == 0){
			listUsers[i] = User;
			break;
		}
	}

}

void userRemoved(char* userName, int size){
	
	int i;
	for(i = 0; i< MaxUsernames; i++){
		
		if(listUsers[i] == 0){
			continue;
		}

		if(strncmp(listUsers[i]->username, userName, size)){
			listUsers[i] = 0;
			free(&listUsers[i]);
			break;
		}
	}

}

void printUsernames(){
	int i;
	for(i = 0; i < MaxUsernames; i++){
		if(listUsers[i] == NULL){
			continue;
		}

		printf("%s\n", listUsers[i]->username);
	}
}



