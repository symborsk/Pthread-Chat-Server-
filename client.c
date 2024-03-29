
#include "client.h"

int main(int argc, char *argv[])
{
    if(argc != 4){
    	printf("This program requires IP Address and Port number and username as 3 input arguments. Exiting...\n");
    	exit(1);
    }
    struct	sockaddr_in	server;
	struct	hostent		*host;

    host = gethostbyname(argv[1]);
    uint16_t port = atoi(argv[2]);
	
	InitializeSignalHandlers();
	timer.tv_sec = 5;
	timer.tv_usec = 0;

	if (host == NULL) {
		perror ("Client: cannot get host description");
		shutdownClient(1);
	}

	socketFD = socket(AF_INET, SOCK_STREAM, 0);

	if (socketFD < 0) {
		perror ("Client: cannot open socket");
		shutdownClient(1);
	}

	bzero (&server, sizeof (server));
	bcopy (host->h_addr, & (server.sin_addr), host->h_length);
	server.sin_family = host->h_addrtype;
	server.sin_port = htons(port);

	if (connect (socketFD, (struct sockaddr*) & server, sizeof (server))) {
		perror ("Client: cannot connect to server");
		shutdownClient(1);
	}
	
	//Handshake includeds readinf of specific usernames as well as confirmation code
	handShake();

	//After the confirmation start a seperate thread for listening
	pthread_t thread;
	int ret = pthread_create(&thread, NULL, &recieveHandler, 0);
	if(ret){
		printf("Error Creating Thread %d", ret);
		shutdownClient(1);
	}

	sendUsername(argv[3]);
	chat();	
}

//Signal handler for ctlr C
void InitializeSignalHandlers(){

	currentSigHandler.sa_handler = shutdownClient;
	sigemptyset(&currentSigHandler.sa_mask);
	currentSigHandler.sa_flags = 0;
	sigaction(SIGINT, &currentSigHandler, &priorSigHandler);
}

//Clean up memory and close socket
void shutdownClient(int signal){

	//free all the malloced users close the socket
	userLink * currentLink = firstLink;
	userLink * nextLink;

	for(;;){
		if(currentLink == NULL){
			break;
		}

		nextLink = currentLink->next;

		free(currentLink->user);
		free(currentLink);

		currentLink = nextLink;
	}

	printf("Exiting chatroom. Goodbye\n");

	//Leave it in the state we got it
	sigaction(SIGINT, &priorSigHandler, 0);
	close(socketFD);
	exit(1);
}

//This sepereate threaded function is always listening to ensure the client can always recieve messages
void * recieveHandler(void * unUsed){
	
	//We passed in a void * that is an int * so cast it and dereference it
	uint16_t code;
	unsigned char buffUsername[BufferSize];
	unsigned char buffMessage[BufferSize];
	memset(buffUsername, '\0', BufferSize);
	memset(buffMessage, '\0', BufferSize);
	while(1){
		
		recievedBytes(socketFD, buffMessage, 1);
		code = (uint16_t)buffMessage[0];
		
		//clear the buffer after we use it
		memset(buffMessage, '\0', BufferSize);

		//User message
		if(code == 0x00){
				
			//Recieving user message
			unsigned char stringSizeChar;
			recievedBytes(socketFD, &stringSizeChar, 1);

			//Casting to an int to tell us how many expected bytes
			uint16_t stringSizeInt = (uint16_t)stringSizeChar;
			recievedBytes(socketFD, buffUsername, stringSizeInt);

			recievedBytes(socketFD, buffMessage, 2);
			//This simply just allows us to cast two bytes to 2 byte ints
			uint16_t networkSize = buffMessage[1] << 8 | buffMessage[0];
			uint16_t sizeMessage = ntohs(networkSize);
		
			//Clear our buffer before using it again for the message
			memset(buffMessage, '\0', BufferSize);

			recievedBytes(socketFD, buffMessage, sizeMessage);
			
			//This specific series of print statments just make a nice readable username: message string
			printf("%s : %s\n", buffUsername, buffMessage);
			
			memset(buffMessage, '\0', BufferSize);
			memset(buffUsername, '\0', BufferSize);
		}

		//Add user message
		else if(code == 0x01){
			
			readAndAddUserName();
		}

		//Remove user message
		else if(code == 0x02){
			
			readAndRemoveUserName();
		}

		//Something weird happened..... shouldnt happen unless server and client disconnect
		else{
			printf("Server sent uknown code: %u\n", code);
			shutdownClient(0);
		}
	}
}

//Handshake procedure for our server recieving the proper authentication
void handShake(){
	unsigned char confirmation_one_byte;
	unsigned char confirmation_two_byte;
	
	//Make sure there is a timeout so the client doesnt wait for handshake forever
	if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0){
	 	perror("setsockopt failed\n");
	 	exit(1);
	}
	
	recievedBytes(socketFD, &confirmation_one_byte, sizeof(confirmation_one_byte));
	recievedBytes(socketFD, &confirmation_two_byte, sizeof(confirmation_two_byte));
	
	uint16_t first_confirmation = (uint16_t)confirmation_one_byte;
	uint16_t second_confirmation = (uint16_t)confirmation_two_byte;
	
	if( first_confirmation != 0xCF || second_confirmation != 0xA7){
		perror("Client: Did not recieve correct server authentication");
		exit(1);
	}

	uint16_t numberOfUsersNetwork;
	recievedBytes(socketFD, (unsigned char *)&numberOfUsersNetwork, sizeof(numberOfUsersNetwork));

	
	uint16_t intUsers  = ntohs(numberOfUsersNetwork);
	
	readMultipleUsernames(intUsers);

	//After the authentication is complete there is not timeout on the receive end of the user
	timer.tv_sec = 0;
	if (setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timer, sizeof(timer)) < 0){	
	 	perror("setsockopt failed\n");
	 	exit(1);
	}
}

//Read and add multiple usernames
void readMultipleUsernames(uint16_t numberOfUsers){
	
	int i;
	for( i = 0;  i < numberOfUsers ; i++ ){
		readAndAddUserName();
	}
}

//Read in and add a username to our linked list
void readAndAddUserName(){
		
	unsigned char buff[BufferSize];
	memset(buff, '\0', BufferSize);
	
	recievedBytes(socketFD, buff, 1);

	uint16_t size = (uint16_t)buff[0];
	memset(buff, '\0', BufferSize);

	recievedBytes(socketFD, buff, size);
	
	userAdded(buff, size);
}

//Read in and remove a username from link list
void readAndRemoveUserName(){

	unsigned char buff[BufferSize];
	memset(buff, '\0', BufferSize);
	
	recievedBytes(socketFD, buff, 1);

	uint16_t size = (uint16_t)buff[0];
	memset(buff, '\0', BufferSize);

	recievedBytes(socketFD, buff, size);
	
	userRemoved(buff, size);
}

//Generic method that returns the length of a message based on null a new line terminates
uint16_t lengthOfString(unsigned char* userName){
	
	int i;
	for(i = 0; i < BufferSize; i++){
		
		if(userName[i] == '\0' || userName[i] == '\n' )
			break;
	}

	return i;
}

//Send the username passed into the function to server
void sendUsername(unsigned char* username){
	//Put in the length at the start
	uint16_t sizeUser = lengthOfString(username);
	unsigned char user = (unsigned char) sizeUser;
	
	sendBytes(socketFD, &user, 1);
	sendBytes(socketFD, username, sizeUser);
}

//Generic recieve that has safeguard if the message is sent in pieces
void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes){

	int numberToRead = numBytes;
	unsigned char * spotInBuffer = buff;
	while(numberToRead > 0){
		
		int readBytes = recv(sock, spotInBuffer, numberToRead, 0);
		if(readBytes <= 0){
			printf("Server has hung up shutting down...\n");
			shutdownClient(0);			
		}

		// Increment the char pointer to current sunfilled spot 
		spotInBuffer += readBytes;
		numberToRead -= readBytes;
	}
}

//Generic send so if send can send in pieces if unsuccesful
void sendBytes(int sock, unsigned char* buff, uint16_t numBytes){

	int numberToWrite = numBytes;
	unsigned char* spotInBuffer = buff;

 	while(numberToWrite > 0){
		int sentBytes = send(sock, spotInBuffer, numberToWrite, 0);

		if(sentBytes <= 0){
			printf("Server has hung up shutting down...\n");
			shutdownClient(0);			
		}

		// Increment the char pointer to current spot  we sent to
		spotInBuffer += sentBytes;
		numberToWrite -= sentBytes;
	}
}

// Loop for chatting, mainly cover client interface functionality
void chat(){

	for(;;){

		struct timeval tv;
    	fd_set fds;

    	tv.tv_sec = 20;
    	tv.tv_usec = 0;

   		FD_ZERO(&fds);
   		
    	FD_SET(STDIN_FILENO, &fds);

		if(select(STDIN_FILENO+1, &fds, NULL, NULL, &tv) <= 0){
			
			sendKeepAliveMessage();
			continue;
		}

		unsigned char buff[BufferSize];
		fgets(buff, BufferSize, stdin);

		if(strncmp(buff, ".usernames", 10) == 0){
			sendKeepAliveMessage();
			printUsernames();
			continue;
		}
		
		uint16_t size = lengthOfString(buff);
		uint16_t networkSize = htons(size);

		sendBytes(socketFD, (unsigned char *)&networkSize, 2);
		sendBytes(socketFD, buff, size);

		//clear the buffer for next time
		memset(buff, '\0', BufferSize);
	}
}

// Create and add a user of specific username and size and add it to link list
void userAdded(char* username, int size){
	
	user * User = (user *)malloc(sizeof(user));
	User->len = size;
	memcpy(&(User->username), username, size); 
	User->username[size] = '\0';

	userLink* link = malloc(sizeof(userLink));
	link->user = User;
	link->next = NULL;

	pthread_rwlock_wrlock(&lock);

	if(firstLink == NULL){
		
		firstLink = link;	
	}

	else{
		
		userLink* currentLink = firstLink;
		while(currentLink->next != NULL){
			currentLink = currentLink->next;
		}
		currentLink->next = link;
	}

	printf(" %s has been added to chat room\n", link->user->username);

	pthread_rwlock_unlock(&lock);
}

//Remove a username with a specific username from link list
void userRemoved(char* userName, int size){

	pthread_rwlock_wrlock(&lock);

	userLink* priorLink = firstLink;
	userLink* currentLink = firstLink;
	for(;;){
		
		if(currentLink == NULL){
			printf("Unable to find username to remove\n");
			break;
		}

		//Look for the username of the same name and remove it
		if(strncmp(currentLink->user->username, userName, size) == 0){
			userLink* removedLink = currentLink;
			printf(" %s has been removed from the chat room\n", removedLink->user->username);
			
			if(priorLink != currentLink){
				
				priorLink->next = currentLink->next;

			}
			//Removing the first user we need a new head
			else{
				
				firstLink = currentLink->next;
			}

			free(removedLink->user);
			free(removedLink);
			break;
		}

		priorLink = currentLink;
		currentLink = currentLink->next;
	}

	pthread_rwlock_unlock(&lock);
}

//Print out all the usernames in link list to user
void printUsernames(){
	
	pthread_rwlock_rdlock(&lock);
	
	userLink * currentLink = firstLink;
	while(currentLink != NULL){
	
		printf("%s\n", currentLink->user->username);
		currentLink = currentLink->next;
	}
	pthread_rwlock_unlock(&lock);
}

//Send an message of length 0 to prevent timeout
void sendKeepAliveMessage(){

	uint16_t zeroCode = 0;
	uint16_t networkZeroCode = htons(zeroCode);

	sendBytes(socketFD, (unsigned char *)&networkZeroCode, 2);
}





