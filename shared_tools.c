#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "shared_tools.h"

/*===========================SHARED MEMORY OPERATIONS COMMON FOR APP, ENC AND CHAN PROCESSES======================*/

int attachToSharedMemoryArea(key_t shm_key, int* shmid_Ptr, SharedMemorySegment** shmPtr){
	
	void* tmp_shm_ptr = (void*) NULL;

	/*----Shmget()----*/
	*shmid_Ptr = shmget(shm_key, sizeof(SharedMemorySegment), PERMS | IPC_CREAT);
	if(*shmid_Ptr == -1){
		printf("Shmget failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	/*----Shmattach()----*/
	tmp_shm_ptr = shmat(*shmid_Ptr, (void*) 0, 0);
	if(tmp_shm_ptr == (char*) -1){
		printf("Shmat failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	*shmPtr = (SharedMemorySegment*) tmp_shm_ptr;
		
	return 0;
	
}

int detachSharedMemory(SharedMemorySegment* shmPtr){

	/*----ShmDetach----*/
	if(shmdt((void*) shmPtr) == -1){
		printf("Shmdt failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	return 0;

}

/*===========================SEMAPHORE OPERATIONS COMMON FOR APP, ENC AND CHAN PROCESSES======================*/

int getSemaphores(key_t sem_key, int* semid,int nsems){

	*semid = semget(sem_key, nsems, PERMS | IPC_CREAT);
	if(*semid == -1){
		printf("Semget failed! Error Message: |%s|\n", strerror(errno));
		return -1;
	}

	return 0;

}

int getSetupSemaphores(char* processName, key_t sem_key, int* semid_setup){

	/*----semget----*/
	printf("%s: Attempting to sem_get Setup Semaphore Set... - KEY: %d\n",processName, (int) sem_key);
	*semid_setup = semget(sem_key, NSEMS_SETUP, PERMS | IPC_CREAT);
	if(*semid_setup == -1){
		printf("%s: Semget failed! Error Message: |%s|\n", processName, strerror(errno));
		return -1;
	}

	return 0;

}

/*===========================COMMUNICATING WITH SETUP COMMON FOR APP, ENC AND CHAN PROCESSES======================*/

/*----This functions helps the Processes either to Wait for Setup to complete or to Report their completion to it----*/
/*----The Setup Semaphores are 10 Semaphores with 0,1,2,3,4 to be used for App1,App2,Enc1,Enc2,Chan to wait for Setup to complete and ----*/
/*----5,6,7,8,9 to be used by the same processes to Report their completion----*/
/*----This functions helps pick the right semaphore depending on which process calls the function----*/

int waitOrSignalSetup(char* processName,int semid_setup,int type,int processNumber,int opType){

	if(opType == DOWN){ /*----Process will be waiting for the Setup to complete----*/

		printf("%s: Waiting for Setup Process to singal...\n", processName);
		struct sembuf waitFinish;

		if(type == APP){ /*---Application Process---*/
			if(processNumber == 1){ /*---App1---*/
				waitFinish.sem_num = 0;
			}
			else{ /*---App2---*/
				waitFinish.sem_num = 1;
			}
		}
		else if(type == ENC){
			if(processNumber == 1){
				waitFinish.sem_num = 2;
			}
			else{
				waitFinish.sem_num = 3;
			}
		}
		else if(type == CHAN){
			waitFinish.sem_num = 4;
		}
		else{
			printf("waitForSetupToFinish: This Process is neither App nor Enc nor Chan!?\n");
			exit(1);
		}

		waitFinish.sem_op = -1;
		waitFinish.sem_flg = 0;
		if(semop(semid_setup, &waitFinish, 1) == -1){ /*----Perform DOWN On selected Semaphore----*/
			printf("%s: Failed to DOWN Setup Semaphore!\n",processName);
			return -1;
		}

	}
	else if(opType == UP){

		printf("%s: Signaling to Setup Process that I finished my work...\n", processName);
		struct sembuf signalFinish;

		if(type == APP){
			if(processNumber == 1){
				signalFinish.sem_num = 5;
			}
			else{
				signalFinish.sem_num = 6;
			}
		}
		else if(type == ENC){
			if(processNumber == 1){
				signalFinish.sem_num = 7;
			}
			else{
				signalFinish.sem_num = 8;
			}
		}
		else if(type == CHAN){
			signalFinish.sem_num = 9;
		}
		signalFinish.sem_op = 1;
		signalFinish.sem_flg = 0;
		if(semop(semid_setup, &signalFinish, 1) == -1){
			printf("%s: Failed to UP Setup Semaphore!\n",processName);
			return -1;
		}

	}
	else{
		printf("%s: Invalid OpType!\n", processName);
		exit(1);
	}

	return 0;

}

int waitForSetupToFinish(char* processName,int semid_setup,int type,int processNumber){ //Will Block calling Process if Setup has not finished setting shared memories and semaphores*/

	return waitOrSignalSetup(processName,semid_setup,type,processNumber,DOWN);

}

int signalFinish(char* processName,int semid_setup,int type,int processNumber){

	return waitOrSignalSetup(processName,semid_setup,type,processNumber,UP);

}

/*==================================CONSUMER AND PRODUCER BUFFER FUNCTIONS===================================*/

/*----Basic Producer Function that uses an Empty, a Full and a Mutex Semaphore----*/
/*----The Producer waits until an Empty slot has appeared then it locks the Queue Mutex and----*/
/*----Adds its item to the buffer. Then it unlocks the Queue Mutex and Ups the Full Semaphore----*/
/*----To report a new item has arrived to the Consumer----*/
/*----If a TERM Message Arrives while a Process is in this function the function will revert the Semaphores----*/
/*----To the state they were and exit---*/

int addToSharedBuffer(char* threadName,MessagePacket* packet_ptr,MessageQueueArray* queueArrayPtr,int semid,int sem_numMutex,int sem_numEmpty,int sem_numFull,int* termFlag,pthread_mutex_t* malloc_mtx,pthread_mutex_t* termFlagLock){

	struct sembuf semaphoreOperation;
	MessagePacket* tmp_packet = (MessagePacket*) NULL;

	if(packet_ptr == NULL){
		printf("%s - addToSharedBuffer: packet_ptr is NULL!\n",threadName);
		fflush(stdout);
		return -1;
	}

	//printf("%s - addToSharedBuffer: Attempting to ADD a MessagePacket to the Shared Memory Buffer...\n", threadName);
	//fflush(stdout);

	/*---------------Wait for an empty space----------------*/
	semaphoreOperation.sem_num = sem_numEmpty;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	pthread_mutex_lock(termFlagLock);
	if(*termFlag == TERM){ /*TERM Message has arrived/Stopping execution*/
		printf("%s - addToSharedBuffer: Received TERM Message before passing my message! Exiting\n", threadName);
		fflush(stdout);
		pthread_mutex_unlock(termFlagLock);
		return 0;
	}
	pthread_mutex_unlock(termFlagLock);

	//printf("%s - addToSharedBuffer: Will DOWN Empty Semaphore...\n",threadName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - addToSharedBuffer: Failed to DOWN EMPTY Semaphore!\n",threadName);
		fflush(stdout);
		return -1;
	}	

	//printf("%s - addToSharedBuffer: DOWNed Empty Semaphore!\n",threadName);
	//fflush(stdout);

	pthread_mutex_lock(termFlagLock);
	if(*termFlag == TERM){ /*TERM Message has arrived/Stopping execution*/
		printf("%s - addToSharedBuffer: Received TERM Message before passing my message! Exiting\n", threadName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numEmpty; /*Revert Semaphore Operations*/
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n",threadName);
			fflush(stdout);
		}	

		pthread_mutex_unlock(termFlagLock);

		return 0;
	}	
	pthread_mutex_unlock(termFlagLock);

	/*----Down Queue Mutex----*/
	semaphoreOperation.sem_num = sem_numMutex;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	pthread_mutex_lock(termFlagLock);
	if(*termFlag == TERM){ /*TERM Message has arrived/Stopping execution*/
		printf("%s - takeFromSharedBuffer: Received TERM Message before passing my message! Exiting\n", threadName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numEmpty; /*Revert Semaphore Operations*/
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s-IN: takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n",threadName);
			fflush(stdout);
		}	

		pthread_mutex_unlock(termFlagLock);

		return 0;
	}
	pthread_mutex_unlock(termFlagLock);

	//printf("%s - addToSharedBuffer: DOWNing Queue Mutex...\n",threadName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s-IN: addToSharedBuffer: Failed to DOWN MUTEX Semaphore!\n",threadName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numEmpty; //Revert Semaphore Operations
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s-IN: takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n",threadName);
			fflush(stdout);
		}	

		return -1;
	}	

	//printf("%s - addToSharedBuffer: DOWNed Queue Mutex!\n",threadName);
	//fflush(stdout);

	pthread_mutex_lock(termFlagLock);
	if(*termFlag == TERM){ /*TERM Message has arrived/Stopping execution*/
		printf("%s:-IN: Received TERM Message before passing my message! Exiting\n", threadName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numMutex; /*Revert Semaphore Operations*/
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s-IN: addToSharedBuffer: Failed to UP MUTEX Semaphore!\n",threadName);
			fflush(stdout);
		}	

		semaphoreOperation.sem_num = sem_numEmpty;
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s-IN: takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n",threadName);
			fflush(stdout);
		}	

		pthread_mutex_unlock(termFlagLock);

		return 0;
	}
	pthread_mutex_unlock(termFlagLock);

	//printf("%s - addToSharedBuffer: Calling enQueue_arrayQueue...!\n",threadName);
	//fflush(stdout);

	/*----Store the item----*/
	if(enQueue_arrayQueue(queueArrayPtr, *packet_ptr) == -1){
		printf("%s-IN: addToSharedBuffer: enQueue_arrayQueue Failed!\n",threadName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numMutex; //Revert Semaphore Operations
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s-IN: addToSharedBuffer: Failed to UP MUTEX Semaphore!\n",threadName);
			fflush(stdout);
		}

		semaphoreOperation.sem_num = sem_numEmpty;
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s-IN: takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n",threadName);
			fflush(stdout);
		}	

		return -1;

	}

	//printf("%s - addToSharedBuffer: Testing Packet...\n",threadName);
	//fflush(stdout);

	/*----Testing to see what was inserted----*/
	/*pthread_mutex_lock(malloc_mtx);
	tmp_packet = rear_arrayQueue(*queueArrayPtr);
	pthread_mutex_unlock(malloc_mtx);
	if(tmp_packet != NULL){
		printf("%s-IN: MessagePacketTEST: |%s|\n", threadName, tmp_packet->message_buffer);
		fflush(stdout);
		printf("%s-IN: MessagePacketTEST: |%s|\n", threadName, tmp_packet->md5_hash);
		fflush(stdout);
		printf("%s-IN: MessagePacketTEST: |%d|\n", threadName, tmp_packet->retryFlag);
		fflush(stdout);
		printf("%s-IN: MessagePacketTEST: |%d|\n", threadName, tmp_packet->termFlag);
		fflush(stdout);
		free(tmp_packet);
	}
	else{
		printf("%s-IT: REAR_ARRAYQUEUE FAILED!\n", threadName);
		fflush(stdout);
	}*/

	//printf("%s - addToSharedBuffer: UPing Queue Mutex...\n",threadName);
	//fflush(stdout);

	/*----Up Queue Mutex----*/
	semaphoreOperation.sem_num = sem_numMutex;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - addToSharedBuffer: Failed to UP MUTEX Semaphore!\n",threadName);
		fflush(stdout);
		return -1;
	}	

	//printf("%s - addToSharedBuffer: UP Queue Mutex!\n",threadName);
	//fflush(stdout);

	/*----Report full new slot----*/
	semaphoreOperation.sem_num = sem_numFull;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	//printf("%s - addToSharedBuffer: UPed Queue Mutex!\n",threadName);
	//fflush(stdout);

	//printf("%s - addToSharedBuffer: UPing FULL semaphore...\n",threadName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - addToSharedBuffer: Failed to UP FULL Semaphore!\n",threadName);
		fflush(stdout);
		return -1;
	}	

	//printf("%s - addToSharedBuffer: UPed FULL semaphore!\n",threadName);
	//fflush(stdout);

	return 0;

}

/*----Basic Consumer Function that uses a Full, an Empty and a Mutex Semaphore----*/
/*----The Consumer waits until an item has appeared then it locks the Queue Mutex and----*/
/*----Removes the item from the buffer. Then it unlocks the Queue Mutex and Ups the Empty Semaphore----*/
/*----To report a new empty slot has been created to the Producer----*/
/*----If a TERM Message Arrives while a Process is in this function the function will revert the Semaphores----*/
/*----To the state they were and exit---*/

MessagePacket* takeFromSharedBuffer(char* threadName,MessageQueueArray* queueArrayPtr,int semid,int sem_numMutex,int sem_numEmpty,int sem_numFull,int* termFlag,pthread_mutex_t* malloc_mtx,pthread_mutex_t* termFlagLock){

	struct sembuf semaphoreOperation;
	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	//printf("%s - takeFromSharedBuffer: Attempting to CONSUME a MessagePacket from the Shared Memory Buffer...\n",threadName);
	//fflush(stdout);

	/*----Wait for a stored MessagePacket----*/

	semaphoreOperation.sem_num = sem_numFull;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	pthread_mutex_lock(termFlagLock);
	if(*termFlag == TERM){ /*TERM Message has arrived/Stopping execution*/
		printf("%s - takeFromSharedBuffer: Received TERM Message from IN Thread before grabbing a Packet!\n",threadName);
		fflush(stdout);

		pthread_mutex_unlock(termFlagLock);

		return (MessagePacket*) NULL;
	}
	pthread_mutex_unlock(termFlagLock);
	//printf("%s - takeFromSharedBuffer: DOWNing FULL Semaphore...\n",threadName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - takeFromSharedBuffer: takeFromSharedBuffer: Failed to DOWN FULL Semaphore!\n",threadName);
		fflush(stdout);
		return (MessagePacket*) NULL;
	}	

	//printf("%s - takeFromSharedBuffer: DOWNed FULL Semaphore!\n",threadName);
	//fflush(stdout);
	pthread_mutex_lock(termFlagLock);
	if(*termFlag == TERM){ /*TERM Message has arrived/Stopping execution*/
		printf("%s - takeFromSharedBuffer: Received TERM Message from IN Thread before grabbing a Packet!\n",threadName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numFull; /*Revert Semaphore Operations*/
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to DOWN FULL Semaphore!\n",threadName);
			fflush(stdout);
		}	
		pthread_mutex_unlock(termFlagLock);
		return (MessagePacket*) NULL;
	}
	pthread_mutex_unlock(termFlagLock);
	//printf("%s - takeFromSharedBuffer: DOWNing Queue MUTEX...\n",threadName);
	//fflush(stdout);

	/*----Down Queue Mutex----*/
	semaphoreOperation.sem_num = sem_numMutex;
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - takeFromSharedBuffer: Failed to DOWN MUTEX Semaphore!\n",threadName);
		fflush(stdout);
		return (MessagePacket*) NULL;
	}	

	//printf("%s - takeFromSharedBuffer: DOWNed MUTEX Semaphore!\n",threadName);
	//fflush(stdout);
	pthread_mutex_lock(termFlagLock);
	if(*termFlag == TERM){ /*TERM Message has arrived/Stopping execution*/
		printf("%s - takeFromSharedBuffer: Received TERM Message from IN Thread before grabbing a Packet!\n",threadName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numFull; /*Revert Semaphore Operations*/
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to DOWN FULL Semaphore!\n",threadName);
			fflush(stdout);
		}	

		semaphoreOperation.sem_num = sem_numMutex;
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to UP MUTEX Semaphore!\n",threadName);
			fflush(stdout);
		}	
		pthread_mutex_unlock(termFlagLock);
		return (MessagePacket*) NULL;
	}
	pthread_mutex_unlock(termFlagLock);
	//printf("%s - takeFromSharedBuffer: Calling deQueue_arrayQueue...\n",threadName);
	//fflush(stdout);

	/*----Remove the item----*/
	pthread_mutex_lock(malloc_mtx);
	packet_ptr = deQueue_arrayQueue(queueArrayPtr);
	pthread_mutex_unlock(malloc_mtx);

	if(packet_ptr == NULL){
		printf("%s - takeFromSharedBuffer: deQueue_arrayQueue Failed!\n",threadName);
		fflush(stdout);

		semaphoreOperation.sem_num = sem_numMutex;
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to UP MUTEX Semaphore!\n",threadName);
			fflush(stdout);
		}

		semaphoreOperation.sem_num = sem_numFull;
		semaphoreOperation.sem_op = 1;
		semaphoreOperation.sem_flg = 0;	

		if(semop(semid, &semaphoreOperation, 1) == -1){
			printf("%s - takeFromSharedBuffer: Failed to UP FULL Semaphore!\n",threadName);
			fflush(stdout);
		}	

		return (MessagePacket*) NULL;
	}

	/*----Up Queue Mutex----*/
	semaphoreOperation.sem_num = sem_numMutex;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	//printf("%s - takeFromSharedBuffer: UPing MUTEX semaphore...\n",threadName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - takeFromSharedBuffer: Failed to UP MUTEX Semaphore!\n",threadName);
		fflush(stdout);
		//pthread_mutex_lock(free_mtx);
		free(packet_ptr);
		//pthread_mutex_unlock(free_mtx);
		return (MessagePacket*) NULL;
	}	

	/*----Report new empty slot----*/
	semaphoreOperation.sem_num = sem_numEmpty;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	//printf("%s - takeFromSharedBuffer: UPed EMPTY semaphore!\n",threadName);
	//fflush(stdout);

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s - takeFromSharedBuffer: takeFromSharedBuffer: Failed to UP EMPTY Semaphore!\n",threadName);
		fflush(stdout);
		//pthread_mutex_lock(free_mtx);
		free(packet_ptr);
		//pthread_mutex_unlock(free_mtx);
		return (MessagePacket*) NULL;
	}	

	return packet_ptr;

}

int sigFinishToOther(char* processName, pthread_mutex_t* mutex_ptr,int* termFlag,int semid,int semnum){ //Used by one thread to notify the other of exiting

	struct sembuf semaphoreOperation;

	pthread_mutex_lock(mutex_ptr);
	if(*termFlag != TERM){
		*termFlag = TERM;
	}
	pthread_mutex_unlock(mutex_ptr);

	semaphoreOperation.sem_num = semnum; //Help the other thread unblock and see exit flag
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;	

	if(semop(semid, &semaphoreOperation, 1) == -1){
		printf("%s-THREAD-sigFinishToOther: Failed to UP given semnum!\n",processName);
		fflush(stdout);
	}
	
}
