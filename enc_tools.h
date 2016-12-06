#include "shared_tools.h"

typedef struct EncThreadSharedData{
	char* processName;
	key_t shm_appEnckey;
	key_t shm_encChankey;
	key_t sem_setupKey;
	key_t sem_appEnckey;
	key_t sem_encChankey;
	int shmid_appEnc;
	int shmid_encChan;
	int semid_appEnc;
	int semid_encChan;
	int semid_setup;
	int processNumber;
	int termFlag;
	int retryFlag;
	SharedMemorySegment* appEncShmPtr;
	SharedMemorySegment* encChanShmPtr;
	pthread_t threadIDs[2];
	pthread_mutex_t termFlagLock;
	pthread_mutex_t retryFlagLock;
	pthread_mutex_t malloc_mtx;
	pthread_mutex_t free_mtx;
} EncThreadSharedData;

/*----Basic Setup functions for Encoder Process----*/
int argumentHandling(int argc, char* argv[]);
int attachToAppEncSharedMemory();
int detachFromAppEncSharedMemory();
int getAppEncSemaphores();
int attachToEncChanSharedMemory();
int detachFromEncChanSharedMemory();
int getEncChanSemaphores();
int EncWaitsForSetupToFinish();
int EncSignalsToSetupFinish();
int encSetupOperations();
int encTearDownOperations();

/*----Thread functions for Encoder Process----*/
void* TalkerThread();
void* ListenerThread();
int checkMessageIntegrity(MessagePacket);
int addHashValueToMessage(MessagePacket*);
int confirmationForTalker(MessagePacket*);
int waitForConfirmation();
