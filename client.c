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

void * recieveHandler(void * unUsed);
void handShake();
uint16_t lengthOfString(unsigned char* userName);
void sendUsername();
void printUsernames();
void readMultipleUsernames(uint16_t numberOfUsers);
void readAndAddUserName();
void readAndRemoveUserName();
void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes);
void sendBytes(int sock, unsigned char* buff, uint16_t numBytes);
void printUsername();
void userAdded(char* username, int size);
void userRemoved(char* userName, int size);
void chat();
void sendKeepAliveMessage();
void InitializeSignalHandlers();
void shutdownClient(int signal);

struct timeval timer;

struct sigaction priorSigHandler;
struct sigaction currentSigHandler;
int socketFD;

int main(int argc, char *argv[])
{
    if(argc != 3){
    	printf("This program requires IP Address and Port number as two input arguments. Exiting...\n");
    	exit(1);
    }
    struct	sockaddr_in	server;
	struct	hostent		*host;

    host = gethostbyname(argv[1]);
    uint16_t port = atoi(argv[2]);
	
	InitializeSignalHandlers();
	timer.tv_sec = 5;
	timer.tv_usec = 0;

	if (host == NULL) {
		perror ("Client: cannot get host description");
		shutdownClient(1);
	}

	socketFD = socket(AF_INET, SOCK_STREAM, 0);

	if (socketFD < 0) {
		perror ("Client: cannot open socket");
		shutdownClient(1);
	}

	bzero (&server, sizeof (server));
	bcopy (host->h_addr, & (server.sin_addr), host->h_length);
	server.sin_family = host->h_addrtype;
	server.sin_port = htons(port);

	if (connect (socketFD, (struct sockaddr*) & server, sizeof (server))) {
		perror ("Client: cannot connect to server");
		shutdownClient(1);
	}
	
	//Handshake includeds readinf of specific usernames as well as confirmation code
	handShake();

	//After the confirmation start a seperate thread for listening
	pthread_t thread;
	int ret = pthread_create(&thread, NULL, &recieveHandler, 0);
	if(ret){
		printf("Error Creating Thread %d", ret);
		shutdownClient(1);
	}

	sendUsername();
	chat();	
}


void InitializeSignalHandlers(){

	currentSigHandler.sa_handler = shutdownClient;
	sigemptyset(&currentSigHandler.sa_mask);
	currentSigHandler.sa_flags = 0;
	sigaction(SIGINT, &currentSigHandler, &priorSigHandler);
}

void shutdownClient(int signal){

	//free all the malloced users close the socket
	int i;
	for(i = 0; i < MaxUsernames; i++){
		
		if(listUsers[i] == NULL){
			continue;
		}

		free(listUsers[i]);
	}

	printf("Exiting chatroom. Goodbye\n");

	//Leave it in the state we got it
	sigaction(SIGINT, &priorSigHandler, 0);
	
	// close(socket);
	exit(1);
}

void * recieveHandler(void * unUsed){
	
	//We passed in a void * that is an int * so cast it and dereference it
	uint16_t code;
	unsigned char buffUsername[BufferSize];
	unsigned char buffMessage[BufferSize];
	memset(buffUsername, '\0', BufferSize);
	memset(buffMessage, '\0', BufferSize);
	while(1){
		
		recievedBytes(socketFD, buffMessage, 1);
		code = (uint16_t)buffMessage[0];
		
		//clear the buffer after we use it
		memset(buffMessage, '\0', BufferSize);

		//User message
		if(code == 0x00){
				
			//Recieving user message
			unsigned char stringSizeChar;
			recievedBytes(socketFD, &stringSizeChar, 1);

			//Casting to an int to tell us how many expected bytes
			uint16_t stringSizeInt = (uint16_t)stringSizeChar;
			recievedBytes(socketFD, buffUsername, stringSizeInt);

			recievedBytes(socketFD, buffMessage, 2);
			//This simply just allows us to cast two bytes to 2 byte ints
			uint16_t networkSize = buffMessage[1] << 8 | buffMessage[0];
			uint16_t sizeMessage = ntohs(networkSize);
		
			//Clear our buffer before using it again for the message
			memset(buffMessage, '\0', BufferSize);

			recievedBytes(socketFD, buffMessage, sizeMessage);
			
			//This specific series of print statments just make a nice readable username: message string
			printf("%s : %s\n", buffUsername, buffMessage);
			
			memset(buffMessage, '\0', BufferSize);
			memset(buffUsername, '\0', BufferSize);
		}

		//Add user message
		else if(code == 0x01){
			
			readAndAddUserName();
		}

		//Remove user message
		else if(code == 0x02){
			
			readAndRemoveUserName();
		}

		//Something weird happened..... shouldnt happen
		else{
			printf("Server sent uknown code");
			shutdownClient(0);
		}
	}
}

void handShake(){
	unsigned char confirmation_one_byte;
	unsigned char confirmation_two_byte;
	
	//Make sure there is a timeout so the client doesn t wait for handshake forever
	if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0){
	 	perror("setsockopt failed\n");
	 	exit(1);
	}
	
	recievedBytes(socketFD, &confirmation_one_byte, sizeof(confirmation_one_byte));
	recievedBytes(socketFD, &confirmation_two_byte, sizeof(confirmation_two_byte));
	
	uint16_t first_confirmation = (uint16_t)confirmation_one_byte;
	uint16_t second_confirmation = (uint16_t)confirmation_two_byte;
	
	if( first_confirmation != 0xCF || second_confirmation != 0xA7){
		perror("Client: Did not recieve correct server authentication");
		exit(1);
	}

	unsigned char numberOfUsers;
	recievedBytes(socketFD, &numberOfUsers, sizeof(numberOfUsers));
	
	uint16_t intUsers  = (uint16_t)numberOfUsers;
	
	readMultipleUsernames(intUsers);

	//After the authentication is complete there is not timeout on the receive end of the user
	timer.tv_sec = 0;
	if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0){	
	 	perror("setsockopt failed\n");
	 	exit(1);
	}
}

void readMultipleUsernames(uint16_t numberOfUsers){
	
	int i;
	for( i = 0;  i < numberOfUsers ; i++ ){
		readAndAddUserName();
	}
}

void readAndAddUserName(){
		
	unsigned char buff[BufferSize];
	memset(buff, '\0', BufferSize);
	
	recievedBytes(socketFD, buff, 1);

	uint16_t size = (uint16_t)buff[0];
	memset(buff, '\0', BufferSize);

	recievedBytes(socketFD, buff, size);
	
	userAdded(buff, size);
}

void readAndRemoveUserName(){

	unsigned char buff[BufferSize];
	memset(buff, '\0', BufferSize);
	
	recievedBytes(socketFD, buff, 1);

	uint16_t size = (uint16_t)buff[0];
	memset(buff, '\0', BufferSize);

	recievedBytes(socketFD, buff, size);
	
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

void sendUsername(){

	unsigned char buff[MaxUsernameLength];
	printf("Enter username: \n>>");
	fgets(buff, MaxUsernameLength, stdin);
	
	//Put in the length at the start
	uint16_t sizeUser = lengthOfString(buff);
	unsigned char username[sizeUser+1];
	username[0] = (unsigned char)sizeUser;
	
	//Put in the username chars
	memcpy(&username[1], (void *)buff, sizeUser);
	 
	sendBytes(socketFD, username, sizeof(username));
}

void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes){

	int numberToRead = numBytes;
	unsigned char * spotInBuffer = buff;
	while(numberToRead > 0){
		
		int readBytes = recv(sock, spotInBuffer, numberToRead, 0);
		if(readBytes <= 0){
			printf("Server has hung up shutting down...\n");
			shutdownClient(0);			
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
			printf("Server has hung up shutting down...\n");
			shutdownClient(0);			
		}

		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += sentBytes;
		numberToWrite -= sentBytes;
	}
}

void chat(){

	for(;;){

		struct timeval tv;
    	fd_set fds;

    	tv.tv_sec = 3;
    	tv.tv_usec = 0;

   		FD_ZERO(&fds);
   		
    	FD_SET(STDIN_FILENO, &fds);

		if(select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) <= 0){
			
			sendKeepAliveMessage();
			continue;
		}

		unsigned char buff[BufferSize];
		fgets(buff, BufferSize, stdin);

		if(strncmp(buff, ".usernames", 10) == 0){
			printUsernames();
			continue;
		}
		
		uint16_t size = lengthOfString(buff);
		uint16_t networkSize = htons(size);

		sendBytes(socketFD, (unsigned char *)&networkSize, 2);
		sendBytes(socketFD, buff, size);

		//clear the buffer for next time
		memset(buff, '\0', BufferSize);
	}
}

void userAdded(char* username, int size){
	int i;
	
	user * User = (user *)malloc(sizeof(user));
	User->len = size;
	memcpy(&(User->username), username, size); 
	User->username[size] = '\0';
	
	for(i = 0; i < MaxUsernames ; i++){
	
		//Look through our list and find the first free spot
		if(listUsers[i] == 0){
			listUsers[i] = User;
			printf("%s has entered the chat room\n", User->username);
			return;
		}
	}

	printf("We have reached our limit of username storage\n");

}

void userRemoved(char* userName, int size){
	
	int i;
	for(i = 0; i< MaxUsernames; i++){
		
		if(listUsers[i] == 0){
			continue;
		}

		if(strncmp(listUsers[i]->username, userName, size) == 0){
			printf("%s has left the chat room\n", listUsers[i]->username);
			free(listUsers[i]);
			listUsers[i] = 0;
			return;
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

void sendKeepAliveMessage(){

	uint16_t zeroCode = 0;
	uint16_t networkZeroCode = htons(zeroCode);

	sendBytes(socketFD, (unsigned char *)&networkZeroCode, 2);
}





