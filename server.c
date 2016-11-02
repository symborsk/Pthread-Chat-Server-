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

pthread_rwlock_t lock=PTHREAD_RWLOCK_INITIALIZER;
/* ---------------------------------------------------------------------
 This	is  a sample server which opens a stream socket and then awaits
 requests coming from client processes. In response for a request, the
 server sends an integer number  such	 that  different  processes  get
 distinct numbers. The server and the clients may run on different ma-
 chines.
 --------------------------------------------------------------------- */
typedef struct users{
	int fd;
	uint16_t length;
	unsigned char usernamestr [MAXSIZE];
} user;

void * handShake(void*  u);
uint16_t lengthOfUsername(unsigned char userName[MAXSIZE]);
void sendCurrentUserNames(user * targetUser);
void getUsername(void* u);
void recievedBytes(user * currentUser, unsigned char* buff, uint16_t numBytes);
//void * recieveMessage(void* socket);
void removeUser(user* u);
void addUser(user* u);
void chat(user* u);
void sendMessage(char* p, uint16_t size, unsigned char type);
void sendBytes(user * currentUser, unsigned char* buff, uint16_t numBytes);
void sendJoin(user * currentUser);
void sendExit(user * currentUser);

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
	timer.tv_sec=20;
	timer.tv_usec=0;
	forkstatus =0;

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
					memset(newUser->usernamestr,'\0',MAXSIZE);
					newUser->fd = snew;
					
					//Create a listener thread
					printf("making a new thread\n");
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

void addUser(user* u){
	int i;
	printf("Taking the lock for adduser\n");
	pthread_rwlock_wrlock(&lock);
	for(i = 0; i< MAXSIZE ; i++){
		
		if(listofusers[i] == NULL){
			continue;
		}

		printf("Comparing %s, to %s\n", listofusers[i]->usernamestr, u->usernamestr);
		if(strncmp(listofusers[i]->usernamestr, u->usernamestr,(u->length > listofusers[i]->length)? u->length:listofusers[i]->length)==0){
				close(u->fd);
				//free(u);
				pthread_rwlock_unlock(&lock);
				printf("Freed the lock adduser.\n");
				return;
		}
		printf("past Comparing\n");
	}
	for(i=0;i<MAXSIZE;i++){
		

		if(listofusers[i] == NULL){	
			
			listofusers[i] = u;
			numberofclients++;
			pthread_rwlock_unlock(&lock);
			printf("Freed the lock adduser.\n");
			sendJoin(u);
			return;
		}
	}
	
	pthread_rwlock_unlock(&lock);
	printf("Freed the lock adduser.\n");
}

void removeUser(user * u){
	
	int i;
	printf("Taking the lock removeuser.\n");
	pthread_rwlock_wrlock(&lock);
	for(i = 0; i< MAXSIZE ; i++){
		
		if(listofusers[i] == NULL){
			continue;
		}

		if(listofusers[i]->fd == u->fd){
			listofusers[i] = NULL;
			numberofclients--;
			pthread_rwlock_unlock(&lock);
			printf("Freed the lock remove user.\n");
			sendExit(u);
			return;
		}
	}

	pthread_rwlock_unlock(&lock);
	printf("Freed the lock remove user.\n");
}

void * handShake(void* u){
 	user* currentUser = (user *) u;
 	uint16_t outnum=0;

 	unsigned char outarray [2];
	uint16_t holdingNumber = 0xcf;
	outarray[0] = (unsigned char) (holdingNumber);
	holdingNumber = 0xa7;
	outarray[1] = (unsigned char) (holdingNumber);
	//send (currentUser->fd, &outarray, sizeof (outarray),0);
	sendBytes(currentUser,outarray,2);

	unsigned char numberOfUsers = (unsigned char)numberofclients; 
	send (currentUser->fd,&numberOfUsers, sizeof(numberOfUsers),0);
	
	sendCurrentUserNames(currentUser);
	getUsername(currentUser);
	chat(currentUser);

}	

void sendCurrentUserNames(user * targetUser){
	int i;
	printf("Took the lock sendCurrentUserNames user.\n");
	pthread_rwlock_rdlock(&lock);

	for(i=0;i <MAXSIZE;i++){

		user* currentuser = listofusers[i];
		
		if(currentuser == NULL){
			continue;
		}
		unsigned char outLength = (unsigned char)currentuser->length;
		
		sendBytes(targetUser, &outLength, 1);
		sendBytes(targetUser,currentuser->usernamestr,currentuser->length);
		
	}

	pthread_rwlock_unlock(&lock);
	printf("Freed the lock sendCurrentUserNames.\n");

}

uint16_t lengthOfUsername(unsigned char userName[MAXSIZE]){
	uint16_t i;
	for(i = 0; i < MAXSIZE; i++){

		if(userName[i] == '\0')
			break;
	}

	return i + 1;
}
void closeSocket(user * u){
	printf("Starting to close socket\n");
	removeUser(u);
	close(u->fd);
	printf("closing socket\n");
	free(u);
	pthread_exit(NULL);
}

void getUsername(void* u){
	user* currentUser = (user *) u;
	
	unsigned char sizeOfUsername;

	recievedBytes(currentUser,(unsigned char*) &sizeOfUsername, 1);
	
	//int receivebytes_val  = recv(currentUser->fd, &sizeOfUsername, sizeof(sizeOfUsername), 0);
	// if(receivebytes_val<=0){
	// 	closeSocket(currentUser);
	// }
	
	uint16_t size = (uint16_t) sizeOfUsername;
	unsigned char buff [MAXSIZE];

	recievedBytes(currentUser, buff, sizeOfUsername);
	
	memcpy(currentUser->usernamestr, (void *)buff, sizeof(buff));
	
	currentUser->length= size;
	
	printf("about to add new user\n" );
	addUser(currentUser); 

}

void recievedBytes(user * currentUser, unsigned char* buff, uint16_t numBytes){

	int sock = currentUser->fd;

	int numberToRead = numBytes;
	unsigned char * spotInBuffer = buff;
	
	while(numberToRead > 0){
		
		int recievedBytes = recv(sock, spotInBuffer, numberToRead, 0);
		if(recievedBytes <= 0){
			perror("Socket close or server timeout");
			closeSocket(currentUser);			
		}

		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += recievedBytes;
		numberToRead -= recievedBytes;
	}
}

void chat(user * u){
	user * currentUser = u;

	uint16_t inlength;
	for(;;){
		unsigned char buffSize[2];
		recievedBytes(currentUser, buffSize, 2);
		
		uint16_t temp = buffSize[1] << 8 + buffSize[0];
		uint16_t size = ntohs(temp);

		unsigned char buffMessage[MAXSIZE];
		memset(buffMessage, '\0', MAXSIZE);
		
		recievedBytes(currentUser, buffMessage, size);
		
		unsigned char type = (unsigned char) 0x0;
		
		uint16_t outsize=size+(currentUser->length)+2;
		unsigned char outMessage[MAXSIZE];
		memset(outMessage, '\0', MAXSIZE);

	
		
		memcpy(outMessage,(void*) currentUser->usernamestr,currentUser->length);
		
		memset(&outMessage[currentUser->length], ':',1);
		memset(&outMessage[currentUser->length+1 ], ' ',1);

		
		memcpy(&outMessage[currentUser->length +2], (void *) buffMessage, size);

		outMessage[outsize] = '\0';

		sendMessage(outMessage,outsize,type);

		memset(buffMessage, '\0', MAXSIZE);
		memset(outMessage, '\0', MAXSIZE);

	}
}
 
void sendMessage(char* p, uint16_t size, unsigned char type){

	printf("Taking the lock sendMessage.\n");
	pthread_rwlock_rdlock(&lock);
	int i;
	for(i = 0; i < MAXSIZE; i++){
		user * currentUser  = listofusers[i];
		
		if(currentUser != NULL){
			// send(currentUser->fd, &type, sizeof(type), 0);
			uint16_t hostToNet=htons(size);
			// send(currentUser->fd,&hostToNet,sizeof(hostToNet),0);
			// send(currentUser->fd,p,size,0);
			
			sendBytes(currentUser,&type,1);
			sendBytes(currentUser,(unsigned char*)&hostToNet,2);
			sendBytes(currentUser,p,size);

		}
	}
	pthread_rwlock_unlock(&lock);
	printf("Freeing the lock sendMessage.\n");
}
void sendBytes(user * currentUser, unsigned char* buff, uint16_t numBytes){
	int sock =currentUser->fd;

	int numberToSend = numBytes;
	unsigned char * spotInBuffer = buff;
	
	while(numberToSend > 0){
		int sentBytes = send(sock, spotInBuffer, numberToSend, 0);
		if(sentBytes <= 0){
			perror("Socket close or server timeout");
			closeSocket(currentUser);			
		}
	
		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += sentBytes;
		numberToSend -= sentBytes;
	}
}
void sendJoin(user * currentUser){
	unsigned char type= (unsigned char) 0x01;
	sendMessage(currentUser->usernamestr,currentUser->length ,type);
}

void sendExit(user * currentUser){
	unsigned char type= (unsigned char) 0x02;	
	sendMessage(currentUser->usernamestr,currentUser->length ,type);
}