#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "chan_tools.h"

ChanThreadSharedData globalChanData;

int main(int argc,char* argv[]){

	/*----Argument Handling----*/
	argumentHandling(argc, argv);

	/*----Channel Setup Operations----*/
	chanSetupOperations();

	/*----Channel Tear Down Operations----*/
	chanTearDownOperations();

	printf("%s: Bye-bye\n", globalChanData.processName);

	return 0;

}
