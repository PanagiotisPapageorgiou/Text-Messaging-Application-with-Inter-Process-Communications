#include "shared_structs.h"
#include <pthread.h>

/*-----Shared Memory----*/
int attachToSharedMemoryArea(key_t, int*, SharedMemorySegment**);
int detachSharedMemory(SharedMemorySegment*);

/*----Semaphores----*/
int getSemaphores(key_t, int*, int);
int getSetupSemaphores(char*, key_t, int*);

/*----Communicating with Setup----*/
int waitOrSignalSetup(char*,int,int,int,int);
int waitForSetupToFinish(char*,int,int,int);

/*----Producer and Consumer Shared Buffer functions----*/
int addToSharedBuffer(char*, MessagePacket*, MessageQueueArray*,int, int, int, int, int*, pthread_mutex_t*, pthread_mutex_t*);
MessagePacket* takeFromSharedBuffer(char* , MessageQueueArray*, int, int, int, int, int*,  pthread_mutex_t*, pthread_mutex_t*);

/*----Termination functions----*/
int sigFinishToOther(char*,pthread_mutex_t*,int*,int,int);
