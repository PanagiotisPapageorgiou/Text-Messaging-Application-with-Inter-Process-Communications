#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include "setup_tools.h"

int main(int argc,char* argv[]){

	int i=0,errorCode;
	int shmid_app1Enc1=UNINITIALISED, shmid_app2Enc2=UNINITIALISED, shmid_enc1Channel=UNINITIALISED, shmid_enc2Channel=UNINITIALISED;
	int semid_app1Enc1=UNINITIALISED, semid_app2Enc2=UNINITIALISED, semid_enc1Channel=UNINITIALISED, semid_enc2Channel=UNINITIALISED, semid_setup = UNINITIALISED;

	printf("Setup: Hello! I will setup all Semaphores and Shared Memory Segments required!\n");

	createAndInitialiseSemaphores(&semid_setup,&semid_app1Enc1,&semid_app2Enc2,&semid_enc1Channel,&semid_enc2Channel);

	createAndInitialiseSharedMemorySegments(semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Channel,semid_enc2Channel,&shmid_app1Enc1,&shmid_app2Enc2,&shmid_enc1Channel,&shmid_enc2Channel);

	signalAndWaitProcesses(semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Channel,semid_enc2Channel,shmid_app1Enc1,shmid_app2Enc2,shmid_enc1Channel,shmid_enc2Channel);

	ShmAndSemDestruction(shmid_app1Enc1,shmid_app2Enc2,shmid_enc1Channel,shmid_enc2Channel,semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Channel,semid_enc2Channel);

	printf("\nSetup: Bye-bye\n");

	return 0;

}
