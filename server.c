#include "server.h"


void sigHandler(int sig){
	fprintf(fp, "-CLOSING- SIGTERM recieved\n");
	exitAll();
	sigaction(SIGTERM, &priorSigHandler, 0);
	fprintf(fp, "-SHUTTING DOWN- All users exited successfully\n");
	fprintf(fp, "Terminating...\n");
	fclose(fp);	
	close(sock);
	exit(1);
}

int main(int argc, char *argv[]){
	if(argc!=2){
		printf("Invalid arguments: this server requires one input arguement as the port number\n");
		exit(1);
	}
	uint16_t portnumber =atoi(argv[1]);
	
	sid = 0;
	char cwd[1024];
   	if (getcwd(cwd, sizeof(cwd)) == NULL)
   	{
   		perror("getcwd() error");
   	}
 

	//daemonizing
	int forkstatus=fork();
	if(forkstatus!=0){
		
		exit(1);
	}

	pid_t myPid=getpid();
	
	char filename[BUFFSIZE];
	char temp[20];
	sprintf(temp,"%d",myPid);
	strcpy(filename,"server379");
	strcat(filename,temp);
	strcat(filename,".log");
	
	
    if ((chdir(cwd)) < 0) {
      
      printf("Could not change working directory\n");
      exit(1);
    }

	umask(0);
 	
 	fp = fopen(filename, "w+");
 	if(fp == NULL){
 		printf("Failed to open a log file.\n");
 		exit(1);		
 	}

 	//create new process group -- don't want to look like an orphan
    sid = setsid();
    if(sid < 0)
    {
    	printf("cannot create new process group\n");
        exit(1);
    }

 	//Intialize sig handlers
	currentSigHandler.sa_handler = sigHandler;
	sigemptyset(&currentSigHandler.sa_mask);
	currentSigHandler.sa_flags = 0;
 	sigaction(SIGTERM, &currentSigHandler, &priorSigHandler);


	//close standard fds
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);	

	numberofclients=0;
	
	fd_set fd_active;
	fd_set read_fd_set;
	struct timeval timer;
	timer.tv_sec = 30;
	timer.tv_usec=0;
	forkstatus =0;

	int	snew, fromlength;
	struct	sockaddr_in	master, from;
	
	
	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		fprintf(fp, "-ERROR- Could not create socket: %s\n",strerror(errno));
		exit (1);
	}

	
	master.sin_family = AF_INET;
	master.sin_addr.s_addr = INADDR_ANY;
	master.sin_port = htons (portnumber);

	if (bind (sock, (struct sockaddr*) &master, sizeof (master))) {
		fprintf(fp, "-ERROR- Could not bind to socket: %s\n",strerror(errno));
		exit (1);
	}

	listen (sock, 5);
	FD_ZERO(&fd_active);
	FD_SET(sock,&fd_active);
	read_fd_set=fd_active;
	pthread_t thread;
	fprintf(fp, "-START- Initializing Chat Room\n");
	for(;;){


		if(select(FD_SETSIZE,&read_fd_set,NULL,NULL,NULL)<0){
			perror("select()");
			exit(1);
		}
		
		int i=0;
		for(i;i<FD_SETSIZE;i++){			
			if(FD_ISSET(i, &read_fd_set)){
				if(i == sock){
					fromlength = sizeof (from);
					
					snew = accept (sock, (struct sockaddr*) & from, & fromlength);

					if(setsockopt(snew,SOL_SOCKET,SO_RCVTIMEO, (char *)&timer, sizeof(timer))<0){
						fprintf(fp, "-ERROR-Time out cannot be set for socket: %s\n",strerror(errno) );
					}
					if (snew < 0) 
					{
						fprintf(fp, "-ERROR-Socket could not be created for client: %s\n",strerror(errno) );
						continue;
					}
					
					user* newUser = (user *)malloc(sizeof(user));
					memset(newUser->usernamestr,'\0',MAXSIZE);
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

void addUser(user* u){
	

	userLink* link = malloc(sizeof(userLink));
	link->user = u;
	link->next = NULL;

	pthread_rwlock_wrlock(&lock);
	numberofclients++;
	
	if(firstLink ==NULL){
		firstLink = link;
	}

	else{
		
		userLink * currentLink = firstLink;
		while(currentLink->next != NULL){
			currentLink =currentLink->next;
		}

		currentLink->next = link;
	}

	pthread_rwlock_unlock(&lock);
}

void removeUser(user * u){
	
	
	pthread_rwlock_wrlock(&lock);
	
	userLink* priorLink = firstLink;
	userLink* currentLink = firstLink;
	for(;;){
		if(currentLink == NULL){
			fprintf(fp, "-ERROR- Unable to find FD to remove\n" );
			break;
		}
		
		//look for the fd of the same user and remove it
		if(currentLink->user->fd == u->fd){
			userLink* removedLink= currentLink;
			
			if(priorLink != currentLink){
				priorLink->next= currentLink->next;
			}
			// removing the first user, need new first pointer
			else{
				firstLink= firstLink->next;
			}

			free(removedLink);
			numberofclients --;
			break;
		}

		priorLink = currentLink;
		currentLink = currentLink->next;
	}

	pthread_rwlock_unlock(&lock);
}

void * handShake(void* u){
 	user* currentUser = (user *) u;
 	uint16_t outnum=0;

 	unsigned char outarray [2];
	uint16_t holdingNumber = 0xcf;
	outarray[0] = (unsigned char) (holdingNumber);
	holdingNumber = 0xa7;
	outarray[1] = (unsigned char) (holdingNumber);
	sendBytes(currentUser,outarray,2);

	uint16_t hostToNet=htons(numberofclients);
	sendBytes(currentUser,(unsigned char*)&hostToNet,2);
	
	sendCurrentUserNames(currentUser);

	getUsername(currentUser);

	addUser(currentUser); 

	sendJoin(currentUser);

	chat(currentUser);
}	

void sendCurrentUserNames(user * targetUser){
	
	pthread_rwlock_rdlock(&lock);

	userLink* currentLink = firstLink;
	user* currentUser;
	while(currentLink != NULL){
		currentUser = currentLink->user;

		unsigned char outLength = (unsigned char)currentUser->length;

		sendBytes(targetUser, &outLength, 1);
		sendBytes(targetUser, currentUser->usernamestr, currentUser->length); 

		currentLink = currentLink->next;
	}

	pthread_rwlock_unlock(&lock);
}

void closeSocket(user* u){
	
	removeUser(u);
	sendExit(u);
	close(u->fd);
	free(u);
	pthread_exit(NULL);
}

void getUsername(void* u){
	user* currentUser = (user *) u;
	
	unsigned char sizeOfUsername;
	recievedBytes(currentUser,(unsigned char*) &sizeOfUsername, 1);
	
	uint16_t size = (uint16_t) sizeOfUsername;
	unsigned char buff [MAXSIZE];

	recievedBytes(currentUser, buff, sizeOfUsername);

	//Tell us if it unique
	if(! IsUserNameUnique(buff, size) ){
		close(currentUser->fd);
		free(currentUser);
		pthread_exit(NULL);
	}
	
	memcpy(currentUser->usernamestr, (void *)buff, size);
	
	currentUser->length= size;
}

void chat(user * u){
	user * currentUser = u;

	uint16_t inlength;
	for(;;){
		unsigned char buffSize[2];
		recievedBytes(currentUser, buffSize, 2);
		
		uint16_t temp = buffSize[1] << 8 + buffSize[0];
		uint16_t size = ntohs(temp);

		unsigned char buffMessage[BUFFSIZE];
		memset(buffMessage, '\0', BUFFSIZE);
		
		if(size==0){
			continue;
		}
		recievedBytes(currentUser, buffMessage, size);

		sendChatMessage(buffMessage,size,u);

		memset(buffMessage, '\0', BUFFSIZE);
	}
}
 
void sendChatMessage(char* p,  uint16_t size, user * sender){

	pthread_rwlock_rdlock(&lock);

	userLink * currentLink =firstLink;
	user * currentUser;
	while(currentLink!=NULL){
		currentUser= currentLink->user;
		unsigned char type = 0x0;
		uint16_t hostToNet=htons(size);

		sendBytes(currentUser,&type,1);
		sendBytes(currentUser,(unsigned char *)&(sender->length),1);
		sendBytes(currentUser,sender->usernamestr,sender->length);
		sendBytes(currentUser,(unsigned char*)&hostToNet,2);
		sendBytes(currentUser,p,size);

		currentLink = currentLink->next;
	}

	pthread_rwlock_unlock(&lock);
}

void sendJoinMessage(char* p, uint16_t size){

	

	pthread_rwlock_rdlock(&lock);

	userLink * currentLink = firstLink;
	user* currentUser;

	while(currentLink != NULL){
		currentUser = currentLink->user;
		printf("%s\n", currentUser->usernamestr);
		
		unsigned char type = 0x1;
		unsigned char charSize = (unsigned char) size;
		
		
		sendBytes(currentUser, &type,1);		
		sendBytes(currentUser, &charSize, 1);		
		sendBytes(currentUser, p, size);
		currentLink = currentLink->next;
	}
	
	pthread_rwlock_unlock(&lock);
}

void sendExitMessage(char* p, uint16_t size){
	pthread_rwlock_rdlock(&lock);
	

	userLink * currentLink = firstLink;
	user* currentUser;
	while(currentLink!=NULL){
		currentUser=currentLink->user;
		unsigned char type = 0x2;
		unsigned char charSize = (unsigned char) size;
		
		sendBytes(currentUser,&type,1);
		sendBytes(currentUser, &charSize, 1);
		sendBytes(currentUser, p ,size);
		currentLink=currentLink->next;
	}
		
	pthread_rwlock_unlock(&lock);
	
}

void sendBytes(user * currentUser, unsigned char* buff, uint16_t numBytes){
	int sock =currentUser->fd;

	int numberToSend = numBytes;
	unsigned char * spotInBuffer = buff;
	
	while(numberToSend > 0){
		int sentBytes = send(sock, spotInBuffer, numberToSend, 0);
		if(sentBytes <= 0){
			
			if(sentBytes == -1)
				fprintf(fp, "-ERROR-Failed in sending: %s\n",strerror(errno));
			
			closeSocket(currentUser);			
		}
	
		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += sentBytes;
		numberToSend -= sentBytes;
	}
}

void recievedBytes(user* currentUser, unsigned char* buff, uint16_t numBytes){

	int sock = currentUser->fd;

	int numberToRead = numBytes;
	unsigned char * spotInBuffer = buff;
	
	while(numberToRead > 0){
		
		int recievedBytes = recv(sock, spotInBuffer, numberToRead, 0);
		if(recievedBytes <= 0){
			
			if(recievedBytes == -1)
				fprintf(fp, "-ERROR-Failed in receiving: %s\n",strerror(errno));

			closeSocket(currentUser);			
		}

		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += recievedBytes;
		numberToRead -= recievedBytes;
	}
}

void sendJoin(user * currentUser){
	
	sendJoinMessage(currentUser->usernamestr,currentUser->length);
	writeToLogJoin(currentUser);
}

void sendExit(user * currentUser){

	sendExitMessage( currentUser->usernamestr, currentUser->length);
	writeToLogExit(currentUser);
}

void exitAll(){
	int i;
	pthread_rwlock_wrlock(&lock);
	userLink* currentLink=firstLink;
	userLink* nextLink;
	
	while(currentLink != NULL){
		nextLink = currentLink->next;

		writeToLogExit(currentLink->user);
		close(currentLink->user->fd);

		
		free(currentLink->user);
		free(currentLink);

		currentLink = nextLink;
	}	
	pthread_rwlock_unlock(&lock);	
}

void writeToLogJoin(user *u){
	
	char line[BUFFSIZE];
	memset(line,'\0',BUFFSIZE);
	strncpy(line,u->usernamestr,u->length);
	strcat(line," joined");
	fprintf(fp, "%s\n",line );
}

void writeToLogExit(user *u){
	char line[BUFFSIZE];
	memset(line,'\0',BUFFSIZE);
	strncpy(line,u->usernamestr,u->length);
	strcat(line," left");
	fprintf(fp, "%s\n",line );
}

int IsUserNameUnique(unsigned char *name ,int  size){
	printf("Acquiring lock unique username\n");
	pthread_rwlock_rdlock(&lock);
	
	int ret = 1;

	userLink* currentLink =firstLink;
	user* currentUser;
	while(currentLink!=NULL){
		currentUser=currentLink->user;
		
		if(strncmp(currentUser->usernamestr, name, ( size > currentUser->length)? size:currentUser->length) == 0){
			fprintf(fp, "-ERROR- New user tried to use existing username %s\n", name);
			ret = 0;
			break;
		}

		currentLink=currentLink->next;
	}

	pthread_rwlock_unlock(&lock);

	return ret;
}