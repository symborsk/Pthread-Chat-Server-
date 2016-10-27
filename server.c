#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#define	MY_PORT	2222
#define MAXSIZE 20
/* ---------------------------------------------------------------------
 This	is  a sample server which opens a stream socket and then awaits
 requests coming from client processes. In response for a request, the
 server sends an integer number  such	 that  different  processes  get
 distinct numbers. The server and the clients may run on different ma-
 chines.
 --------------------------------------------------------------------- */
typedef struct users{
	int length;
	unsigned char usernamestr [MAXSIZE];
	pid_t pid;
} user;

int main()
{	user listofuser[MAXSIZE];
	fd_set fd_active;
	fd_set read_fd_set;
	struct timeval timer;
	timer.tv_sec=5;
	timer.tv_usec=2000;
	
		

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

	listen (sock, 5);
	FD_ZERO(&fd_active);
	FD_SET(sock,&fd_active);

	for(;;){
		

		number = 0;
		user currentuser=listofuser[0];

		read_fd_set=fd_active;
		printf("got past select\n");
	
		if(select(FD_SETSIZE,&read_fd_set,NULL,NULL,NULL)<0){
			
			perror("timeout123");
			exit(1);
		}
		
		// pid_t forkstatus=fork();
		// if(forkstatus==0)
		// {
		int i=0;
		for(i;i<FD_SETSIZE;i++){			
			if(FD_ISSET(i, &read_fd_set)){
				if(i == sock){
					fromlength = sizeof (from);
					printf("this is snew: %d\n",snew);
					snew = accept (sock, (struct sockaddr*) & from, & fromlength);
					if(setsockopt(snew,SOL_SOCKET,SO_RCVTIMEO, (char *)&timer,sizeof(timer))<0){
						perror("setting timeout failed");
					}
					if (snew < 0) 
					{
						perror ("Server: accept failed");
						exit (1);
					}
					number=0xcf;
					outnum = htonl (number);
					send (snew, &outnum, sizeof (outnum),0);

					number=0xa7;
					outnum = htonl (number);
					send (snew, &outnum, sizeof (outnum),0);
					
					FD_SET(snew, &fd_active);
					// int myint=0;
					// int tmp=0;
					// read(snew,&tmp,sizeof(tmp));
					// myint=ntohl(tmp);
					// printf("%d",myint);

					inlength=-1;
					int a = recv(snew,&inlength,sizeof(inlength),0);
					printf(" a is %d\n", a);

					if(a  ==  -1){
						
						printf("closing\n");
						uint16_t exit_val = -1;
						//uint16_t outexit=htons(exit_val);
						send(snew,&exit_val,sizeof(exit_val),0); 
						//shutdown(snew,SHUT_RDWR);	
						close (snew);	
					}
					else{
						usernameLength=ntohs(inlength);
						printf("%d\n",usernameLength);

						unsigned char usernamestr [usernameLength];
						recv(snew,&usernamestr,sizeof(usernamestr),0);

						// uint32_t unCodedName = ntohl(inusername);
					    // printf("%u\n",unCodedName);
						// memcpy(username, (unsigned char*)&unCodedName, sizeof(unCodedName));
						printf("This: %s  is the username",usernamestr);
						currentuser.length=usernameLength;
						strcpy(currentuser.usernamestr,usernamestr);
						//currentuser.usernamestr=usernamestr;
						currentuser.pid=0;
						printf("%s\n",currentuser.usernamestr );
						close (snew);
					}
					
				}
			}
			else
			{
				FD_CLR(i,&fd_active);
			}
		}
		// }
		// else
		// {

		// }

	}	
	
}
