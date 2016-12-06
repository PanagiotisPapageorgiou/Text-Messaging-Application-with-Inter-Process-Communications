#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "enc_tools.h"

EncThreadSharedData globalEncData;

int main(int argc,char* argv[]){
	
	/*----Argument Handling----*/
	argumentHandling(argc, argv);

	/*----Setup Encoder Process (get shared memory segments, semaphores and deploy worker threads for send/receive of messages)----*/
	encSetupOperations();

	/*----TearDown Encoder Process (detach from shared memories and wait for threads to finish)----*/
	encTearDownOperations();

	printf("%s: Bye-bye\n", globalEncData.processName);

	return 0;

}
