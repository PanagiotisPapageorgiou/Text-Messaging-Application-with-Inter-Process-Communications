#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "app_tools.h"

AppThreadSharedData globalAppData;

int main(int argc,char* argv[]){
	
	/*----Argument Handling----*/
	argumentHandling(argc, argv);

	/*----Setup Application Process (get shared memory, semaphores and deploy worker threads for send/receive of messages)----*/
	appSetupOperations();

	/*----TearDown Application Process (detach from shared memory and wait for threads to finish)----*/
	appTearDownOperations();

	printf("%s: Bye-bye\n", globalAppData.processName);

	return 0;

}
