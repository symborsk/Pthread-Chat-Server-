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
#include <pthread.h>

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
	int fd;
	int length;
	unsigned char usernamestr [MAXSIZE];
} user;

void * handShake(void*  u);
uint16_t lengthOfUsername(unsigned char userName[MAXSIZE]);
void sendCurrentUserNames(int snew);
void getUsername(void* u);
void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes);
void * recieveMessage(void* socket);
void removeUser(void* u);
void addUser(void* u);
void chat(void* u);
void sendMessage(char* p, uint16_t size, unsigned char type);

int numberofclients;
int fd;
pid_t forkstatus;
user * listofusers[MAXSIZE];
// volatile user* sharedMem=listofusers;

int main(){	
	numberofclients=0;
	
	fd_set fd_active;
	fd_set read_fd_set;
	struct timeval timer;
	timer.tv_sec=8;
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
	read_fd_set=fd_active;
	pthread_t thread;
	
	for(;;){
		

		if(select(FD_SETSIZE,&read_fd_set,NULL,NULL,NULL)<0){
			perror("select()");
			exit(1);
		}
		
		

		//Only want to frok if we are at the parent

		int i=0;
		for(i;i<FD_SETSIZE;i++){			
			if(FD_ISSET(i, &read_fd_set)){
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
					
					user* newUser = (user *)malloc(sizeof(user));
					newUser->fd = snew;
					
					//Create a listener thread
					int ret = pthread_create(&thread, NULL, &handShake, (void*)newUser);
				}
				
				else
				{
					FD_CLR(i,&fd_active);
				}
			}

		}	
	
	}
}

void addUser(void * u){
	user * currentUser = (user *)u;
	int i;
	for(i = 0; i< MAXSIZE ; i++){
		
		if(listofusers[i] == NULL){
			listofusers[i] = u;
			return;
		}
	}

	printf("Chatroom is full\n");
}

void removeUser(void* u){
	user * currentUser = (user *)u;
	int i;
	for(i = 0; i< MAXSIZE ; i++){
		
		if(listofusers[i]->fd == currentUser->fd){
			listofusers[i] = NULL;
			free(currentUser);
		}
	}
}

void * handShake(void* u){
 	user* currentUser = (user *) u;
 	uint16_t outnum=0;

 	unsigned char outarray [2];
	uint16_t holdingNumber = 0xcf;
	outarray[0] = (unsigned char) (holdingNumber);
	holdingNumber = 0xa7;
	outarray[1] = (unsigned char) (holdingNumber);
	send (currentUser->fd, &outarray, sizeof (outarray),0);

	unsigned char numberOfUsers = (unsigned char)numberofclients; 
	printf("Number of users  %d\n", numberofclients);
	send (currentUser->fd,&numberOfUsers, sizeof(numberOfUsers),0);
	sendCurrentUserNames(currentUser->fd);
	getUsername(currentUser);
	chat(currentUser);

}	

void sendCurrentUserNames(int snew){
	int i;
	printf("numberofclients %d\n",numberofclients);
	for(i=0;i <MAXSIZE;i++){
		user* currentuser = listofusers[i];
		
		if(currentuser == NULL){
			continue;
		}
		
		printf("this is the sent usernamestr %s\n", currentuser->usernamestr );
		printf("this is the username length sent %d\n", currentuser->length );

		
		unsigned char length = (unsigned char) currentuser->length;
		printf("length from array %d\n", (uint16_t) length);
		send(snew, &length, sizeof(length), 0);


		int sendval  = send(snew, (void *)currentuser->usernamestr, currentuser->length ,0 );
		
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

void getUsername(void* u){
	user* currentUser = (user *) u;
	
	unsigned char sizeOfUsername;
	int receivebytes_val  = recv(currentUser->fd, &sizeOfUsername, sizeof(sizeOfUsername), 0);
	if(receivebytes_val<=0){
		close(currentUser->fd);
		printf("closing socket\n");
		free(currentUser);
		pthread_exit(NULL);
	}
	
	uint16_t temp = (uint16_t) sizeOfUsername;
	unsigned char buff [temp];

	recievedBytes(currentUser->fd, buff, sizeOfUsername);
	memcpy(currentUser->usernamestr, (void *)buff, sizeof(buff));

	currentUser->length=sizeOfUsername;
	numberofclients++;
	addUser(currentUser); 
		
	// // pid_t tempfork = fork();
	// // forkstatus = tempfork;
	// // currentuser->pid = forkstatus;

	// printf("temp_fork%d\n", tempfork);
}

void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes){

	int numberToRead = numBytes;
	unsigned char * spotInBuffer = buff;
	
	while(numberToRead > 0){
		
		int recievedBytes = recv(sock, spotInBuffer, numberToRead, 0);
		if(recievedBytes <= 0){
			perror("Socket close or server timeout");
			close(sock);
			pthread_exit(NULL);
						
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
			pthread_exit(NULL);
		}

		unsigned char buff[sSize];
		recievedBytes = recv(newSocket, &buff, sizeof(buff), 0);
		printf("%d", recievedBytes);
		if(recievedBytes <= 0){
			close(newSocket);
			pthread_exit(NULL);
		}

		break;

		int i;
		for( i=0; i<sSize; i++){
			printf("%c", sSize);
		}
		printf("\n");
	}

}

void chat(void* u){
	user * currentUser = (user *)u;

	uint16_t inlength;
	for(;;){
		unsigned char buffSize[2];
		recievedBytes(currentUser->fd, buffSize, 2);
		
		uint16_t temp = buffSize[1] << 8 + buffSize[0];
		uint16_t size = ntohs(temp);
		printf("Size is %d: \n",size);

		unsigned char buffMessage[size];
		recievedBytes(currentUser->fd, buffMessage, size);
		
		buffMessage[size] = '\0';
		printf("Server Recieved message %s \n", buffMessage);
		unsigned char type=(unsigned char)0x0;
		
		uint16_t outsize=size+(currentUser->length)+2;
		unsigned char outMessage[outsize];

	
		
		memcpy(outMessage,(void*) currentUser->usernamestr,size);
		
		memset(&outMessage[currentUser->length], ':',1);
		memset(&outMessage[currentUser->length+1 ], ' ',1);

		
		memcpy(&outMessage[currentUser->length +2], (void *) buffMessage,size);
		
		sendMessage(outMessage,outsize,type);

		memset(buffMessage, '\0', size);
	
	}
	
	close(currentUser->fd);
	pthread_exit(NULL);
}
 
void sendMessage(char* p, uint16_t size, unsigned char type){


	int i;
	for(i = 0; i < MAXSIZE; i++){
		user * currentUser  = listofusers[i];
		
		if(currentUser != NULL){
			send(currentUser->fd, &type, sizeof(type), 0);
			uint16_t hostToNet=htons(size);
			send(currentUser->fd,&hostToNet,sizeof(hostToNet),0);
			send(currentUser->fd,p,size,0);
		}

	}
}
