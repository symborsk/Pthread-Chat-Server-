#ifndef CLIENT
#define CLIENT

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


#define MAXSIZE 20
#define BUFFSIZE 255



pthread_rwlock_t lock=PTHREAD_RWLOCK_INITIALIZER;
typedef struct users{
	int fd;
	uint16_t length;
	unsigned char usernamestr [MAXSIZE];
} user;

void * handShake(void*  u);
int IsUserNameUnique(unsigned char *name ,int  size);
uint16_t lengthOfUsername(unsigned char userName[MAXSIZE]);
void sendCurrentUserNames(user * targetUser);
void getUsername(void* u);
void recievedBytes(user * currentUser, unsigned char* buff, uint16_t numBytes);
//void * recieveMessage(void* socket);
void removeUser(user* u);
void addUser(user* u);
void chat(user* u);
void sendChatMessage(char* p,  uint16_t size, user * sender);
void sendJoinMessage(char* p, uint16_t size);
void sendExitMessage(char* p, uint16_t size);
void sendBytes(user * currentUser, unsigned char* buff, uint16_t numBytes);
void sendJoin(user * currentUser);
void sendExit(user * currentUser);
void writeToLogJoin(user *u);
void writeToLogExit(user *u);
void exitAll();

struct sigaction priorSigHandler;
struct sigaction currentSigHandler;

uint16_t numberofclients;
int fd;
pid_t forkstatus;
pid_t sid;
user * listofusers[MAXSIZE];
FILE *fp; 

#endif