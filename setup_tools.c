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

/*=========================================SHARED MEMORY FUNCTIONS====================================*/

int shared_memory_destruction(int shmid_app1Enc1,int shmid_app2Enc2,int shmid_enc1Channel,int shmid_enc2Channel){ //Destroys any created shared memory segments

	if((shmid_app1Enc1 != UNINITIALISED) && (shmid_app1Enc1 != -1)){ //or simply if a shmid is positive/initialised
		printf("Setup: Now Destroying App1-Enc1 Shared Memory Segment...\n");
		if(shmctl(shmid_app1Enc1, IPC_RMID, NULL) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));		
			return -1;
		}		
	}

	if((shmid_app2Enc2 != UNINITIALISED) && (shmid_app2Enc2 != -1)){
		printf("Setup: Now Destroying App2-Enc2 Shared Memory Segment...\n");
		if(shmctl(shmid_app2Enc2, IPC_RMID, NULL) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));	
			return -1;	
		}			
	}

	if((shmid_enc1Channel != UNINITIALISED) && (shmid_enc1Channel != -1)){
		printf("Setup: Now Destroying Enc1-Channel Shared Memory Segment...\n");
		if(shmctl(shmid_enc1Channel, IPC_RMID, NULL) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));	
			return -1;	
		}				
	}

	if((shmid_enc2Channel != UNINITIALISED) && (shmid_enc2Channel != -1)){
		printf("Setup: Now Destroying Enc2-Channel Shared Memory Segment...\n");
		if(shmctl(shmid_enc2Channel, IPC_RMID, NULL) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));	
			return -1;	
		}	
	}

	return 0;
}

int shared_memory_creation(int* shmid_app1Enc1ptr,int* shmid_app2Enc2ptr,int* shmid_enc1Channelptr,int* shmid_enc2Channelptr){ //Creates all required shared memory segments

	/*----Creating Memory Segment App1-Enc1----*/
	printf("Setup: Creating App1-Enc1 Shared Memory Segment... - APP1_ENC1_SHARED_MEMORY_KEY:%d\n", SHMKEY_APP1_ENC1);
	*shmid_app1Enc1ptr = shmget(SHMKEY_APP1_ENC1, sizeof(SharedMemorySegment), PERMS | IPC_CREAT);
	if(*shmid_app1Enc1ptr == -1){
		printf("Setup: Shmget failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	/*----Creating Memory Segment App2-Enc2----*/
	printf("Setup: Creating App2-Enc2 Shared Memory Segment... - APP2_ENC2_SHARED_MEMORY_KEY:%d\n", SHMKEY_APP2_ENC2);
	*shmid_app2Enc2ptr = shmget(SHMKEY_APP2_ENC2, sizeof(SharedMemorySegment), PERMS | IPC_CREAT);
	if(*shmid_app2Enc2ptr == -1){
		printf("Setup: Shmget failed! Error Message: |%s|\n", strerror(errno));
		return shared_memory_destruction(*shmid_app1Enc1ptr,*shmid_app2Enc2ptr,*shmid_enc1Channelptr,*shmid_enc2Channelptr);
	}

	/*----Creating Memory Segment Enc1-Channel----*/
	printf("Setup: Creating Enc1-Channel Shared Memory Segment... - ENC1_CHANNEL_SHARED_MEMORY_KEY:%d\n", SHMKEY_ENC1_CHANNEL);
	*shmid_enc1Channelptr = shmget(SHMKEY_ENC1_CHANNEL, sizeof(SharedMemorySegment), PERMS | IPC_CREAT);
	if(*shmid_enc1Channelptr == -1){
		printf("Setup: Shmget failed! Error Message: |%s|\n", strerror(errno));
		return shared_memory_destruction(*shmid_app1Enc1ptr,*shmid_app2Enc2ptr,*shmid_enc1Channelptr,*shmid_enc2Channelptr);
	}

	/*----Creating Memory Segment Enc2-Channel----*/
	printf("Setup: Creating Enc2-Channel Shared Memory Segment... - ENC2_CHANNEL_SHARED_MEMORY_KEY:%d\n", SHMKEY_ENC2_CHANNEL);
	*shmid_enc2Channelptr = shmget(SHMKEY_ENC2_CHANNEL, sizeof(SharedMemorySegment), PERMS | IPC_CREAT);
	if(*shmid_enc2Channelptr == -1){
		printf("Setup: Shmget failed! Error Message: |%s|\n", strerror(errno));
		return shared_memory_destruction(*shmid_app1Enc1ptr,*shmid_app2Enc2ptr,*shmid_enc1Channelptr,*shmid_enc2Channelptr);
	}

	return 0;
}

int initialiseSharedMemorySegment(int shmid){ //Attach to specific Shared Memory Segment and Initialise its buffers

	void* tmp_shm_ptr = (void*) NULL;
	SharedMemorySegment* shmPtr = (SharedMemorySegment*) NULL;

	tmp_shm_ptr = shmat(shmid, (void*) 0, 0);
	if(tmp_shm_ptr == (char*) -1){
		printf("Shmat failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	shmPtr = (SharedMemorySegment*) tmp_shm_ptr;
	if(init_arrayQueue(&(shmPtr->inputQueue)) == -1){
		printf("Setup: Failed to initialise inputQueue!\n");
		return -1;
	}
	if(init_arrayQueue(&(shmPtr->outputQueue)) == -1){
		printf("Setup: Failed to initialise outputQueue!\n");
		return -1;
	}

	if(detachSharedMemory(shmPtr) == -1){
		printf("Setup: Failed to detach from Shared Memory after Initialisation!\n");
		return -1;
	}

	return 0;

}

int shared_memory_initialisation(int shmid_app1Enc1,int shmid_app2Enc2,int shmid_enc1Channel,int shmid_enc2Channel){ //Initialise all buffers of all Shared Memories

	printf("Setup: Initialising App1-Enc1 Shared Memory Segment...\n");
	if(initialiseSharedMemorySegment(shmid_app1Enc1) == -1){
		printf("Setup: Failed to initialiase App1-Enc1 Shared Memory Segment!\n");
		return -1;
	}

	printf("Setup: Initialising App2-Enc2 Shared Memory Segment...\n");
	if(initialiseSharedMemorySegment(shmid_app2Enc2) == -1){
		printf("Setup: Failed to initialiase App2-Enc2 Shared Memory Segment!\n");
		return -1;
	}

	printf("Setup: Initialising Enc1-Chan Shared Memory Segment...\n");
	if(initialiseSharedMemorySegment(shmid_enc1Channel) == -1){
		printf("Setup: Failed to initialiase Enc1-Chan Shared Memory Segment!\n");
		return -1;
	}

	printf("Setup: Initialising Enc2-Chan Shared Memory Segment...\n");
	if(initialiseSharedMemorySegment(shmid_enc2Channel) == -1){
		printf("Setup: Failed to initialiase Enc2-Chan Shared Memory Segment!\n");
		return -1;
	}
	
	return 0;

}

int createAndInitialiseSharedMemorySegments(int semid_setup,int semid_app1Enc1,int semid_app2Enc2,int semid_enc1Chan,int semid_enc2Chan,int* shmid_app1Enc1,int* shmid_app2Enc2,int* shmid_enc1Chan,int* shmid_enc2Chan){ //Create, Attach and Initialise all Shared Memory Segments

	/*----Shared Memory Segments Creation----*/
	printf("\nSetup: Creating all Shared Memory Segments...\n");
	if(shared_memory_creation(shmid_app1Enc1,shmid_app2Enc2,shmid_enc1Chan,shmid_enc2Chan) == -1){
		printf("Setup: Failed at shared memory creation!\n");
		printf("Setup: Now Destroying Semaphores...\n");
		if(semaphores_destruction(semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Chan,semid_enc2Chan) == -1){
			printf("Setup: Failed at Semaphores Destruction!\n");
		}
		printf("Exiting...");	
		exit(1);
	}

	if(shared_memory_initialisation(*shmid_app1Enc1, *shmid_app2Enc2, *shmid_enc1Chan, *shmid_enc2Chan) == -1){
		printf("Setup: Failed at shared memory initialisation!\n");
		ShmAndSemDestruction(*shmid_app1Enc1,*shmid_app2Enc2,*shmid_enc1Chan,*shmid_enc2Chan,semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Chan,semid_enc2Chan);
		exit(1);
	}

	return 0;

}

/*=======================================SEMAPHORE FUNCTIONS==========================================*/

int semaphores_destruction(int semid_setup,int semid_app1Enc1,int semid_app2Enc2,int semid_enc1Channel,int semid_enc2Channel){

	int semnum;

	if((semid_setup != UNINITIALISED) && (semid_setup != -1)){
		printf("Setup: Now Destroying Setup Semaphores...\n");
		if(semctl(semid_setup, semnum, IPC_RMID) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));		
			return -1;
		}		
	}

	if((semid_app1Enc1 != UNINITIALISED) && (semid_app1Enc1 != -1)){
		printf("Setup: Now Destroying App1-Enc1 Semaphores...\n");
		if(semctl(semid_app1Enc1, semnum, IPC_RMID) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));		
			return -1;
		}		
	}

	if((semid_app2Enc2 != UNINITIALISED) && (semid_app2Enc2 != -1)){
		printf("Setup: Now Destroying App2-Enc2 Semaphores...\n");
		if(semctl(semid_app2Enc2, semnum, IPC_RMID) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));	
			return -1;	
		}			
	}

	if((semid_enc1Channel != UNINITIALISED) && (semid_enc1Channel != -1)){
		printf("Setup: Now Destroying Enc1-Channel Semaphores...\n");
		if(semctl(semid_enc1Channel, semnum, IPC_RMID) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));	
			return -1;	
		}				
	}

	if((semid_enc2Channel != UNINITIALISED) && (semid_enc2Channel != -1)){
		printf("Setup: Now Destroying Enc2-Channel Semaphores...\n");
		if(semctl(semid_enc2Channel, semnum, IPC_RMID) == -1){
			printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));	
			return -1;	
		}	
	}

	return 0;
}

int semaphores_creation(int* semid_setupPtr,int* semid_app1Enc1ptr,int* semid_app2Enc2ptr,int* semid_enc1Channelptr,int* semid_enc2Channelptr){
	
	/*----Creating Semaphore for Setup execution----*/
	printf("Setup: Creating Semaphores for Setup execution... - SETUP_SEMAPHORE_KEY:%d\n", SEMKEY_SETUP);
	*semid_setupPtr = semget(SEMKEY_SETUP, NSEMS_SETUP, PERMS | IPC_CREAT);
	if(*semid_setupPtr == -1){
		printf("Setup: Semget failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}
	
	/*----Creating Semaphores for App1-Enc1----*/
	printf("Setup: Creating Semaphores for App1-Enc1... - APP1_ENC1_SEMAPHORE_KEY:%d\n", SEMKEY_APP1_ENC1);
	*semid_app1Enc1ptr = semget(SEMKEY_APP1_ENC1, NSEMS_APP_ENC, PERMS | IPC_CREAT);
	if(*semid_app1Enc1ptr == -1){
		printf("Setup: Semget failed! Error Message: |%s|\n", strerror(errno));
		return semaphores_destruction(*semid_setupPtr,*semid_app1Enc1ptr,*semid_app2Enc2ptr,*semid_enc1Channelptr,*semid_enc2Channelptr);
	}

	/*----Creating Semaphores for App2-Enc2----*/
	printf("Setup: Creating Semaphores for App2-Enc2... - APP2_ENC2_SEMAPHORE_KEY:%d\n", SEMKEY_APP2_ENC2);
	*semid_app2Enc2ptr = semget(SEMKEY_APP2_ENC2, NSEMS_APP_ENC, PERMS | IPC_CREAT);
	if(*semid_app2Enc2ptr == -1){
		printf("Setup: Semget failed! Error Message: |%s|\n", strerror(errno));
		return semaphores_destruction(*semid_setupPtr,*semid_app1Enc1ptr,*semid_app2Enc2ptr,*semid_enc1Channelptr,*semid_enc2Channelptr);
	}

	/*----Creating Semaphores for Enc1-Channel----*/
	printf("Setup: Creating Semaphores for Enc1-Channel... - ENC1_CHANNEL_SEMAPHORE_KEY:%d\n", SEMKEY_ENC1_CHANNEL);
	*semid_enc1Channelptr = semget(SEMKEY_ENC1_CHANNEL, NSEMS_ENC_CHAN, PERMS | IPC_CREAT);
	if(*semid_enc1Channelptr == -1){
		printf("Setup: Semget failed! Error Message: |%s|\n", strerror(errno));
		return semaphores_destruction(*semid_setupPtr,*semid_app1Enc1ptr,*semid_app2Enc2ptr,*semid_enc1Channelptr,*semid_enc2Channelptr);
	}

	/*----Creating Semaphores for Enc2-Channel----*/
	printf("Setup: Creating Semaphores for Enc2-Channel... - ENC2_CHANNEL_SEMAPHORE_KEY:%d\n", SEMKEY_ENC2_CHANNEL);
	*semid_enc2Channelptr = semget(SEMKEY_ENC2_CHANNEL, NSEMS_ENC_CHAN, PERMS | IPC_CREAT);
	if(*semid_enc2Channelptr == -1){
		printf("Setup: Semget failed! Error Message: |%s|\n", strerror(errno));
		return semaphores_destruction(*semid_setupPtr,*semid_app1Enc1ptr,*semid_app2Enc2ptr,*semid_enc1Channelptr,*semid_enc2Channelptr);
	}

	return 0;

}

int semaphores_initialisation(int semid_setup, int semid_app1Enc1,int semid_app2Enc2,int semid_enc1Channel,int semid_enc2Channel){

	int semnum;

	union semun{
		int val;
		struct semid_ds *buf;
		unsigned short *array;
	} argAppEnc, argEncChan, argSetup;

	unsigned short setupArray[NSEMS_SETUP] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //App1,App2,Enc1,Enc2,Chan (wait) and then 5 for (finish)
	unsigned short appEncArray[NSEMS_APP_ENC] = {1, QUEUE_ARRAY_SIZE, 0, 1, QUEUE_ARRAY_SIZE, 0}; //Input Buffer(mutex,empty,full) Output Buffer(mutex,empty,full)
	unsigned short encChanArray[NSEMS_ENC_CHAN] = {1, QUEUE_ARRAY_SIZE, 0, 1, QUEUE_ARRAY_SIZE, 0, 0}; //Same as above PLUS ONE MORE semaphore to synchronise between thread1 and thread2 of Encoder

	argAppEnc.array = appEncArray;
	argEncChan.array = encChanArray;
	argSetup.array = setupArray;

	/*----Setup Semaphores Initialisation----*/
	if(semctl(semid_setup, semnum, SETALL, argSetup) == -1){
		printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));
		printf("Setup: Destroying semaphore...\n");
		return -1;
	}

	/*----App1-Enc1 Semaphores Initialisation----*/
	if(semctl(semid_app1Enc1, semnum, SETALL, argAppEnc) == -1){
		printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));
		printf("Setup: Destroying semaphore...\n");
		return -1;
	}

	/*----App2-Enc2 Semaphores Initialisation----*/
	if(semctl(semid_app2Enc2, semnum, SETALL, argAppEnc) == -1){
		printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));
		printf("Setup: Destroying semaphore...\n");
		return -1;
	}

	/*----Enc1-Channel Semaphores Initialisation----*/
	if(semctl(semid_enc1Channel, semnum, SETALL, argEncChan) == -1){
		printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));
		printf("Setup: Destroying semaphore...\n");
		return -1;
	}

	/*----Enc2-Channel Semaphores Initialisation----*/
	if(semctl(semid_enc2Channel, semnum, SETALL, argEncChan) == -1){
		printf("Setup: Semctl failed! Error Message: |%s|\n", strerror(errno));
		printf("Setup: Destroying semaphore...\n");
		return -1;
	}

	return 0;

}

int createAndInitialiseSemaphores(int* semid_setup,int* semid_app1Enc1,int* semid_app2Enc2,int* semid_enc1Channel,int* semid_enc2Channel){

	/*----Semaphores Creation----*/
	printf("\nSetup: Creating all Semaphores...\n");
	if(semaphores_creation(semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Channel,semid_enc2Channel) == -1){
		printf("Setup: Failed at Semaphores Creation!\n");	
		printf("Exiting...");
		exit(1);
	}

	/*----Semaphores Initialisation----*/
	printf("\nSetup: Initialising Semaphores...\n");
	if(semaphores_initialisation(*semid_setup,*semid_app1Enc1,*semid_app2Enc2,*semid_enc1Channel,*semid_enc2Channel) == -1){
		printf("Setup: Failed at Semaphores Initialisation!\n");
		printf("Setup: Now Destroying Semaphores...\n");
		if(semaphores_destruction(*semid_setup,*semid_app1Enc1,*semid_app2Enc2,*semid_enc1Channel,*semid_enc2Channel) == -1){
			printf("Setup: Failed at Semaphores Destruction!\n");
		}
		printf("Exiting...");		
		exit(1);
	}
	
	return 0;

}

/*=======================================COMMUNICATING WITH PROCESSES=====================================*/

int signalProcessesToBegin(int semid_setup){ //All Processes will be waiting for Setup to Signal them before they begin their work
	
	int i;
	struct sembuf signalStart;
	
	for(i=0; i < 5; i++){ //The FIRST 5 semaphores are used by the processes to wait for the setup to complete
		signalStart.sem_num = i;
		signalStart.sem_op = 1;
		signalStart.sem_flg = 0;
		if(semop(semid_setup, &signalStart, 1) == -1){
			printf("Setup: Failed to UP %dth Setup Semaphore! Error: |%s|\n",i, strerror(errno));
			return -1;
		}
	}

	return 0;

}

int waitForProcessesToFinish(int semid_setup){ //Blocking and waiting until all processes finish
	
	int i;
	struct sembuf waitFinish;
	
	for(i=5; i < 10; i++){ //The LAST 5 semaphores are used by the processes to signal the setup that they finished execution
		waitFinish.sem_num = i;
		waitFinish.sem_op = -1;
		waitFinish.sem_flg = 0;
		if(semop(semid_setup, &waitFinish, 1) == -1){
			printf("Setup: Failed to DOWN %dth Setup Semaphore!\n",i);
			return -1;
		}
		switch(i){
			case 5:
				printf("Setup: Process App1 must have finished successfully!\n");
				break;
			case 6:
				printf("Setup: Process App2 must have finished successfully!\n");
				break;
			case 7:
				printf("Setup: Process Enc1 must have finished successfully!\n");
				break;
			case 8:
				printf("Setup: Process Enc2 must have finished successfully!\n");
				break;
			case 9:
				printf("Setup: Process Chan must have finished successfully!\n");
				break;
		}
	}

	/*printf("Setup: Waiting ONLY for App1 To finish...\n");
	waitFinish.sem_num = 5;
	waitFinish.sem_op = -1;
	waitFinish.sem_flg = 0;
	if(semop(semid_setup, &waitFinish, 1) == -1){
		printf("Setup: Failed to DOWN %dth Setup Semaphore!\n",i);
		return -1;
	}

	printf("Setup: App1 must have finished successfully!\n");*/

	return 0;

}

int signalAndWaitProcesses(int semid_setup,int semid_app1Enc1,int semid_app2Enc2,int semid_enc1Channel,int semid_enc2Channel,int shmid_app1Enc1,int shmid_app2Enc2,int shmid_enc1Channel,int shmid_enc2Channel){

	/*----Signaling processes to start-----*/
	printf("\nSetup: Setup is complete! Signaling P1,P2,ENC1,ENC2 and CHAN to begin work!\n");
	if(signalProcessesToBegin(semid_setup) == -1){
		printf("Setup: Failed to signal Processes to begin!\n");
		ShmAndSemDestruction(shmid_app1Enc1,shmid_app2Enc2,shmid_enc1Channel,shmid_enc2Channel,semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Channel,semid_enc2Channel);
		printf("Exiting...");
		exit(1);
	}
	
	/*----Wait for processes to finish----*/
	printf("\nSetup: Now I am going to sleep until all processes finish...\n");
	if(waitForProcessesToFinish(semid_setup) == -1){
		printf("Setup: Failed to wait for Processes to finish!\n");
		ShmAndSemDestruction(shmid_app1Enc1,shmid_app2Enc2,shmid_enc1Channel,shmid_enc2Channel,semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Channel,semid_enc2Channel);
		printf("Exiting...");
		exit(1);
	}	

	return 0;

}

/*==========================SEMAPHORE AND SHARED MEMORY TEAR DOWN OPERATIONS=========================*/

int ShmAndSemDestruction(int shmid_app1Enc1,int shmid_app2Enc2,int shmid_enc1Channel,int shmid_enc2Channel,int semid_setup,int semid_app1Enc1,int semid_app2Enc2,int semid_enc1Channel,int semid_enc2Channel){

	/*----Shared Memory Segments Destruction----*/
	printf("\nSetup: Destroying Shared Memory Segments...\n");
	if(shared_memory_destruction(shmid_app1Enc1,shmid_app2Enc2,shmid_enc1Channel,shmid_enc2Channel) == -1){
		printf("Setup: Failed at shared memory destruction!\n");
		printf("\nSetup: Destroying all Semaphores...\n");
		if(semaphores_destruction(semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Channel,semid_enc2Channel) == -1){
			printf("Setup: Failed at Semaphores Destruction!\n");	
		}			
		printf("Exiting...");
		exit(1);
	}	

	/*----Semaphores Destruction----*/
	printf("\nSetup: Destroying all Semaphores...\n");
	if(semaphores_destruction(semid_setup,semid_app1Enc1,semid_app2Enc2,semid_enc1Channel,semid_enc2Channel) == -1){
		printf("Setup: Failed at Semaphores Destruction!\n");
		printf("Exiting...");		
		exit(1);
	}

	return 0;
	
}
