#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "enc_tools.h"

extern EncThreadSharedData globalEncData;

/*======================Setup Encoder Process=======================*/

int argumentHandling(int argc, char* argv[]){

	globalEncData.processName = (char*) NULL;
	globalEncData.shmid_appEnc = UNINITIALISED;
	globalEncData.shmid_encChan = UNINITIALISED;
	globalEncData.semid_appEnc = UNINITIALISED;
	globalEncData.semid_encChan = UNINITIALISED;
	globalEncData.semid_setup = UNINITIALISED;

	globalEncData.appEncShmPtr = (SharedMemorySegment*) NULL;
	globalEncData.encChanShmPtr = (SharedMemorySegment*) NULL;

	char c;
	globalEncData.processName = argv[0]; /*Get Process Name to Use when printing*/

	if(argc == 1){
		printf("%s: Please provide SETUP_SEMAPHORE_KEY: ", globalEncData.processName);
		scanf("%d", (int*) &(globalEncData.sem_setupKey));
		c = getchar(); //Get newline
		printf("%s: Please provide APP-ENC_SEMAPHORE_KEY: ", globalEncData.processName);
		scanf("%d", (int*) &(globalEncData.sem_appEnckey));	
		c = getchar(); //Get newline
		printf("%s: Please provide ENC-CHAN_SEMAPHORE_KEY: ", globalEncData.processName);
		scanf("%d", (int*) &(globalEncData.sem_encChankey));	
		c = getchar(); //Get newline
		printf("%s: Please provide APP-ENC_SHARED_MEMORY_KEY: ", globalEncData.processName);
		scanf("%d", (int*) &(globalEncData.shm_appEnckey));
		c = getchar(); //Get newline
		printf("%s: Please provide ENC-CHAN_SHARED_MEMORY_KEY: ", globalEncData.processName);
		scanf("%d", (int*) &(globalEncData.shm_encChankey));
		c = getchar(); //Get newline
		printf("%s: Please provide Enc number (1 or 2)\n", globalEncData.processName);
		scanf("%d", &(globalEncData.processNumber));
		c = getchar(); //Get newline
		if((globalEncData.processNumber != 1) && (globalEncData.processNumber != 2)){
			printf("%s: Encoder Number can be 1 for enc1 or 2 for enc2 only!\n", globalEncData.processName);
			exit(1);
		}	
	}
	else if(argc == 7){
		globalEncData.sem_setupKey = (key_t) atoi(argv[1]); //Get Semaphore key for Setup Semaphores
		globalEncData.sem_appEnckey = (key_t) atoi(argv[2]); //Get Semaphore key for appEnc Semaphores
		globalEncData.sem_encChankey = (key_t) atoi(argv[3]); //Get Semaphore key for encChan Semaphores
		globalEncData.shm_appEnckey = (key_t) atoi(argv[4]); //Get Shared Memory key for AppEnc
		globalEncData.shm_encChankey = (key_t) atoi(argv[5]); //Get Shared Memory key for encChan
		globalEncData.processNumber = atoi(argv[6]);
		if((globalEncData.processNumber != 1) && (globalEncData.processNumber != 2)){
			printf("%s: Enocder Number can be 1 for enc1 or 2 for enc2 only!\n", globalEncData.processName);
			exit(1);
		}
	}
	else{
		printf("%s: Encoder process should be run with only 6 arguments and AFTER setup process only!\n",argv[0]);
		printf("%s: HOW TO RUN: %s [SETUP_SEM_KEY] [APPENC_SEM_KEY] [ENCCHAN_SEM_KEY] [APPENC_SHM_KEY] [ENCCHAN_SHM_KEY] [PROCESS_NUMBER]\n",argv[0],argv[0]);
		printf("%s: Example: %s 54410 24411 24413 34313 34314 1\n",argv[0],argv[0]);
		exit(1);
	}

}

int attachToAppEncSharedMemory(){
	
	printf("%s: Attempting to shm_get and shm_attach to App-Enc Shared Memory Segment... - KEY:%d\n", globalEncData.processName, (int) globalEncData.shm_appEnckey);

	return attachToSharedMemoryArea(globalEncData.shm_appEnckey, &(globalEncData.shmid_appEnc), &(globalEncData.appEncShmPtr));

}

int detachFromAppEncSharedMemory(){

	printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
	return detachSharedMemory(globalEncData.appEncShmPtr);

}

int getAppEncSemaphores(){

	/*----semget----*/
	printf("%s: Attempting to sem_get AppEnc Semaphore Set... - KEY: %d\n", globalEncData.processName, (int) globalEncData.sem_appEnckey);
	return getSemaphores(globalEncData.sem_appEnckey, &(globalEncData.semid_appEnc), NSEMS_APP_ENC);

}

int attachToEncChanSharedMemory(){
	
	printf("%s: Attempting to shm_get and shm_attach to Enc-Chan Shared Memory Segment... - KEY:%d\n", globalEncData.processName, globalEncData.shm_encChankey);

	return attachToSharedMemoryArea(globalEncData.shm_encChankey, &(globalEncData.shmid_encChan), &(globalEncData.encChanShmPtr));

}

int detachFromEncChanSharedMemory(){

	printf("%s: Detaching from Enc-Chan Shared Memory Segment!\n", globalEncData.processName);
	return detachSharedMemory(globalEncData.encChanShmPtr);

}

int getEncChanSemaphores(){

	/*----semget----*/
	printf("%s: Attempting to sem_get EncChan Semaphore Set... - KEY: %d\n", globalEncData.processName, (int) globalEncData.sem_encChankey);
	return getSemaphores(globalEncData.sem_encChankey, &(globalEncData.semid_encChan), NSEMS_ENC_CHAN);

}

int EncWaitsForSetupToFinish(){

	return waitForSetupToFinish(globalEncData.processName,globalEncData.semid_setup,ENC,globalEncData.processNumber);

}

int EncSignalsToSetupFinish(){

	return signalFinish(globalEncData.processName,globalEncData.semid_setup,ENC,globalEncData.processNumber);

}

/*Performs all actions required to get the Encoder Working*/
/*The Encoder first attaches to the Setup Semaphores and uses them to wait until Setup is done*/
/*Then gets the Semaphores and attaches to Shared Memory Segments and deployes a Listener Thread and a Talker Thread(see below)*/

int encSetupOperations(){

	/*----Get Setup Semaphores----*/ 
	if(getSetupSemaphores(globalEncData.processName, globalEncData.sem_setupKey, &(globalEncData.semid_setup)) == -1){
		printf("%s: Failed to get Setup Semaphores!\n", globalEncData.processName);
		exit(1);
	}

	/*----Initialise Malloc and Free Mutexes----*/
	if(pthread_mutex_init(&(globalEncData.malloc_mtx), NULL) != 0){
		printf("%s: Failed to initialise Malloc mutex!\n", globalEncData.processName);
		exit(1);		
	}

	if(pthread_mutex_init(&(globalEncData.free_mtx), NULL) != 0){
		printf("%s: Failed to initialise Free mutex!\n", globalEncData.processName);
		pthread_mutex_destroy(&(globalEncData.malloc_mtx));
		exit(1);		
	}

	/*----Wait for Setup to finish----*/
	if(EncWaitsForSetupToFinish() == -1){
		printf("%s: Failed to DOWN Setup Start Semaphore!\n", globalEncData.processName);
		printf("%s: Exiting...\n" , globalEncData.processName);
		exit(1);
	}

	/*----Attaching to App-Enc Shared Memory----*/
	if(attachToAppEncSharedMemory() == -1){
		printf("%s: Failed at Shared Memory shmget or shmattach (APP-ENC)!\n", globalEncData.processName);
		if(EncWaitsForSetupToFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
		}
		printf("%s: Exiting...\n" , globalEncData.processName);
		exit(1);
	}

	/*----Get App-Enc Semaphore Set----*/
	if(getAppEncSemaphores() == -1){
		printf("%s: Failed to get App-Enc Semaphore get!\n", globalEncData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		if(EncWaitsForSetupToFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
		}
		printf("%s: Exiting...\n" , globalEncData.processName);
		exit(1);
	}

	/*----Attaching to Enc-Chan Shared Memory----*/
	if(attachToEncChanSharedMemory() == -1){
		printf("%s: Failed at Shared Memory shmget or shmattach (ENC-CHAN)!\n", globalEncData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		if(EncWaitsForSetupToFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
		}
		printf("%s: Exiting...\n" , globalEncData.processName);
		exit(1);
	}

	/*----Get Enc-Chan Semaphore Set----*/
	if(getEncChanSemaphores() == -1){
		printf("%s: Failed to get Enc-Chan Semaphore get!\n", globalEncData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		printf("%s: Detaching from Enc-Chan Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromEncChanSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		if(EncWaitsForSetupToFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
		}
		printf("%s: Exiting...\n" , globalEncData.processName);
		exit(1);
	}

	/*----Initialise termFlag Mutex----*/
	globalEncData.termFlag = OFF; //Flag that signals termination for threads

	if(pthread_mutex_init(&(globalEncData.termFlagLock), NULL) != 0){
		printf("%s: Failed to initialise termFlag mutex!\n", globalEncData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		printf("%s: Detaching from Enc-Chan Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromEncChanSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		if(EncWaitsForSetupToFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
		}
		printf("%s: Exiting improperly!\n" , globalEncData.processName);
		exit(1);		
	}

	/*----Initialise retryFlag Mutex----*/
	globalEncData.retryFlag = OFF; //Flag that Listener Thread uses to tell Talker to resend

	if(pthread_mutex_init(&(globalEncData.retryFlagLock), NULL) != 0){
		printf("%s: Failed to initialise retryFlag mutex!\n", globalEncData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		printf("%s: Detaching from Enc-Chan Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromEncChanSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		if(pthread_mutex_destroy(&(globalEncData.termFlagLock)) != 0){
			printf("%s - Failed to destroy termFlagLock!\n", globalEncData.processName);
		}
		if(EncWaitsForSetupToFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
		}
		printf("%s: Exiting improperly!\n" , globalEncData.processName);
		exit(1);		
	}

	/*----Deploy Input and Output Threads----*/

	if(pthread_create(&(globalEncData.threadIDs[0]), NULL, ListenerThread, NULL) != 0){ //Lists for new messages from channel
		printf("%s: Failed to create InputWorker thread!\n", globalEncData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		printf("%s: Detaching from Enc-Chan Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromEncChanSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		printf("%s: Destroying termFlag Mutex...\n", globalEncData.processName);
		if(pthread_mutex_destroy(&(globalEncData.termFlagLock)) != 0){
			printf("%s: Failed to destroy termFlag mutex!\n", globalEncData.processName);
		}
		printf("%s: Destroying retryFlag Mutex...\n", globalEncData.processName);
		if(pthread_mutex_destroy(&(globalEncData.retryFlagLock)) != 0){
			printf("%s: Failed to destroy retryFlag mutex!\n", globalEncData.processName);
		}		
		if(EncWaitsForSetupToFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
		}
		printf("%s: Exiting...\n" , globalEncData.processName);
		exit(1);
	}
	
	if(pthread_create(&(globalEncData.threadIDs[1]), NULL, TalkerThread, NULL) != 0){ //Passes new messages from Application to channel
		printf("%s: Failed to create OutputWorker thread!\n", globalEncData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		printf("%s: Detaching from Enc-Chan Shared Memory Segment!\n", globalEncData.processName);
		if(detachFromEncChanSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
		}
		printf("%s: Destroying termFlag Mutex...\n", globalEncData.processName);
		if(pthread_mutex_destroy(&(globalEncData.termFlagLock)) != 0){
			printf("%s: Failed to destroy termFlag mutex!\n", globalEncData.processName);
		}
		printf("%s: Destroying retryFlag Mutex...\n", globalEncData.processName);
		if(pthread_mutex_destroy(&(globalEncData.retryFlagLock)) != 0){
			printf("%s: Failed to destroy retryFlag mutex!\n", globalEncData.processName);
		}		
		if(EncWaitsForSetupToFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
		}
		printf("%s: Exiting...\n" , globalEncData.processName);
		exit(1);
	}

	return 0;

}

int encTearDownOperations(){ /*----Free any resources allocated by Encoder and its threads while detaching from Shared Memories----*/

	int* exit_valPtr = (int*) NULL;

	/*----Wait for Threads to Finish----*/
	printf("%s: Waiting for threads to terminate...\n", globalEncData.processName);

	pthread_join(globalEncData.threadIDs[0], (void**) &exit_valPtr);
	printf("%s: ListenerThread terminated with Exit_Code: %d\n", globalEncData.processName, *exit_valPtr);
	if(exit_valPtr != NULL) free(exit_valPtr);

	pthread_join(globalEncData.threadIDs[1], (void**) &exit_valPtr);
	printf("%s: TalkerThread terminated with Exit_Code: %d\n", globalEncData.processName, *exit_valPtr);
	if(exit_valPtr != NULL) free(exit_valPtr);

	/*----Destroying termFlag Mutex----*/
	printf("%s: Destroying termFlag Mutex...\n", globalEncData.processName);
	if(pthread_mutex_destroy(&(globalEncData.termFlagLock)) != 0){
		printf("%s: Failed to destroy termFlag mutex!\n", globalEncData.processName);
	}

	/*----Destroying Malloc/Free mutexes----*/
	if(pthread_mutex_destroy(&(globalEncData.malloc_mtx)) == -1){
		printf("%s: Failed to destroy Malloc mutex!\n", globalEncData.processName);
	}
	
	if(pthread_mutex_destroy(&(globalEncData.free_mtx)) == -1){
		printf("%s: Failed to destroy Free mutex!\n", globalEncData.processName);
	}

	/*----Destroying retryFlag Mutex----*/
	printf("%s: Destroying retryFlag Mutex...\n", globalEncData.processName);
	if(pthread_mutex_destroy(&(globalEncData.retryFlagLock)) != 0){
		printf("%s: Failed to destroy retryFlag mutex!\n", globalEncData.processName);
	}		

	/*----Detach from App-Enc Shared Memory----*/
	printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalEncData.processName);
	if(detachFromAppEncSharedMemory() == -1){
		printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
	}

	/*----Detach from Enc-Chan Shared Memory----*/
	printf("%s: Detaching from Enc-Chan Shared Memory Segment!\n", globalEncData.processName);
	if(detachFromEncChanSharedMemory() == -1){
		printf("%s - Shared Memory Detachment failed!\n", globalEncData.processName);
	}

	/*----Signal execution Finish----*/
	if(EncSignalsToSetupFinish() == -1){
		printf("%s: Failed to signal that I finished execution!\n", globalEncData.processName);
	}

	return 0;

}

/*======================Encoder Thread functions======================*/

int waitForConfirmation(){ /*When a TalkerThread(see below) sends a Message it waits for a Confirmation for it to know if it needs to be resent*/
	
	int return_value = -1;
	struct sembuf semaphoreOperation;

	printf("%s-Talker: Waiting for Confirmation from ListenerThread...\n", globalEncData.processName);
	fflush(stdout);

	/*-----Wait here until the Listener passes me the Confirmation for my message----*/
	semaphoreOperation.sem_num = CONFIRM_SEM; /*Confirmation Semaphore*/
	semaphoreOperation.sem_op = -1;
	semaphoreOperation.sem_flg = 0;

	if(semop(globalEncData.semid_encChan, &semaphoreOperation, 1) == -1){ /*DOWN*/
		printf("%s-Talker: waitForConfirmation: Failed to DOWN Confirmation Semaphore!\n", globalEncData.processName);
		fflush(stdout);
		exit(1);
	}	

	/*-----Confirmation has arrived-----*/

	pthread_mutex_lock(&(globalEncData.retryFlagLock));
	if(globalEncData.retryFlag == RESEND){ /*Notified from Listener that I must RESEND*/
		return_value = RESEND;
	}
	else{ /*Notified from Listener that my Message has arrived successfully - OK*/
		return_value = OK;
	}
	globalEncData.retryFlag = OFF;
	pthread_mutex_unlock(&(globalEncData.retryFlagLock));

	pthread_mutex_lock(&(globalEncData.termFlagLock));
	if(globalEncData.termFlag == TERM){ /*----TERM notification arrives instead of confirmation----*/
		printf("\n%s-Talker: TERM Message Arrived from Listener instead of Confirmation!\n", globalEncData.processName);
		fflush(stdout);
		return_value = TERM;
	}
	pthread_mutex_unlock(&(globalEncData.termFlagLock));

	return return_value;

}

void* TalkerThread(){ /*----The Thread that encodes our application's messages and sends them----*/
					  /*----This thread adds a Hash Value to a new Message from the Application it serves----*/
					  /*----It sends the message and waits for Confirmation to be send from Listener Thread----*/
					  /*----If the Listener thread informs the TalkerThread that the message must be resend it will resend it until OK is received----*/

	MessagePacket* packet_ptr = (MessagePacket*) NULL;
	int retryValue = OFF;
	int confirmationResult = OFF;

	printf("\n%s-TalkerThread (%ld): Hello from TalkerThread!\n", globalEncData.processName, pthread_self());
	fflush(stdout);

	while(globalEncData.termFlag == OFF){
		printf("\n%s-Talker: Waiting for Message from Application Process...\n", globalEncData.processName);
		fflush(stdout);

		/*----Take Message from Application----*/
		packet_ptr = takeFromSharedBuffer("TalkerThread",&(globalEncData.appEncShmPtr->inputQueue),globalEncData.semid_appEnc,INPUT_SEM_MUTEX,INPUT_SEM_EMPTY,INPUT_SEM_FULL,&(globalEncData.termFlag), &(globalEncData.malloc_mtx), &(globalEncData.termFlagLock));

		if(packet_ptr != NULL){
			printf("\n%s-Talker: Message from Application arrived!\n", globalEncData.processName);
			fflush(stdout);
			if(packet_ptr->termFlag == TERM){
				printf("\n%s-Talker: TERM Message from my Application received! Sending it and notifying Listener!\n", globalEncData.processName);
				fflush(stdout);
			}
			else{
				printf("\n%s-Talker: Adding Hash Value to MessagePacket...\n", globalEncData.processName);
				fflush(stdout);
				addHashValueToMessage(packet_ptr);
			}

			do{ /*-----Before proceeding to next Packet from Application send the current one until it reaches properly-----*/
				printf("\n%s-Talker: Sending Packet...\n", globalEncData.processName);
				fflush(stdout);

				/*----Send Message to Channel----*/
				addToSharedBuffer("TalkerThread",packet_ptr,&(globalEncData.encChanShmPtr->inputQueue),globalEncData.semid_encChan,INPUT_SEM_MUTEX,INPUT_SEM_EMPTY,INPUT_SEM_FULL, &(globalEncData.termFlag), &(globalEncData.malloc_mtx), &(globalEncData.termFlagLock));

				if(packet_ptr->termFlag == TERM){ /*I sent a TERM Message - Notifying Listener and Exiting*/
					sigFinishToOther(globalEncData.processName, &(globalEncData.termFlagLock), &(globalEncData.termFlag), globalEncData.semid_encChan, OUTPUT_SEM_FULL);
					sigFinishToOther(globalEncData.processName, &(globalEncData.termFlagLock), &(globalEncData.termFlag), globalEncData.semid_encChan, INPUT_SEM_EMPTY);						
					break;
				}

				/*----Wait for information from Listener Thread about if our Message Arrived OK or not----*/
				confirmationResult = OFF;
				confirmationResult = waitForConfirmation();

				if(confirmationResult == RESEND){
					printf("\n%s-Talker: Message FAILED to arrive successfully! Will attempt to send again...\n", globalEncData.processName);
					fflush(stdout);
				}
				else if(confirmationResult == OK){
					printf("\n%s-Talker: Message Arrived Successfully!\n", globalEncData.processName);
					fflush(stdout);
					break;
				}
				else if(confirmationResult == TERM){
					break;
				}

			}while(1);

			/*----Free allocated Packet----*/
			pthread_mutex_lock(&(globalEncData.free_mtx));
			free(packet_ptr);
			pthread_mutex_unlock(&(globalEncData.free_mtx));
			packet_ptr = (MessagePacket*) NULL;

		}
		else{ /*----Packet from Application is NULL----*/
			if(globalEncData.termFlag == TERM){ /*----Packet is NULL because we are exiting!----*/
				printf("\n%s-Talker: Notified of TERM from ListenThread!\n", globalEncData.processName);
				fflush(stdout);
			}
			else{
				printf("\n%s-Talker: Packet Pointer is NULL for unknown reasons!\n", globalEncData.processName);
				fflush(stdout);
			}
		}
	}

	printf("\n%s-TalkerThread (%ld): TERM Value Set! Goodbye from TalkerThread!\n", globalEncData.processName, pthread_self());
	fflush(stdout);

	int* exit_valPtr = malloc(sizeof(int));
	*exit_valPtr = 10;
	pthread_exit((void*) exit_valPtr);

}

int confirmationForTalker(MessagePacket* packet_ptr){ /*----Used by ListenerThread to inform TalkerThread about what happened to its message----*/

	pthread_mutex_lock(&(globalEncData.retryFlagLock));
	globalEncData.retryFlag = packet_ptr->retryFlag;
	pthread_mutex_unlock(&(globalEncData.retryFlagLock));

	struct sembuf semaphoreOperation;

	printf("%s-Listen: Received Special Confirmation Message! Confirmation Type: |%s| - Pass to Talker...\n", globalEncData.processName, packet_ptr->message_buffer);
	fflush(stdout);

	semaphoreOperation.sem_num = 6;
	semaphoreOperation.sem_op = 1;
	semaphoreOperation.sem_flg = 0;

	if(semop(globalEncData.semid_encChan, &semaphoreOperation, 1) == -1){
		printf("%s-Listen: confirmationForTalker: Failed to UP Confirmation Semaphore!\n", globalEncData.processName);
		fflush(stdout);
		exit(1);
	}	

	return 0;

}

void* ListenerThread(){ /*----The Listener Thread Received Messages from The Channel----*/
						/*----These Messages can be Normal Messages, OK/Resend Special Messages or TERM Messages----*/
						/*----If A Normal Message has arrived then the ListenerThread passes it through MD5 to check it has not been corrupted----*/
						/*----Then it SENDS OK through the Channel if it has not been corrupted or RESEND if it has been corrupted----*/
						/*----NOTE: I chose to have the Listener Thread send OK and not the TALKER Thread in order to simplify synchronisation between the 2----*/
						/*----If a Special RetryFlag Message has arrived then this message must be passed to our TalkerThread and notify them to either----*/
						/*----Resend the message or not----*/

	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	printf("\n%s-ListenerThread (%ld): Hello from ListenerThread!\n", globalEncData.processName, pthread_self());
	fflush(stdout);

	while(globalEncData.termFlag == OFF){
		printf("\n%s-Listen: Waiting for Message from Channel process...\n", globalEncData.processName);
		fflush(stdout);

		/*----Wait for Packets from the Channel----*/
		packet_ptr = takeFromSharedBuffer("Listener",&(globalEncData.encChanShmPtr->outputQueue),globalEncData.semid_encChan,OUTPUT_SEM_MUTEX,OUTPUT_SEM_EMPTY,OUTPUT_SEM_FULL, &(globalEncData.termFlag), &(globalEncData.malloc_mtx), &(globalEncData.termFlagLock));

		if(packet_ptr != NULL){ /*----Packet arrived----*/

			if(packet_ptr->termFlag == TERM){ /*----Received TERM Message - Pass to my Application and notify the other Thread to stop----*/
				printf("\n%s-Listen: Received TERM Message! Passing to Application and Notifying Talker Thread to stop!\n", globalEncData.processName);
				fflush(stdout);	

				/*----Pass Message to Application----*/
				addToSharedBuffer("Listener",packet_ptr,&(globalEncData.appEncShmPtr->outputQueue),globalEncData.semid_appEnc,OUTPUT_SEM_MUTEX,OUTPUT_SEM_EMPTY,OUTPUT_SEM_FULL, &(globalEncData.termFlag), &(globalEncData.malloc_mtx), &(globalEncData.termFlagLock));	
			
				if(globalEncData.termFlag == TERM){
					printf("\n%s-Listen: Received TERM Notification Before Passing my own TERM Notification!\n", globalEncData.processName);
					fflush(stdout);
				}

				/*----Signal TalkerThread of Exiting----*/
				sigFinishToOther(globalEncData.processName, &(globalEncData.termFlagLock), &(globalEncData.termFlag), globalEncData.semid_appEnc, INPUT_SEM_FULL);
				sigFinishToOther(globalEncData.processName, &(globalEncData.termFlagLock), &(globalEncData.termFlag), globalEncData.semid_encChan, INPUT_SEM_EMPTY);
				sigFinishToOther(globalEncData.processName, &(globalEncData.termFlagLock), &(globalEncData.termFlag), globalEncData.semid_encChan, CONFIRM_SEM);

			}
			else{ /*----Received non-TERM Message----*/
				if(packet_ptr->retryFlag != OFF){ /*----OK or RESEND Flag Message----*/
					confirmationForTalker(packet_ptr);
				}
				else{ /*----Normal Message - Must Check for its integrity!----*/
					printf("\n%s-Listen: Received (NORMAL): |%s|...\n", globalEncData.processName, packet_ptr->message_buffer);
					fflush(stdout);

					if(checkMessageIntegrity(*packet_ptr) == 1){ /*---Message has not been corrupted!---*/
						printf("\n%s-Listen: Message Integrity has been kept! Sending OK Packet!\n", globalEncData.processName);
						fflush(stdout);
						printf("\n%s-Listen: Passing this Message to Application Process!\n", globalEncData.processName);
						fflush(stdout);

						/*----Passing message to Application----*/
						addToSharedBuffer("Listener",packet_ptr,&(globalEncData.appEncShmPtr->outputQueue),globalEncData.semid_appEnc,OUTPUT_SEM_MUTEX,OUTPUT_SEM_EMPTY,OUTPUT_SEM_FULL, &(globalEncData.termFlag), &(globalEncData.malloc_mtx), &(globalEncData.termFlagLock));	

						/*----Now sending OK Message to the other Encoder----*/
						clear_string_buffer(packet_ptr->message_buffer, MSG_SIZE);
						clear_string_buffer(packet_ptr->md5_hash, 34);
						strcpy(packet_ptr->message_buffer, "OK");
						packet_ptr->retryFlag = OK;

						addToSharedBuffer("Listener",packet_ptr,&(globalEncData.encChanShmPtr->inputQueue),globalEncData.semid_encChan,INPUT_SEM_MUTEX,INPUT_SEM_EMPTY,INPUT_SEM_FULL,&(globalEncData.termFlag), &(globalEncData.malloc_mtx), &(globalEncData.termFlagLock));		

					}
					else{ /*----Message has been corrupted----*/
						printf("\n%s-Listen: Message Integrity has been compromised! Sending Retry Packet!\n", globalEncData.processName);
						fflush(stdout);

						/*----Sending RESEND Message to the other Encoder----*/
						clear_string_buffer(packet_ptr->message_buffer, MSG_SIZE);
						clear_string_buffer(packet_ptr->md5_hash, 34);
						strcpy(packet_ptr->message_buffer, "RESEND");
						packet_ptr->retryFlag = RESEND;

						addToSharedBuffer("Listener",packet_ptr,&(globalEncData.encChanShmPtr->inputQueue),globalEncData.semid_encChan,INPUT_SEM_MUTEX,INPUT_SEM_EMPTY,INPUT_SEM_FULL,&(globalEncData.termFlag), &(globalEncData.malloc_mtx), &(globalEncData.termFlagLock));

					}

				}
			}
			
			/*----Free the allocated Packet----*/
			pthread_mutex_lock(&(globalEncData.free_mtx));
			free(packet_ptr);
			pthread_mutex_unlock(&(globalEncData.free_mtx));
			packet_ptr = (MessagePacket*) NULL;

		}	
		else{ /*----Packet which arrived is NULL----*/
			if(globalEncData.termFlag == TERM){ /*----Received NULL Packet because we are exiting----*/
				printf("\n%s-Listen: Notified of TERM from TalkerThread!\n", globalEncData.processName);
				fflush(stdout);
			}
			else{ /*----Error case----*/
				printf("\n%s-Listen: Packet Pointer is NULL for unknown reasons!\n", globalEncData.processName);
				fflush(stdout);
			}
		}
	}

	printf("\n%s-ListenThread (%ld): TERM Value Set! Goodbye from ListenThread!\n", globalEncData.processName, pthread_self());
	fflush(stdout);

	int* exit_valPtr = malloc(sizeof(int));
	*exit_valPtr = 11;
	pthread_exit((void*) exit_valPtr);

}

/*===================================MD5 HASH FUNCTIONS=======================================*/

int addHashValueToMessage(MessagePacket* packet_ptr){ /*Uses OpenSSL MD5 Hash Algorithm to produce a Hash Value for a Message*/

	char hashString[16];
	char mdString[34];
	int i=0;

	printf("%s-TALKER: Passing message through MD5 Hash function...\n", globalEncData.processName);
	fflush(stdout);

	MD5(packet_ptr->message_buffer, strlen(packet_ptr->message_buffer) + 1, hashString); /*Produces Hexademical Hash Value*/

	clear_string_buffer(mdString, 34);

    for (i = 0; i < 16; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)hashString[i]); /*Copy Hexademical Hash Value in String Form*/

	mdString[33] = '\0';

	printf("%s-TALKER: Adding hash value: |%s| to message: |%s|)\n", globalEncData.processName, mdString, packet_ptr->message_buffer);
	fflush(stdout);

	strcpy(packet_ptr->md5_hash, mdString); /*Add Hash Value to Message Packet*/

	return 0;

}

int checkMessageIntegrity(MessagePacket packet){ 	/*Uses MD5 Hash Function again to produce Hash of given Message and compare with Hash Value stored in the Packet*/
													/*Returns 0 if Message has been found corrupted or 1 if not*/

	char tmp[MSG_SIZE];
	char hashString[16];
	char mdString[34];
	int i=0;

	printf("%s-LISTEN: Checking Message integrity...\n", globalEncData.processName);
	fflush(stdout);
	strcpy(tmp, packet.message_buffer);
	
	MD5(tmp, strlen(tmp) + 1, hashString);

	clear_string_buffer(mdString, 34);
 
    for (i = 0; i < 16; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)hashString[i]);

	mdString[33] = '\0';

	printf("%s-LISTEN: Comparing hash values...(%s)-(%s)\n", globalEncData.processName, mdString, packet.md5_hash);
	fflush(stdout);

	if(!strcmp(mdString, packet.md5_hash)){
		printf("%s-LISTEN: The values match! Message is fine!\n", globalEncData.processName);
		fflush(stdout);
		return 1;
	}
	else{
		printf("%s-LISTEN: The values don't match! The message must be resent!\n", globalEncData.processName);
		fflush(stdout);
		return 0;
	}

}
