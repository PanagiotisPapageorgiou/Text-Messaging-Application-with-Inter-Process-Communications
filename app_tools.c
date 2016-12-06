#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "app_tools.h"

extern AppThreadSharedData globalAppData;

/*======================Setup Application Process=======================*/

int argumentHandling(int argc, char* argv[]){

	globalAppData.processName = (char*) NULL;
	globalAppData.shmid_appEnc = UNINITIALISED;
	globalAppData.semid_appEnc = UNINITIALISED;
	globalAppData.semid_setup = UNINITIALISED;

	globalAppData.appEncShmPtr = (SharedMemorySegment*) NULL;

	char c;
	globalAppData.processName = argv[0]; //Get Process Name to Use when printing

	if(argc == 1){
		printf("%s: Please provide SETUP_SEMAPHORE_KEY: ", globalAppData.processName);
		scanf("%d", (int*) &(globalAppData.sem_setupKey));
		c = getchar(); //Get newline
		printf("%s: Please provide APP-ENC_SEMAPHORE_KEY: ", globalAppData.processName);
		scanf("%d", (int*) &(globalAppData.sem_appEnckey));	
		c = getchar(); //Get newline
		printf("%s: Please provide APP-ENC_SHARED_MEMORY_KEY: ", globalAppData.processName);
		scanf("%d", (int*) &(globalAppData.shm_appEnckey));
		c = getchar(); //Get newline
		printf("%s: Please provide App number (1 or 2)\n", globalAppData.processName);
		scanf("%d", &(globalAppData.processNumber));
		c = getchar(); //Get newline
		if((globalAppData.processNumber != 1) && (globalAppData.processNumber != 2)){
			printf("%s: App Number can be 1 for p1 or 2 for p2 only!\n", globalAppData.processName);
			exit(1);
		}	
	}
	else if(argc == 5){
		globalAppData.sem_setupKey = (key_t) atoi(argv[1]); //Get Semaphore key for Setup Semaphores
		globalAppData.sem_appEnckey = (key_t) atoi(argv[2]); //Get Semaphore key for appEnc Semaphores
		globalAppData.shm_appEnckey = (key_t) atoi(argv[3]); //Get Shared Memory key
		globalAppData.processNumber = atoi(argv[4]);
		if((globalAppData.processNumber != 1) && (globalAppData.processNumber != 2)){
			printf("%s: App Number can be 1 for p1 or 2 for p2 only!\n", globalAppData.processName);
			exit(1);
		}
	}
	else{
		printf("%s: Application process should be run with only 4 arguments and AFTER setup process only!\n",argv[0]);
		printf("%s: HOW TO RUN: %s [SETUP_SEM_KEY] [APPENC_SEM_KEY] [APPENC_SHM_KEY] [PROCESS_NUMBER]\n",argv[0],argv[0]);
		printf("%s: Example: %s 54410 24411 34313 1\n",argv[0],argv[0]);
		exit(1);
	}

}

int attachToAppEncSharedMemory(){
	
	printf("%s: Attempting to shm_get and shm_attach to App-Enc Shared Memory Segment... - KEY:%d\n", globalAppData.processName, (int) globalAppData.shm_appEnckey);
	return attachToSharedMemoryArea(globalAppData.shm_appEnckey, &(globalAppData.shmid_appEnc), &(globalAppData.appEncShmPtr));

}

int detachFromAppEncSharedMemory(){

	printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalAppData.processName);
	return detachSharedMemory(globalAppData.appEncShmPtr);

}

int getAppEncSemaphores(){

	/*----semget----*/
	printf("%s: Attempting to sem_get AppEnc Semaphore Set... - KEY: %d\n", globalAppData.processName, (int) globalAppData.sem_appEnckey);
	return getSemaphores(globalAppData.sem_appEnckey, &(globalAppData.semid_appEnc), NSEMS_APP_ENC);

}

int AppWaitsForSetupToFinish(){

	return waitForSetupToFinish(globalAppData.processName,globalAppData.semid_setup,APP,globalAppData.processNumber);

}

int AppSignalsToSetupFinish(){

	return signalFinish(globalAppData.processName,globalAppData.semid_setup,APP,globalAppData.processNumber);

}

int appSetupOperations(){

	/*----Get Setup Semaphores----*/ 
	if(getSetupSemaphores(globalAppData.processName, globalAppData.sem_setupKey, &(globalAppData.semid_setup)) == -1){
		printf("%s: Failed to get Setup Semaphores!\n", globalAppData.processName);
		exit(1);
	}

	/*----Initialise Malloc and Free Mutexes----*/
	if(pthread_mutex_init(&(globalAppData.malloc_mtx), NULL) != 0){
		printf("%s: Failed to initialise Malloc mutex!\n", globalAppData.processName);
		exit(1);		
	}

	if(pthread_mutex_init(&(globalAppData.free_mtx), NULL) != 0){
		printf("%s: Failed to initialise Free mutex!\n", globalAppData.processName);
		pthread_mutex_destroy(&(globalAppData.malloc_mtx));
		exit(1);		
	}

	/*----Wait for Setup to finish----*/
	if(AppWaitsForSetupToFinish() == -1){
		printf("%s: Failed to DOWN Setup Start Semaphore!\n", globalAppData.processName);
		printf("%s: Exiting...\n" , globalAppData.processName);
		exit(1);
	}

	/*----Attaching to App-Enc Shared Memory----*/
	if(attachToAppEncSharedMemory() == -1){
		printf("%s: Failed at Shared Memory shmget or shmattach!\n", globalAppData.processName);
		if(AppSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalAppData.processName);
		}
		printf("%s: Exiting...\n" , globalAppData.processName);
		exit(1);
	}

	/*----Get App-Enc Semaphore Set----*/
	if(getAppEncSemaphores() == -1){
		printf("%s: Failed to get App-Enc Semaphore get!\n", globalAppData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalAppData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalAppData.processName);
		}
		if(AppSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalAppData.processName);
		}
		printf("%s: Exiting...\n" , globalAppData.processName);
		exit(1);
	}

	/*----Initialise termFlag Mutex----*/
	if(pthread_mutex_init(&(globalAppData.termFlagLock), NULL) != 0){
		printf("%s: Failed to initialise termFlag mutex!\n", globalAppData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalAppData.processName);
		}
		if(AppSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalAppData.processName);
		}
		printf("%s: Exiting improperly!\n" , globalAppData.processName);
		exit(1);		
	}

	/*----Deploy Input and Output Threads----*/

	globalAppData.termFlag = OFF; //Flag that signals termination for threads

	if(pthread_create(&(globalAppData.threadIDs[0]), NULL, InputWorker, NULL) != 0){ //Input Thread
		printf("%s: Failed to create InputWorker thread!\n", globalAppData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalAppData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalAppData.processName);
		}
		printf("%s: Destroying termFlag Mutex...\n", globalAppData.processName);
		if(pthread_mutex_destroy(&(globalAppData.termFlagLock)) != 0){
			printf("%s: Failed to destroy termFlag mutex!\n", globalAppData.processName);
		}	
		if(AppSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalAppData.processName);
		}
		printf("%s: Exiting...\n" , globalAppData.processName);
		exit(1);
	}
	
	if(pthread_create(&(globalAppData.threadIDs[1]), NULL, OutputWorker, NULL) != 0){ //Output Thread
		printf("%s: Failed to create OutputWorker thread!\n", globalAppData.processName);
		printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalAppData.processName);
		if(detachFromAppEncSharedMemory() == -1){
			printf("%s - Shared Memory Detachment failed!\n", globalAppData.processName);
		}
		printf("%s: Destroying termFlag Mutex...\n", globalAppData.processName);
		if(pthread_mutex_destroy(&(globalAppData.termFlagLock)) != 0){
			printf("%s: Failed to destroy termFlag mutex!\n", globalAppData.processName);
		}	
		if(AppSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalAppData.processName);
		}
		printf("%s: Exiting...\n" , globalAppData.processName);
		exit(1);	
	}


	return 0;

}

int appTearDownOperations(){

	int* exit_valPtr = (int*) NULL;

	/*----Wait for Threads to Finish----*/
	printf("%s: Waiting for threads to terminate...\n", globalAppData.processName);

	pthread_join(globalAppData.threadIDs[0], (void**) &exit_valPtr);
	printf("%s: InputWorker terminated with Exit_Code: %d\n", globalAppData.processName, *exit_valPtr);
	if(exit_valPtr != NULL) free(exit_valPtr);

	pthread_join(globalAppData.threadIDs[1], (void**) &exit_valPtr);
	printf("%s: OutputWorker terminated with Exit_Code: %d\n", globalAppData.processName, *exit_valPtr);
	if(exit_valPtr != NULL) free(exit_valPtr);

	/*----Destroying termFlag Mutex----*/
	printf("%s: Destroying termFlag Mutex...\n", globalAppData.processName);
	if(pthread_mutex_destroy(&(globalAppData.termFlagLock)) != 0){
		printf("%s: Failed to destroy termFlag mutex!\n", globalAppData.processName);
	}	

	/*----Destroying Malloc/Free mutexes----*/
	if(pthread_mutex_destroy(&(globalAppData.malloc_mtx)) == -1){
		printf("%s: Failed to destroy Malloc mutex!\n", globalAppData.processName);
	}
	
	if(pthread_mutex_destroy(&(globalAppData.free_mtx)) == -1){
		printf("%s: Failed to destroy Free mutex!\n", globalAppData.processName);
	}

	/*----Detach from App-Enc Shared Memory----*/
	printf("%s: Detaching from App-Enc Shared Memory Segment!\n", globalAppData.processName);
	if(detachFromAppEncSharedMemory() == -1){
		printf("%s - Shared Memory Detachment failed!\n", globalAppData.processName);
		if(AppSignalsToSetupFinish() == -1){
			printf("%s: Failed to signal that I finished execution!\n", globalAppData.processName);
		}
		printf("%s - Exiting...\n",globalAppData.processName);
		exit(1);
	}

	/*----Signal execution Finish----*/
	if(AppSignalsToSetupFinish() == -1){
		printf("%s: Failed to signal that I finished execution!\n", globalAppData.processName);
	}

	return 0;

}

/*======================Application Thread functions======================*/

void* InputWorker(){ /*----The Worker Thread that sends our messages (PRODUCER)----*/

	char input_msg[MSG_SIZE];
	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	printf("\n%s-InputThread (%ld): Hello from InputWorker!\n", globalAppData.processName, pthread_self());
	fflush(stdout);

	while(globalAppData.termFlag == OFF){
		clear_string_buffer(input_msg , MSG_SIZE);
		message_input(input_msg); /*---Receive input from user---*/

		if(globalAppData.termFlag == TERM){ /*---TERM Message has already arrived from the other application---*/
			printf("\n%s-IN: Received TERM notification from OutPut Thread! Exiting!\n", globalAppData.processName);
			fflush(stdout);
			break;
		}

		if(!strcmp(input_msg, "TERM")){ /*---My user just typed TERM---*/

			/*---Create TERM Packet---*/
			pthread_mutex_lock(&(globalAppData.malloc_mtx));
			packet_ptr = createPacket(input_msg, NULL, OFF, TERM);
			pthread_mutex_unlock(&(globalAppData.malloc_mtx));

			/*---Pass TERM Message---*/
			addToSharedBuffer("InputWorker",packet_ptr,&(globalAppData.appEncShmPtr->inputQueue),globalAppData.semid_appEnc,INPUT_SEM_MUTEX,INPUT_SEM_EMPTY,INPUT_SEM_FULL,&(globalAppData.termFlag), &(globalAppData.malloc_mtx), &(globalAppData.termFlagLock));
			printf("\n%s-IN: Sending a TERM Message! Signaling to OUT Thread and stopping execution!\n", globalAppData.processName);
			fflush(stdout);
		
			/*---Notify OutputThread to Stop---*/
			sigFinishToOther(globalAppData.processName,&(globalAppData.termFlagLock), &(globalAppData.termFlag), globalAppData.semid_appEnc, OUTPUT_SEM_FULL);

		}
		else{

			/*---Create Normal Packet---*/
			pthread_mutex_lock(&(globalAppData.malloc_mtx));
			packet_ptr = createPacket(input_msg, NULL, OFF, OFF); //Creates a MessagePacket with this message, no hash value and retryFlag set to OFF
			pthread_mutex_unlock(&(globalAppData.malloc_mtx));

			/*----Pass Message----*/
			addToSharedBuffer("InputWorker",packet_ptr,&(globalAppData.appEncShmPtr->inputQueue),globalAppData.semid_appEnc,INPUT_SEM_MUTEX,INPUT_SEM_EMPTY,INPUT_SEM_FULL,&(globalAppData.termFlag), &(globalAppData.malloc_mtx), &(globalAppData.termFlagLock));

			if(globalAppData.termFlag == TERM){ /*---TERM Message Arrived while adding to Buffer---*/
				printf("\n%s-IN: Received TERM from Output Thread while in addToSharedBuffer! Exiting!\n", globalAppData.processName);
				fflush(stdout);
			}

		}
		
		/*----Free Packet allocated Memory----*/
		pthread_mutex_lock(&(globalAppData.free_mtx));
		free(packet_ptr);
		pthread_mutex_unlock(&(globalAppData.free_mtx));
		packet_ptr = (MessagePacket*) NULL;

	}

	printf("\n%s-InputThread (%ld): TERM Value set! Goodbye from Input Worker!\n", globalAppData.processName, pthread_self());
	fflush(stdout);

	int* exit_valPtr = malloc(sizeof(int));
	*exit_valPtr = 10;
	pthread_exit((void*) exit_valPtr);

}

void* OutputWorker(){ /*----The Worker Thread that receives messages (CONSUMER)----*/

	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	printf("\n%s-OutputThread (%ld): Hello from OutputWorker!\n", globalAppData.processName, pthread_self());
	fflush(stdout);

	while(globalAppData.termFlag == OFF){
		printf("\n%s-OUT: Waiting for Message from opposite Application...\n", globalAppData.processName);
		fflush(stdout);
		packet_ptr = takeFromSharedBuffer("OutputWorker",&(globalAppData.appEncShmPtr->outputQueue),globalAppData.semid_appEnc,OUTPUT_SEM_MUTEX,OUTPUT_SEM_EMPTY,OUTPUT_SEM_FULL,&(globalAppData.termFlag), &(globalAppData.malloc_mtx), &(globalAppData.termFlagLock)); //Extract a packet
		if(packet_ptr != NULL){
			if(packet_ptr->termFlag == OFF){ //Received non-TERM Message, printing it!
				printf("\n%s-OUT: New Message - |%s|\n", globalAppData.processName, packet_ptr->message_buffer);
				fflush(stdout);
			}
			else{ //Received TERM Message
				printf("\n%s-OUT: Received TERM Message! Notifying Input Thread and Exiting...\n", globalAppData.processName);
				fflush(stdout);	
				sigFinishToOther("OutputWorker",&(globalAppData.termFlagLock), &(globalAppData.termFlag), globalAppData.semid_appEnc, INPUT_SEM_EMPTY); //Wake up Input thread
			}
			pthread_mutex_lock(&(globalAppData.free_mtx));
			free(packet_ptr); //Free memory allocated for previous packet
			pthread_mutex_unlock(&(globalAppData.free_mtx));
			packet_ptr = (MessagePacket*) NULL;
		}
		else{
			if(globalAppData.termFlag == TERM){
				printf("\n%s-OUT: Notified of TERM from Input Signal while in takeFromSharedBuffer!\n", globalAppData.processName);
				fflush(stdout);
			}
			else{
				printf("\n%s-OUT: MessagePacket is NULL for some unknown reason!\n", globalAppData.processName);
				fflush(stdout);
			}
		}
	}

	printf("\n%s-OutputThread (%ld): TERM Value set! Goodbye from OutputWorker!\n", globalAppData.processName, pthread_self());
	fflush(stdout);

	int* exit_valPtr = malloc(sizeof(int));
	*exit_valPtr = 11;
	pthread_exit((void*) exit_valPtr);

}

int message_input(char* input_msg){

	printf("\n%s-IN: Please provide a message to send...(MAX_SIZE=%d)\n> ",globalAppData.processName, MSG_SIZE);
	fflush(stdout);

	int i = 0;
	char c;
	do{
		c = getchar();
		//printf("\nRead: |%c|\n", c);
		if(c == EOF) break; //Ctrl-D from user
		else{
			input_msg[i] = (char) c;	
			if(c == '\n'){
				input_msg[i] = '\0';
				//printf("\n%s: Reached newline! End of Input!\n",processName);
				//fflush(stdout);
				break;	
			}
			if(i == MSG_SIZE-2){
				input_msg[i+1] = '\0';
				//printf("\n%s: Sorry but not enough space for more characters in buffer! End of input!\n",processName);
				break;
			}
			i++;
		}
	} while(1);
	
	printf("\n%s-IN You provided the message:\n|%s|\n", globalAppData.processName, input_msg);
	fflush(stdout);

	return 0;

}
