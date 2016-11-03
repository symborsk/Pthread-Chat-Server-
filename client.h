#ifndef CLIENT
#define CLIENT

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
#include <sys/time.h>
#include <signal.h>

#define  MaxUsernameLength 20
#define  MaxUsernames 20
#define  BufferSize 256

typedef struct users{
	int len;
	unsigned char username[MaxUsernameLength];
} user;

//Thread function for recieving
void * recieveHandler(void * unUsed);

//This is the chat that is run always when the chat room is started
void chat();

void InitializeSignalHandlers();

//Two generic sending and recieving files
void recievedBytes(int sock, unsigned char* buff, uint16_t numBytes);
void sendBytes(int sock, unsigned char* buff, uint16_t numBytes);

void handShake();
void sendUsername(unsigned char * username);
void printUsernames();

void readMultipleUsernames(uint16_t numberOfUsers);
void readAndAddUserName();
void readAndRemoveUserName();

//The prints for the user added 
void userAdded(char* username, int size);
void userRemoved(char* userName, int size);

void sendKeepAliveMessage();

//Generic helper that just lets us know how long a char * is
uint16_t lengthOfString(unsigned char* userName);

void shutdownClient(int signal);

//Globals

user* listUsers[MaxUsernameLength];

struct timeval timer;
struct sigaction priorSigHandler;
struct sigaction currentSigHandler;

int socketFD;

pthread_rwlock_t lock =  PTHREAD_RWLOCK_INITIALIZER;

#endif 