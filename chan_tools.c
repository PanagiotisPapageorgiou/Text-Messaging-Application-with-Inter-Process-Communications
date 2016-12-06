#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <time.h>
#include "chan_tools.h"

extern ChanThreadSharedData globalChanData;

int attachToEncChanSharedMemory(key_t shm_key, int* shmid_encChanPtr, SharedMemorySegment** encChanShmPtr){
	
	printf("%s: Attempting to shm_get and shm_attach to Enc-Chan Shared Memory Segment... - KEY:%d\n", globalChanData.processName, (int) shm_key);
	return attachToSharedMemoryArea(shm_key, shmid_encChanPtr, encChanShmPtr);

}

int detachFromEncChanSharedMemory(SharedMemorySegment* encChanShmPtr){

	printf("%s: Detaching from Enc-Chan Shared Memory Segment!\n", globalChanData.processName);
	return detachSharedMemory(encChanShmPtr);

}

int getEncChanSemaphores(key_t sem_key, int* semid_encChan){

	/*----semget----*/
	printf("%s: Attempting to sem_get EncChan Semaphore Set... - KEY: %d\n", globalChanData.processName, (int) sem_key);
	return getSemaphores(sem_key, semid_encChan, NSEMS_ENC_CHAN);

}

int ChanWaitsForSetupToFinish(){

	return waitForSetupToFinish(globalChanData.processName,globalChanData.semid_setup,CHAN,-1);

}

int ChanSignalsToSetupFinish(){

	return signalFinish(globalChanData.processName,globalChanData.semid_setup,CHAN,-1);

}

int argumentHandling(int argc, char* argv[]){

	globalChanData.processName = (char*) NULL;
	globalChanData.shmid_enc1Chan = UNINITIALISED;
	globalChanData.shmid_enc2Chan = UNINITIALISED;
	globalChanData.semid_enc1Chan = UNINITIALISED;
	globalChanData.semid_enc2Chan = UNINITIALISED;
	globalChanData.semid_setup = UNINITIALISED;

	globalChanData.enc1ChanShmPtr = (SharedMemorySegment*) NULL;
	globalChanData.enc2ChanShmPtr = (SharedMemorySegment*) NULL;

	char c;
	globalChanData.processName = argv[0]; //Get Process Name to Use when printing

	if(argc == 1){
		printf("%s: Please provide SETUP_SEMAPHORE_KEY: ", globalChanData.processName);
		scanf("%d", &(globalChanData.sem_setupKey));
		c = getchar(); //Get newline
		printf("%s: Please provide ENC1-CHAN_SEMAPHORE_KEY: ", globalChanData.processName);
		scanf("%d", &(globalChanData.sem_enc1Chankey));	
		c = getchar(); //Get newline
		printf("%s: Please provide ENC1-CHAN_SHARED_MEMORY_KEY: ", globalChanData.processName);
		scanf("%d", &(globalChanData.shm_enc1Chankey));
		c = getchar(); //Get newline
		printf("%s: Please provide ENC2-CHAN_SEMAPHORE_KEY: ", globalChanData.processName);
		scanf("%d", &(globalChanData.sem_enc2Chankey));
		c = getchar(); //Get newline
		printf("%s: Please provide ENC2-CHAN_SHARED_MEMORY_KEY: ", globalChanData.processName);
		scanf("%d", &(globalChanData.shm_enc2Chankey));
		c = getchar(); //Get newline
		printf("%s: Please the Corruption Possibility: \n", globalChanData.processName);
		scanf("%lf", &(globalChanData.corruptionPossibility));
		c = getchar(); //Get newline
		if((globalChanData.corruptionPossibility < 0) || (globalChanData.corruptionPossibility >= 100)){
			printf("%s: corruptionPossibility must be between 0 and 100!\n", globalChanData.processName);
			exit(1);
		}	
	}
	else if(argc == 6){
		globalChanData.sem_setupKey = (key_t) atoi(argv[1]); //Get Semaphore key for Setup Semaphores
		globalChanData.sem_enc1Chankey = (key_t) atoi(argv[2]); 
		globalChanData.shm_enc1Chankey = (key_t) atoi(argv[3]); 
		globalChanData.sem_enc2Chankey = (key_t) atoi(argv[4]); 
		globalChanData.shm_enc2Chankey = (key_t) atoi(argv[5]); 
		printf("%s: Please the Corruption Possibility: \n", globalChanData.processName);
		scanf("%lf", &(globalChanData.corruptionPossibility));
		c = getchar(); //Get newline
		if((globalChanData.corruptionPossibility < 0) || (globalChanData.corruptionPossibility >= 100)){
			printf("%s: corruptionPossibility must be between 0 and 100!\n", globalChanData.processName);
			exit(1);
		}	
	}
	else{
		printf("%s: Channel process should be run with only 5 arguments and AFTER setup process only!\n",argv[0]);
		printf("%s: HOW TO RUN: %s [SETUP_SEM_KEY] [ENC1-CHAN_SEM_KEY] [ENC1-CHAN_SHM_KEY] [ENC2-CHAN_SEM_KEY] [ENC2-CHAN_SHM_KEY]\n",argv[0],argv[0]);
		printf("%s: Example: %s 54410 24411 34313 24412 34314\n",argv[0],argv[0]);
		exit(1);
	}

}

int chanSetupOperations(){

	int* argInt = (int*) NULL;

	srand((unsigned int) time(NULL));

	/*----Get Setup Semaphores----*/ 
	if(getSetupSemaphores(globalChanData.processName, globalChanData.sem_setupKey, &(globalChanData.semid_setup)) == -1){
		printf("%s: Failed to get Setup Semaphores!\n", globalChanData.processName);
		exit(1);
	}

	/*----Initialise Malloc and Free Mutexes----*/
	if(pthread_mutex_init(&(globalChanData.malloc_mtx), NULL) != 0){
		printf("%s: Failed to initialise Malloc mutex!\n", globalChanData.processName);
		exit(1);		
	}

	if(pthread_mutex_init(&(globalChanData.free_mtx), NULL) != 0){
		printf("%s: Failed to initialise Free mutex!\n", globalChanData.processName);
		pthread_mutex_destroy(&(globalChanData.malloc_mtx));
		exit(1);		
	}

	/*----Wait for Setup to finish----*/
	if(ChanWaitsForSetupToFinish() == -1){
		printf("%s: Failed to DOWN Setup Start Semaphore!\n", globalChanData.processName);
		printf("%s: Exiting...\n" , globalChanData.processName);
		exit(1);
	}

	/*----Attaching to Enc1-Chan Shared Memory----*/
	if(attachToEncChanSharedMemory(globalChanData.shm_enc1Chankey, &(globalChanData.shmid_enc1Chan), &(globalChanData.enc1ChanShmPtr)) == -1){
		printf("%s: Failed at Shared Memory shmget or shmattach!\n", globalChanData.processName);
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting...\n" , globalChanData.processName);
		exit(1);
	}

	/*----Attaching to Enc2-Chan Shared Memory----*/
	if(attachToEncChanSharedMemory(globalChanData.shm_enc2Chankey, &(globalChanData.shmid_enc2Chan), &(globalChanData.enc2ChanShmPtr)) == -1){
		printf("%s: Failed at Shared Memory shmget or shmattach!\n", globalChanData.processName);
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting...\n" , globalChanData.processName);
		exit(1);
	}

	/*----Get Enc1-Chan Semaphore Set----*/
	if(getEncChanSemaphores(globalChanData.sem_enc1Chankey, &(globalChanData.semid_enc1Chan)) == -1){
		printf("%s: Failed to get Enc1-Chan Semaphore get!\n", globalChanData.processName);
		printf("%s: Detaching from Enc1-Chan Shared Memory Segment!\n", globalChanData.processName);
		if(detachFromEncChanSharedMemory(globalChanData.enc1ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting...\n" , globalChanData.processName);
		exit(1);
	}

	/*----Get Enc2-Chan Semaphore Set----*/
	if(getEncChanSemaphores(globalChanData.sem_enc2Chankey, &(globalChanData.semid_enc2Chan)) == -1){
		printf("%s: Failed to get Enc2-Chan Semaphore get!\n", globalChanData.processName);
		printf("%s: Detaching from Enc2-Chan Shared Memory Segment!\n", globalChanData.processName);
		if(detachFromEncChanSharedMemory(globalChanData.enc1ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(detachFromEncChanSharedMemory(globalChanData.enc2ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting...\n" , globalChanData.processName);
		exit(1);
	}

	/*----Initialise termFlag Mutex----*/

	globalChanData.termFlag = OFF;

	if(pthread_mutex_init(&(globalChanData.termFlagLock), NULL) != 0){
		printf("%s: Failed to initialise termFlag mutex!\n", globalChanData.processName);
		printf("%s: Detaching from Enc2-Chan Shared Memory Segment!\n", globalChanData.processName);
		if(detachFromEncChanSharedMemory(globalChanData.enc1ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(detachFromEncChanSharedMemory(globalChanData.enc2ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting improperly!\n" , globalChanData.processName);
		exit(1);		
	}

	/*----Deploy Thread to serve Encoder1----*/
	argInt = malloc(sizeof(int));
	if(argInt == NULL){
		printf("%s: Failed to allocate memory for argument of pthread_create!\n", globalChanData.processName);
		printf("%s: Detaching from Enc2-Chan Shared Memory Segment!\n", globalChanData.processName);
		if(detachFromEncChanSharedMemory(globalChanData.enc1ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(detachFromEncChanSharedMemory(globalChanData.enc2ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		printf("%s: Destroying termFlag Mutex...\n", globalChanData.processName);
		if(pthread_mutex_destroy(&(globalChanData.termFlagLock)) != 0){
			printf("%s: Failed to destroy termFlag mutex!\n", globalChanData.processName);
		}	
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting improperly!\n" , globalChanData.processName);
		exit(1);		
	}
	*argInt = 1;

	if(pthread_create(&(globalChanData.threadIDs[0]), NULL, serveEncoderThread, (void*) argInt) != 0){ //Input Thread
		printf("%s: Failed to create serveEnc1Thread thread!\n", globalChanData.processName);
		printf("%s: Detaching from Enc2-Chan Shared Memory Segment!\n", globalChanData.processName);
		if(detachFromEncChanSharedMemory(globalChanData.enc1ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(detachFromEncChanSharedMemory(globalChanData.enc2ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		printf("%s: Destroying termFlag Mutex...\n", globalChanData.processName);
		if(pthread_mutex_destroy(&(globalChanData.termFlagLock)) != 0){
			printf("%s: Failed to destroy termFlag mutex!\n", globalChanData.processName);
		}	
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting improperly!\n" , globalChanData.processName);
		exit(1);	
	}

	/*----Deploy Thread to Serve Encoder 2----*/
	argInt = malloc(sizeof(int));
	if(argInt == NULL){
		printf("%s: Failed to allocate memory for argument of pthread_create!\n", globalChanData.processName);
		printf("%s: Detaching from Enc2-Chan Shared Memory Segment!\n", globalChanData.processName);
		if(detachFromEncChanSharedMemory(globalChanData.enc1ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(detachFromEncChanSharedMemory(globalChanData.enc2ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		printf("%s: Destroying termFlag Mutex...\n", globalChanData.processName);
		if(pthread_mutex_destroy(&(globalChanData.termFlagLock)) != 0){
			printf("%s: Failed to destroy termFlag mutex!\n", globalChanData.processName);
		}	
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting improperly!\n" , globalChanData.processName);
		exit(1);		
	}
	*argInt = 2;
	
	if(pthread_create(&(globalChanData.threadIDs[1]), NULL, serveEncoderThread, (void*) argInt) != 0){ //Input Thread
		printf("%s: Failed to create serveEnc2Thread thread!\n", globalChanData.processName);
		printf("%s: Detaching from Enc2-Chan Shared Memory Segment!\n", globalChanData.processName);
		if(detachFromEncChanSharedMemory(globalChanData.enc1ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		if(detachFromEncChanSharedMemory(globalChanData.enc2ChanShmPtr) == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		}
		printf("%s: Destroying termFlag Mutex...\n", globalChanData.processName);
		if(pthread_mutex_destroy(&(globalChanData.termFlagLock)) != 0){
			printf("%s: Failed to destroy termFlag mutex!\n", globalChanData.processName);
		}	
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s: Exiting improperly!\n" , globalChanData.processName);
		exit(1);	
	}

	return 0;

}

/*===========================================THREAD FUNCTIONS==============================================*/

/*	Generates a random double number for every letter and checks if it is <= than corruptionPossibility
	If it is then that letter gets corrupted and changed randomly with another ASCII table letter*/

int corruptMessage(MessagePacket* packet_ptr){ 

	int i=0;
	double randomDouble=0.0;
	int randomAsciiNumber=0;
	int currentCharacterCode=0;

	if(globalChanData.corruptionPossibility == 0){
		printf("corruptMessage: Corruption Possibility is 0! No point in trying to corrupt!\n");
		return 0;
	}

	printf("corruptMessage: Message BEFORE corruption attempt: |%s|\n", packet_ptr->message_buffer);
	fflush(stdout);

	for(i=0; i < strlen(packet_ptr->message_buffer); i++){
		//printf("corruptMessage: Attempting to corrupt letter: |%c|\n", packet_ptr->message_buffer[i]);
		//fflush(stdout);
		randomDouble = ((double) rand() / (double) (RAND_MAX)) * 100; //Generate random double number from 0 to 100
		if(randomDouble <= globalChanData.corruptionPossibility){ //This byte will be corrupted! mwahahahaha
			//printf("corruptMessage: Generated Double: |%lf| FALLS INTO corruptionPossibility: |%lf|\n", randomDouble, globalChanData.corruptionPossibility);
			//fflush(stdout);
			currentCharacterCode = (int) packet_ptr->message_buffer[i];
			do{ //Pick a different character from the ascii table than the one that already exists
				randomAsciiNumber = rand() % 127;
				if(randomAsciiNumber != currentCharacterCode) break;
			} while(1);
			packet_ptr->message_buffer[i] = (char) randomAsciiNumber; //Corrupted character
			//printf("corruptMessage: New letter: |%c|\n", packet_ptr->message_buffer[i]);
			//fflush(stdout);			
		}
		//else{
		//	printf("corruptMessage: Generated Double: |%lf| DOESN'T FALL INTO corruptionPossibility: |%lf|\n", randomDouble, globalChanData.corruptionPossibility);
		//	fflush(stdout);
		//}
	}

	printf("corruptMessage: Message AFTER corruption attempt: |%s|\n", packet_ptr->message_buffer);
	fflush(stdout);

	return 0;

}

void* serveEncoderThread(void* argNumber){ /*The channel thread deploys 2 of these, 1 for each encoder to pass its messages to the opposite encoder*/

	int* encNumberPtr = (int*) argNumber;
	int encNumber = *encNumberPtr;

	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	printf("\n%s-T%d (%ld): Hello from serveEncoderThread(%d)!\n", globalChanData.processName, encNumber, pthread_self(), encNumber);
	fflush(stdout);

	while(globalChanData.termFlag == OFF){
		printf("\n%s-T%d: Waiting for Message from Enc%d Process...\n", globalChanData.processName, encNumber, encNumber);
		fflush(stdout);
		if(encNumber == 1){ /*---I serve enc1 Process and pass to enc2---*/
			packet_ptr = takeFromSharedBuffer("chanThread1", &(globalChanData.enc1ChanShmPtr->inputQueue), globalChanData.semid_enc1Chan,INPUT_SEM_MUTEX,INPUT_SEM_EMPTY,INPUT_SEM_FULL, &(globalChanData.termFlag), &(globalChanData.malloc_mtx), &(globalChanData.termFlagLock));
		}
		else{ /*---I serve enc2 Process and pass to enc1---*/
			packet_ptr = takeFromSharedBuffer("chanThread2", &(globalChanData.enc2ChanShmPtr->inputQueue), globalChanData.semid_enc2Chan,INPUT_SEM_MUTEX,INPUT_SEM_EMPTY,INPUT_SEM_FULL, &(globalChanData.termFlag), &(globalChanData.malloc_mtx), &(globalChanData.termFlagLock));
		}

		if(packet_ptr != NULL){

			if((packet_ptr->termFlag == OFF) && (packet_ptr->retryFlag == OFF)){ /*---Normal Message - Attempt to corrupt---*/ 
				printf("\n%s-T%d: Normal Message detected: |%s| - Attempting to corrupt it...\n", globalChanData.processName, encNumber, packet_ptr->message_buffer);
				fflush(stdout);
				corruptMessage(packet_ptr); /*---Corrupt Message---*/
			}
			else if(packet_ptr->retryFlag != OFF){ /*---Special Confirmation Message - Don't Corrupt----*/
				printf("\n%s-T%d: Special RetryFlag Message detected: |%s| - Not corrupting it!\n", globalChanData.processName, encNumber, packet_ptr->message_buffer);
				fflush(stdout);				
			}
			else{ /*----TERM Message - Don't Corrupt----*/
				printf("\n%s-T%d: TERM Message detected! Not corrupting it!\n", globalChanData.processName, encNumber);
				fflush(stdout);				
			}

			printf("\n%s-T%d: Passing Message to the opposite Encoder...\n", globalChanData.processName, encNumber);
			fflush(stdout);

			if(encNumber == 1){ /*---Pass the message to the other Encoder---*/

				addToSharedBuffer("chanThread1",packet_ptr,&(globalChanData.enc2ChanShmPtr->outputQueue),globalChanData.semid_enc2Chan,OUTPUT_SEM_MUTEX,OUTPUT_SEM_EMPTY,OUTPUT_SEM_FULL, &(globalChanData.termFlag), &(globalChanData.malloc_mtx), &(globalChanData.termFlagLock));

				if(globalChanData.termFlag == TERM){
					printf("\n%s-T%d: Received TERM from the other Thread!\n", globalChanData.processName, encNumber);
					fflush(stdout);
				}

			}
			else{

				addToSharedBuffer("chanThread2",packet_ptr,&(globalChanData.enc1ChanShmPtr->outputQueue),globalChanData.semid_enc1Chan,OUTPUT_SEM_MUTEX,OUTPUT_SEM_EMPTY,OUTPUT_SEM_FULL, &(globalChanData.termFlag), &(globalChanData.malloc_mtx), &(globalChanData.termFlagLock));

				if(globalChanData.termFlag == TERM){
					printf("\n%s-T%d: Received TERM from the other Thread!\n", globalChanData.processName, encNumber);
					fflush(stdout);
				}
			}

			if(packet_ptr->termFlag == TERM){ /*---The Message I passed was a termination message---*/
				printf("\n%s-T%d: Notifying the other Thread that I am terminating...\n", globalChanData.processName, encNumber);
				fflush(stdout);
				if(encNumber == 1){
					sigFinishToOther(globalChanData.processName, &(globalChanData.termFlagLock), &(globalChanData.termFlag), globalChanData.semid_enc2Chan, INPUT_SEM_FULL);
					sigFinishToOther(globalChanData.processName, &(globalChanData.termFlagLock), &(globalChanData.termFlag), globalChanData.semid_enc1Chan, OUTPUT_SEM_EMPTY);		
				}
				else{
					sigFinishToOther(globalChanData.processName, &(globalChanData.termFlagLock), &(globalChanData.termFlag), globalChanData.semid_enc1Chan, INPUT_SEM_FULL);
					sigFinishToOther(globalChanData.processName, &(globalChanData.termFlagLock), &(globalChanData.termFlag), globalChanData.semid_enc2Chan, OUTPUT_SEM_EMPTY);	
				}

				/*----Free Allocated Packet Memory----*/
				pthread_mutex_lock(&(globalChanData.free_mtx));
				free(packet_ptr);	
				pthread_mutex_unlock(&(globalChanData.free_mtx));
				packet_ptr = (MessagePacket*) NULL;		
		
				break;	/*---Exiting---*/
			}

			/*----Free Allocated Packet Memory----*/
			pthread_mutex_lock(&(globalChanData.free_mtx));
			free(packet_ptr);	
			pthread_mutex_unlock(&(globalChanData.free_mtx));
			packet_ptr = (MessagePacket*) NULL;	

		}
		else{ /*----Packet Pointer is NULL----*/
			if(globalChanData.termFlag == TERM){
				printf("\n%s-T%d: Received TERM from the other Thread!\n", globalChanData.processName, encNumber);
				fflush(stdout);
			}
			else{
				printf("\n%s-T%d: Null Pointer for unknown reasons!\n", globalChanData.processName, encNumber);
				fflush(stdout);				
			}
		}

	}

	printf("\n%s-T%d (%ld): TERM Value Set! Goodbye from serveEncoderThread(%d)!\n", globalChanData.processName, encNumber, pthread_self(), encNumber);
	fflush(stdout);

	pthread_exit(argNumber);

}

int chanTearDownOperations(){

	int* exit_valPtr = (int*) NULL;

	/*----Wait for Threads to Finish----*/
	printf("%s: Waiting for threads to terminate...\n", globalChanData.processName);

	pthread_join(globalChanData.threadIDs[0], (void**) &exit_valPtr);
	printf("%s: serveEnc1Thread terminated with Exit_Code: %d\n", globalChanData.processName, *exit_valPtr);
	if(exit_valPtr != NULL) free(exit_valPtr);

	pthread_join(globalChanData.threadIDs[1], (void**) &exit_valPtr);
	printf("%s: serveEnc2Thread terminated with Exit_Code: %d\n", globalChanData.processName, *exit_valPtr);
	if(exit_valPtr != NULL) free(exit_valPtr);

	/*----Destroying termFlag Mutex----*/
	printf("%s: Destroying termFlag Mutex...\n", globalChanData.processName);
	if(pthread_mutex_destroy(&(globalChanData.termFlagLock)) != 0){
		printf("%s: Failed to destroy termFlag mutex!\n", globalChanData.processName);
	}	

	/*----Destroying Malloc/Free mutexes----*/
	if(pthread_mutex_destroy(&(globalChanData.malloc_mtx)) == -1){
		printf("%s: Failed to destroy Malloc mutex!\n", globalChanData.processName);
	}
	
	if(pthread_mutex_destroy(&(globalChanData.free_mtx)) == -1){
		printf("%s: Failed to destroy Free mutex!\n", globalChanData.processName);
	}

	/*----Detach from Enc1-Chan Shared Memory----*/
	printf("%s: Detaching from Enc1-Chan Shared Memory Segment!\n", globalChanData.processName);
	if(detachFromEncChanSharedMemory(globalChanData.enc1ChanShmPtr) == -1){
		printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s - Exiting...\n", globalChanData.processName);
		exit(1);
	}

	/*----Detach from Enc2-Chan Shared Memory----*/
	printf("%s: Detaching from Enc2-Chan Shared Memory Segment!\n", globalChanData.processName);
	if(detachFromEncChanSharedMemory(globalChanData.enc2ChanShmPtr) == -1){
		printf("%s - Shared Memory Detachment failed!\n", globalChanData.processName);
		if(ChanSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
		}
		printf("%s - Exiting...\n", globalChanData.processName);
		exit(1);
	}

	/*----Signal execution Finish----*/
	if(ChanSignalsToSetupFinish() == -1){
		printf("%s: Failed to signal that I finished execution!\n", globalChanData.processName);
	}

	return 0;

}
