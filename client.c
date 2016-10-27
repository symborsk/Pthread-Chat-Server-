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
#define  MaxLength 20
#define  BuffSize 200

/* ---------------------------------------------------------------------
 This is a sample client program for the number server. The client and
 the server need not run on the same machine.				 
 --------------------------------------------------------------------- */
uint16_t lengthOfUsername(unsigned char userName[MaxLength]){
	uint16_t i;
	for(i = 0; i < MaxLength; i++){

		
		if(userName[i] == '\0')
			break;
	}

	return i + 1;
}

// void sendUsername(unsigned char username[MaxLength], int socket){
// 	uint16_t sizeUser = lengthOfUsername(username);
// 	uint16_t mySize = sizeUser;
// 	uint16_t tmp = htons((uint16_t)mySize);
	
// 	send(socket, &tmp, sizeof(mySize), 0);
// 	send(socket , &username, sizeof(username), 0);
// }
void * recieveMessage(void* socket);

int main()
{
	int	s, first_confirmation, second_confirmation;

	struct	sockaddr_in	server;

	struct	hostent		*host;

	pthread_t thread;

	host = gethostbyname ("129.128.41.73");

	if (host == NULL) {
		perror ("Client: cannot get host description");
		exit (1);
	}

	s = socket (AF_INET, SOCK_STREAM, 0);

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

	int abc = read (s, &first_confirmation, sizeof (first_confirmation));
	printf("successfuly read: %d\n", abc);
	read (s, &second_confirmation, sizeof(second_confirmation));
	first_confirmation = ntohl (first_confirmation);
	second_confirmation = ntohl (second_confirmation);

	if( first_confirmation != 0xCF || second_confirmation != 0xA7){
		perror ("Client: Did not recieve correct server authentication");
		exit(1);
	}
	printf("Connection Complete\n");
	int ret = pthread_create(&thread, NULL, recieveMessage, (void *)s);
	if(ret){
		printf("Error Creating Thread %d", ret);
	}

	
	unsigned char username[MaxLength];
	printf("Enter username: ");
	scanf("%s", username);
	uint16_t sizeUser = lengthOfUsername(username);
	uint16_t mySize = sizeUser;
	uint16_t tmp = htons((uint16_t)mySize);
	
	int what = send(s, &tmp, sizeof(mySize), 0);
	send(s, &username, sizeof(username), 0);
	close(s);
	pthread_exit(NULL);
}

void * recieveMessage(void* socket){
	
	int newSocket = (int)socket;
	uint16_t sSize= 0;
	while(1){
		printf("RUNNING DA THREAD\n");
		recv(newSocket, &sSize, sizeof(sSize), 0);
		printf("Da Thread says: %d\n", sSize);
		if(sSize == 65535){
			close(newSocket);
			exit(1);
			break;
		}

		unsigned char buff[sSize];
		recv(newSocket, &buff, sizeof(buff), 0);
		break;

	}
}





