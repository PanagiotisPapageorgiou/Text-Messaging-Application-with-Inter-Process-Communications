#include "shared_tools.h"

typedef struct ChanThreadSharedData{
	char* processName;
	key_t shm_enc1Chankey;
	key_t shm_enc2Chankey;
	key_t sem_setupKey;
	key_t sem_enc1Chankey;
	key_t sem_enc2Chankey;
	int shmid_enc1Chan;
	int shmid_enc2Chan;
	int semid_enc1Chan;
	int semid_enc2Chan;
	int semid_setup;
	int termFlag;
	double corruptionPossibility;
	SharedMemorySegment* enc1ChanShmPtr;
	SharedMemorySegment* enc2ChanShmPtr;
	pthread_t threadIDs[2];
	pthread_mutex_t termFlagLock;
	pthread_mutex_t malloc_mtx;
	pthread_mutex_t free_mtx;
} ChanThreadSharedData;

/*----Basic Setup functions for Channel Process----*/
int argumentHandling(int argc, char* argv[]);
int attachToEncChanSharedMemory(key_t, int* , SharedMemorySegment**);
int detachFromEncChanSharedMemory(SharedMemorySegment*);
int getEncChanSemaphores(key_t, int*);
int getSetupSemaphores();
int ChanWaitsForSetupToFinish();
int ChanSignalsToSetupFinish();
int chanSetupOperations();
int chanTearDownOperations();

/*----Thread functions for Channel Process----*/
void* serveEncoderThread(void*);
int corruptMessage(MessagePacket*);
