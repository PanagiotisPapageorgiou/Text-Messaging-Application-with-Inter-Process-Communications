#include "shared_tools.h"
#define SHMKEY_APP1_ENC1 (key_t) 34410
#define SHMKEY_APP2_ENC2 (key_t) 34411
#define SHMKEY_ENC1_CHANNEL (key_t) 34412
#define SHMKEY_ENC2_CHANNEL (key_t) 34413
#define SEMKEY_APP1_ENC1 (key_t) 54410
#define SEMKEY_APP2_ENC2 (key_t) 54411
#define SEMKEY_ENC1_CHANNEL (key_t) 54412
#define SEMKEY_ENC2_CHANNEL (key_t) 54413

/*----Shared memory functions----*/
int shared_memory_destruction(int,int,int,int);
int shared_memory_creation(int*,int*,int*,int*);
int initialiseSharedMemorySegment(int);
int shared_memory_initialisation(int,int,int,int);
int createAndInitialiseSharedMemorySegments(int semid_setup,int semid_app1Enc1,int semid_app2Enc2,int semid_enc1Chan,int semid_enc2Chan,int* shmid_app1Enc1,int* shmid_app2Enc2,int* shmid_enc1Chan,int* shmid_enc2Chan);

/*----Semaphore functions----*/
int semaphores_destruction(int,int,int,int,int);
int semaphores_creation(int*,int*,int*,int*,int*);
int semaphores_initialisation(int,int,int,int,int);
int createAndInitialiseSemaphores(int*,int*,int*,int*,int*);

/*----Signaling and Waiting Processes----*/
int signalProcessesToBegin(int);
int waitForProcessesToFinish(int);
int signalAndWaitProcesses(int,int,int,int,int,int,int,int,int);

/*----Free resources----*/
int ShmAndSemDestruction(int,int,int,int,int,int,int,int,int);
