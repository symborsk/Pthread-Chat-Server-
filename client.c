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

int main()
{
	int	s, first_confirmation, second_confirmation;

	struct	sockaddr_in	server;

	struct	hostent		*host;

	pthread_t thread;

	host = gethostbyname ("142.244.113.72");

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
	
	handShake(s);

	//After the confirmation start a seperate thread for listening
	int ret = pthread_create(&thread, NULL, recieveMessage, (void *)s);
	if(ret){
		printf("Error Creating Thread %d", ret);
	}

	sendUsername(s);	
	// unsigned char username[MaxUsernameLength];
	// printf("Enter username: ");
	// scanf("%s", username);
	// uint16_t sizeUser = lengthOfUsername(username);
	// uint16_t mySize = sizeUser;
	// uint16_t tmp = htons((uint16_t)mySize);
	
	// int what = send(s, &tmp, sizeof(mySize), 0);
	// send(s, &username, sizeof(username), 0);
	close(s);
	pthread_exit(NULL);
}

void * recieveMessage(void* socket){
	
	int newSocket = (int)socket;
	uint16_t sSize= 0;
	int i;
	while(1){
		recv(newSocket, &sSize, sizeof(sSize), 0);
		if(sSize == 65535){
			close(newSocket);
			exit(1);
			break;
		}

		unsigned char buff[sSize];
		recv(newSocket, &buff, sizeof(buff), 0);
		break;


		for(i=0; i<sSize; i++){
			printf("%c", sSize);
		}
		printf("\n");
	}

}

void handShake(int s){
	unsigned char confirmation[2];
	recv(s, &confirmation, sizeof (confirmation), 0);
	
	uint16_t first_confirmation = (uint16_t)confirmation[0];
	uint16_t second_confirmation = (uint16_t)confirmation[1];

	unsigned char numberOfUsers;
	recv(s, &numberOfUsers, sizeof(numberOfUsers), 0);
	uint16_t intUsers  = (uint16_t)numberOfUsers;
	readUsernames(intUsers);

	if( first_confirmation != 0xCF || second_confirmation != 0xA7){
		perror ("Client: Did not recieve correct server authentication");
		exit(1);
	}

	printf("Connection Complete\n");
}

void readUsernames(int numberOfUsers){
	int i;
	for(int i = 0; i < numberOfUsers ; i++ ){
		unsigned char buff[BufferSize];
		
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
	 
	printf("\nlength of username %u", username[0]);
	send(s, &username, sizeof(username), 0);
}





