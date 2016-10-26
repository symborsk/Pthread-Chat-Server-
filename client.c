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

#define	 MY_PORT  2222
#define  MaxLength 20

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

int main()
{
	int	s, first_confirmation, second_confirmation;

	struct	sockaddr_in	server;

	struct	hostent		*host;

	host = gethostbyname ("129.128.41.71");

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

	read (s, &first_confirmation, sizeof (first_confirmation));
	read (s, &second_confirmation, sizeof(second_confirmation));
	first_confirmation = ntohl (first_confirmation);
	second_confirmation = ntohl (second_confirmation);

	if( first_confirmation != 0xCF || second_confirmation != 0xA7){
		perror ("Client: Did not recieve correct server authentication");
		exit(1);
	}
	printf("Connection Complete\n");

	
	unsigned char username[MaxLength];
	printf("Enter username: ");
	scanf("%s", username);
	uint16_t sizeUser = lengthOfUsername(username);
	uint16_t mySize = sizeUser;
	uint16_t tmp = htons((uint16_t)mySize);
	
	send(s, &tmp, sizeof(mySize), 0);
	send(s , &username, sizeof(username), 0);

	
	printf("Error: %s\n",strerror(errno));
	close (s);
}





