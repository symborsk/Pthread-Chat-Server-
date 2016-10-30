#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define	MY_PORT	2222
#define MAXSIZE 20

/* ---------------------------------------------------------------------
 This	is  a sample server which opens a stream socket and then awaits
 requests coming from client processes. In response for a request, the
 server sends an integer number  such	 that  different  processes  get
 distinct numbers. The server and the clients may run on different ma-
 chines.
 --------------------------------------------------------------------- */
void handShake(int snew);
uint16_t lengthOfUsername(unsigned char userName[MAXSIZE]);
void sendCurrentUserNames(int snew);
void getUsername(int snew);
void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes);
void * recieveMessage(void* socket);

typedef struct users{
	int length;
	unsigned char usernamestr [MAXSIZE];
	pid_t pid;
} user;

int number = 0;
int numberofclients=0;
int fd;
pid_t forkstatus;
user listofusers[MAXSIZE];
// volatile user* sharedMem=listofusers;

int main(){	
	
	fd_set fd_active;
	fd_set read_fd_set;
	struct timeval timer;
	timer.tv_sec=5;
	timer.tv_usec=2000;
	forkstatus =0;

	// fd = shm_open("/myregion", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
	// if(fd==-1){
	// 	printf("error opening fd\n");
	// }
	// if(ftruncate(fd,sizeof(listofusers))==1){
	// 	printf("truncate error\n");
	// }
	// sharedMem = mmap(NULL, sizeof(user)*MAXSIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fd, 0);
	
	// if(sharedMem == MAP_FAILED){
	// 	printf("Error: %s\n", strerror(errno));
	// 	exit(1);
	// }

	int	sock, snew, fromlength, number, outnum;
	uint16_t inlength,usernameLength, in_uint16_t;
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
		

		read_fd_set=fd_active;

		if(select(FD_SETSIZE,&read_fd_set,NULL,NULL,NULL)<0){
			
			
			perror("select()");
			exit(1);
		}
		
		

		//Only want to frok if we are at the parent


		int i=0;
		for(i;i<FD_SETSIZE;i++){			
			if(FD_ISSET(i, &read_fd_set)){
				printf("numberofclients %d  forkstatus: %d\n",numberofclients, forkstatus);
				if(i == sock){
					fromlength = sizeof (from);
					
					snew = accept (sock, (struct sockaddr*) & from, & fromlength);

					if(setsockopt(snew,SOL_SOCKET,SO_RCVTIMEO, (char *)&timer, sizeof(timer))<0){
						perror("setting timeout failed");
					}
					if (snew < 0) 
					{
						perror ("Server: accept failed");
						exit (1);
					}
					
					handShake(snew);
					
					//Child process
					if(forkstatus == 0){
						close(sock);
						printf("child proccess\n");
						FD_SET(snew, &fd_active);
						

						//chatting
						for(;;){
							int a = recv(snew,&inlength,sizeof(inlength),0);
							printf("a is : %d\n",a );
							if(a<=0){

								printf("exiting\n");
								break;
							}
						}
						// in_uint16_t=-1;
						// int a = recv(snew,&inlength,sizeof(inlength),0);
						

						// if(a  ==  -1){
							
						// 	uint16_t exit_val = -1;
						// 	//uint16_t outexit=htons(exit_val);
						// 	send(snew,&exit_val,sizeof(exit_val),0); 
						// 	//shutdown(snew,SHUT_RDWR);	
						// 	close (snew);	
						// }
						// else{
						// 	// usernameLength=ntohs(inlength);
						

						// 	// unsigned char usernamestr [usernameLength];
						// 	// recv(snew,&usernamestr,sizeof(usernamestr),0);
							
							
						// 	// currentuser.length=usernameLength;
						// 	// strcpy(currentuser.usernamestr,usernamestr);
						// 	// currentuser.pid=0;


						// 	close(snew);
						// 	exit(1);
						// }
						close(snew);
						exit(1);
					}
					else{
						close(snew);	
					}
				}
				
				else
				{
					FD_CLR(i,&fd_active);
				}
			}

		}	
	
	}
}

void handShake(int snew){
 	uint16_t outnum=0;

 	unsigned char outarray [2];
	uint16_t holdingNumber = 0xcf;
	outarray[0] = (unsigned char) (holdingNumber);
	holdingNumber = 0xa7;
	outarray[1] = (unsigned char) (holdingNumber);
	send (snew, &outarray, sizeof (outarray),0);

	unsigned char numberOfUsers = (unsigned char)numberofclients; 
	printf("Number of users  %d\n",numberofclients);
	send (snew,&numberOfUsers, sizeof(numberOfUsers),0);
	sendCurrentUserNames(snew);
	getUsername(snew);	
}	

void sendCurrentUserNames(int snew){
	int i;
	printf("numberofclients %d\n",numberofclients);
	for(i=0;i<numberofclients;i++){
		user currentuser= listofusers[i];
		
		printf("this is the sent usernamestr %s\n", currentuser.usernamestr );
		printf("this is the username length sent %d\n", currentuser.length );

		
		unsigned char length = (unsigned char) currentuser.length;
		printf("length from array %d\n", (uint16_t) length);
		send(snew, &length, sizeof(length), 0);

		// unsigned char username [currentuser.length];
		// memcpy(username, (void *)currentuser.usernamestr, currentuser.length);

		int sendval  = send(snew, (void *)currentuser.usernamestr, currentuser.length ,0 );
		// printf("%s\n",username );
		// memset(username, '\0', currentuser.length);
		if(sendval<=0){
			printf("send failed\n");
		}


	}

}
uint16_t lengthOfUsername(unsigned char userName[MAXSIZE]){
	uint16_t i;
	for(i = 0; i < MAXSIZE; i++){

		if(userName[i] == '\0')
			break;
	}

	return i + 1;
}

void getUsername(snew){
	
	unsigned char sizeOfUsername;
	int receivebytes_val  = recv(snew, &sizeOfUsername, sizeof(sizeOfUsername), 0);
	if(receivebytes_val<=0){
		close(snew);
		printf("closing socket\n");
		exit(1);
	}
	uint16_t temp=(uint16_t) sizeOfUsername;
	unsigned char buff [temp];

	recievedBytes(snew,buff,sizeOfUsername);
	user * currentuser = &listofusers[numberofclients];

	currentuser->length=sizeOfUsername;
	memcpy(currentuser->usernamestr, (void *)buff, sizeof(buff));
	
	numberofclients++;

	pid_t tempfork = fork();
	forkstatus = tempfork;
	currentuser->pid = forkstatus;

	printf("temp_fork%d\n", tempfork);
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