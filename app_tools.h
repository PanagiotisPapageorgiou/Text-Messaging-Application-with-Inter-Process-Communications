#include "shared_tools.h"

typedef struct AppThreadSharedData{
	char* processName;
	key_t shm_appEnckey;
	key_t sem_setupKey;
	key_t sem_appEnckey;
	int shmid_appEnc;
	int semid_appEnc;
	int semid_setup;
	int processNumber;
	int termFlag;
	SharedMemorySegment* appEncShmPtr;
	pthread_t threadIDs[2];
	pthread_mutex_t termFlagLock;
	pthread_mutex_t malloc_mtx;
	pthread_mutex_t free_mtx;
} AppThreadSharedData;

/*----Basic Setup functions for App Process----*/
int argumentHandling(int argc, char* argv[]);
int attachToAppEncSharedMemory();
int detachFromAppEncSharedMemory();
int getAppEncSemaphores();
int AppWaitsForSetupToFinish();
int AppSignalsToSetupFinish();
int appSetupOperations();
int appTearDownOperations();

/*----Thread functions for App Process----*/
void* InputWorker();
void* OutputWorker();
int message_input(char*);
