#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define	MY_PORT	2222
#define MAXSIZE 20
/* ---------------------------------------------------------------------
 This	is  a sample server which opens a stream socket and then awaits
 requests coming from client processes. In response for a request, the
 server sends an integer number  such	 that  different  processes  get
 distinct numbers. The server and the clients may run on different ma-
 chines.
 --------------------------------------------------------------------- */
typedef struct usernames{
	int length;
	unsigned char usernamestr [MAXSIZE];
} username;
int main()
{
	int	sock, snew, fromlength, number, outnum;
	uint16_t inlength,usernameLength;
	uint32_t inusername;
	struct	sockaddr_in	master, from;


	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		perror ("Server: cannot open master socket");
		exit (1);
	}

	master.sin_family = AF_INET;
	master.sin_addr.s_addr = INADDR_ANY;
	master.sin_port = htons (MY_PORT);

	if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
		perror ("Server: cannot bind master socket");
		exit (1);
	}

	number = 0;

	listen (sock, 5);

	
	fromlength = sizeof (from);
	snew = accept (sock, (struct sockaddr*) & from, & fromlength);
	if (snew < 0) {
		perror ("Server: accept failed");
		exit (1);
	}
	number=0xcf;
	outnum = htonl (number);
	send (snew, &outnum, sizeof (outnum),0);

	number=0xa7;
	outnum = htonl (number);
	send (snew, &outnum, sizeof (outnum),0);
	// int myint=0;
	// int tmp=0;
	// read(snew,&tmp,sizeof(tmp));
	// myint=ntohl(tmp);
	// printf("%d",myint);

	inlength=0;
	int err=recv(snew,&inlength,sizeof(inlength),0);
	printf("%d %d\n",err,errno);
	printf("%u\n",inlength);
	usernameLength=ntohs(inlength);
	printf("%d\n",usernameLength);

	unsigned char usernamestr [usernameLength];
	recv(snew,&usernamestr,sizeof(usernamestr),0);
	
	
	// uint32_t unCodedName = ntohl(inusername);
    // printf("%u\n",unCodedName);
	// memcpy(username, (unsigned char*)&unCodedName, sizeof(unCodedName));
	printf("This: %s  is the username",usernamestr);
	username usernames [MAXSIZE];
	int i;
	for(i=0;i<MAXSIZE;i++){
		
	}
	close (snew);
		
	
}
