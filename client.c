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

#define	 MY_PORT  2222
#define  MaxUsernameLength 20
#define  BufferSize 256

/* ---------------------------------------------------------------------
 This is a sample client program for the number server. The client and
 the server need not run on the same machine.				 
 --------------------------------------------------------------------- */

void * recieveMessage(void* socket);
void handShake(int s);
uint16_t lengthOfUsername(unsigned char userName[MaxUsernameLength]);
void sendUsername(int s);
void readUsernames(int s, int numberOfUsers);
void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes);

struct timeval timer;
int main()
{
	int	s, first_confirmation, second_confirmation;
	struct	sockaddr_in	server;
	struct	hostent		*host;
	pthread_t thread;

	
	timer.tv_sec = 5;
	timer.tv_usec = 0;

	host = gethostbyname ("129.128.41.43");

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

	//After the confirmation start a seperate thread for listening
	int ret = pthread_create(&thread, NULL, recieveMessage, (void *)s);
	if(ret){
		printf("Error Creating Thread %d", ret);
	}

		
	close(s);
	pthread_exit(NULL);
}

void * recieveMessage(void* socket){
	
	int newSocket = (int)socket;
	unsigned char sSize;
	while(1){
		
		int recievedBytes = recv(newSocket, &sSize, sizeof(sSize), 0);
		printf("%d", recievedBytes);
		if(recievedBytes <= 0){
			close(newSocket);
			exit(1);
		}

		unsigned char buff[sSize];
		recievedBytes = recv(newSocket, &buff, sizeof(buff), 0);
		printf("%d", recievedBytes);
		if(recievedBytes <= 0){
			close(newSocket);
			exit(1);
		}

		break;

		int i;
		for( i=0; i<sSize; i++){
			printf("%c", sSize);
		}
		printf("\n");
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
	readUsernames(s, intUsers);

	timer.tv_sec = 0;
	if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0)
	 	perror("setsockopt failed second time\n");
	
	printf("Connection Complete\n");
}

void readUsernames(int s, int numberOfUsers){
	printf("Number of users %d\n", numberOfUsers);
	int i;
	for( i = 0;  i < numberOfUsers ; i++ ){
		unsigned char size;
		int bytesRecievied = recv(s, &size, sizeof(size), 0);
		
		if(bytesRecievied <= 0){
			printf("This many bytes recieve: %d\n", bytesRecievied);
			perror("Server Closed");
			exit(1);
		}
		
		uint16_t intSize = (uint16_t)size;
		unsigned char buff[intSize];
		printf("trying to read %d bytes\n", intSize);
		recievedBytes(s, buff, intSize);
		
		printf("Username %s has entered the chatroom", buff);
	}
}	

uint16_t lengthOfUsername(unsigned char userName[MaxUsernameLength]){
	uint16_t i;
	for(i = 0; i < MaxUsernameLength; i++){

		
		if(userName[i] == '\0')
			break;
	}

	return i + 1;
}

void sendUsername(int s){

	unsigned char buff[MaxUsernameLength];
	printf("Enter username: \n");
	scanf("%s", buff);
	unsigned char username[MaxUsernameLength];
	
	//Put in the length at the start
	uint16_t sizeUser = lengthOfUsername(buff);
	username[0] = (unsigned char) sizeUser;
	
	//Put in the username chars
	memcpy(&username[1], (void *)buff, sizeUser-1);
	 
	send(s, &username, sizeof(username), 0);
}

void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes){

	int numberToRead = numBytes;
	unsigned char * spotInBuffer = buff;
	
	while(numberToRead > 0){
		
		int recievedBytes = recv(sock, spotInBuffer, numberToRead, 0);
		if(recievedBytes <= 0){
			perror("Socket close or server timeout");
			exit(1);			
		}

		printf("recieved byte : %d\n", recievedBytes);

		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += recievedBytes;
		numberToRead -= recievedBytes;
	}
}





