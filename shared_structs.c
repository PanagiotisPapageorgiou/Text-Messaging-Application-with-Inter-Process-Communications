#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared_structs.h"

/*=======================================MESSAGE PACKET=============================================*/

int clear_string_buffer(char* buffer,int limit){ //Fills a buffer with '\0'

        int i = 0;
        
        if(buffer == (char*) NULL){
                printf("clear_string_buffer: NULL buffer!\n");
                return -1;
        }
        
        for(i=0; i < limit; i++){
                buffer[i] = '\0';
        }

        return 0;
        
}

int copyTo_string_buffer(char* source, char* target, int limit){ //Same as strcpy / This function is NEVER USED

        int i=0;

        if(source == NULL){
                printf("copyTo_string_buffer: NULL source buffer!\n");
                return -1;
        }

        if(target == NULL){
                printf("copyTo_string_buffer: NULL target buffer!\n");
                return -1;
        }

        for(i=0; i < limit; i++){
                target[i] = source[i];
        }

        return 0;

}

MessagePacket* createPacket(char* msg, char* hash, int retryFlag, int termFlag){ //Allocates and initialises a new MessagePacket

	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	packet_ptr = malloc(sizeof(MessagePacket));
	if(packet_ptr == NULL){
		printf("createPacket: Error! Failed to allocate memory for packet!\n");
		fflush(stdout);
		return (MessagePacket*) NULL;
	}

	if(msg == NULL){
		printf("createPacket: Error! Message is NULL!\n");
		fflush(stdout);
		return (MessagePacket*) NULL;
	}

	clear_string_buffer(packet_ptr->md5_hash , 34);
	clear_string_buffer(packet_ptr->message_buffer, MSG_SIZE);

	strcpy(packet_ptr->message_buffer, msg);
	if(hash != NULL) strcpy(packet_ptr->md5_hash, hash);
	packet_ptr->retryFlag = retryFlag;
	packet_ptr->termFlag = termFlag;

	return packet_ptr;

}

/*=============================================Array Queue=============================================*/

/*-----I have created a circular array implementation for the Queue buffers of the Shared Memories----*/
/*----Credits for help: geekzquiz.com/queue-set-1introduction-and-array-implementation----*/

int init_arrayQueue(MessageQueueArray* queue_arrayPtr){

	int i=0;

	//printf("init_arrayQueue: Initialising New MessageQueueArray!\n");
	//fflush(stdout);

	if(queue_arrayPtr != NULL){
		return clear_arrayQueue(queue_arrayPtr);
	}
	else{
		printf("init_arrayQueue: Error! MessageQueueArray Pointer is NULL!\n");
		fflush(stdout);
		return -1;
	}

}

int clear_arrayQueue(MessageQueueArray* queue_arrayPtr){

	int i=0;

	queue_arrayPtr->head = 0;
	queue_arrayPtr->tail = QUEUE_ARRAY_SIZE - 1;
	queue_arrayPtr->items = 0;

	return 0;

}

int isEmpty_arrayQueue(MessageQueueArray queue_array){

	//printf("isEmpty_arrayQueue: Checking if queue is empty...\n");
	//fflush(stdout);
	return (queue_array.items == 0);

}

int isFull_arrayQueue(MessageQueueArray queue_array){

	//printf("isFull_arrayQueue: Checking if full...\n");
	//fflush(stdout);
	return (queue_array.items == QUEUE_ARRAY_SIZE);

}

int enQueue_arrayQueue(MessageQueueArray* queue_arrayPtr, MessagePacket packet){

	//printf("enQueue_arrayQueue: Attempting to add new MessagePacket...\n");
	//fflush(stdout);

	if(queue_arrayPtr != NULL){
		if(isFull_arrayQueue(*queue_arrayPtr)){
			printf("enQueue_arrayQueue: Queue is FULL!!! SEMAPHORE ERROR!\n");
			fflush(stdout);
			exit(1);
		}
		else{

			//printQueue_arrayQueue(*queue_arrayPtr);

			queue_arrayPtr->tail = (queue_arrayPtr->tail + 1) % QUEUE_ARRAY_SIZE; /*----Tail must not exceed Queue boundaries (circular)----*/

			/*----Clear existing Packet in Queue and Copy the New one over it----*/
			clear_string_buffer(queue_arrayPtr->packets_array[queue_arrayPtr->tail].message_buffer , MSG_SIZE);
			clear_string_buffer(queue_arrayPtr->packets_array[queue_arrayPtr->tail].md5_hash , 34);
			strcpy(queue_arrayPtr->packets_array[queue_arrayPtr->tail].message_buffer, packet.message_buffer);
			strcpy(queue_arrayPtr->packets_array[queue_arrayPtr->tail].md5_hash, packet.md5_hash);
			//copyTo_string_buffer(packet.message_buffer, queue_arrayPtr->packets_array[queue_arrayPtr->tail].message_buffer, MSG_SIZE);
			//copyTo_string_buffer(packet.md5_hash, queue_arrayPtr->packets_array[queue_arrayPtr->tail].md5_hash, 33);
			queue_arrayPtr->packets_array[queue_arrayPtr->tail].retryFlag = packet.retryFlag;
			queue_arrayPtr->packets_array[queue_arrayPtr->tail].termFlag = packet.termFlag;
			(queue_arrayPtr->items)++;

			//printf("enQueue_arrayQueue: Item added successfully!\n");
			//fflush(stdout);

			//printQueue_arrayQueue(*queue_arrayPtr);

			return 1;
		}
	}
	else{
		printf("enQueue_arrayQueue: queue_arrayPtr is NULL!\n");
		fflush(stdout);
		return -1;
	}

}

MessagePacket* deQueue_arrayQueue(MessageQueueArray* queue_arrayPtr){ /*---Remember to free the memory this function allocates---*/

	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	//printf("deQueue_arrayQueue: Attempting to remove MessagePacket from Queue...\n");
	//fflush(stdout);

	if(queue_arrayPtr == NULL){
		printf("deQueue_arrayQueue: queue_arrayPtr is NULL!\n");
		fflush(stdout);
		return (MessagePacket*) NULL;
	}
	else{
		if(isEmpty_arrayQueue(*queue_arrayPtr) == 1){
			printf("deQueue_arrayQueue: Queue is Empty! SEMAPHORE ERROR\n");
			fflush(stdout);
			exit(1);
		}
		else{
			packet_ptr = (MessagePacket*) malloc(sizeof(MessagePacket));
			if(packet_ptr == NULL){
				printf("deQueue_arrayQueue: Failed to allocate memory for MessagePacket copy!\n");
				fflush(stdout);
				exit(1);
			}

			clear_string_buffer(packet_ptr->message_buffer , MSG_SIZE);
			clear_string_buffer(packet_ptr->md5_hash , 34);
			strcpy(packet_ptr->message_buffer, queue_arrayPtr->packets_array[queue_arrayPtr->head].message_buffer);
			strcpy(packet_ptr->md5_hash, queue_arrayPtr->packets_array[queue_arrayPtr->head].md5_hash);
			//copyTo_string_buffer(queue_arrayPtr->packets_array[queue_arrayPtr->head].message_buffer, packet_ptr->message_buffer, MSG_SIZE);
			//copyTo_string_buffer(queue_arrayPtr->packets_array[queue_arrayPtr->head].md5_hash, packet_ptr->md5_hash, MSG_SIZE);
			packet_ptr->retryFlag = queue_arrayPtr->packets_array[queue_arrayPtr->head].retryFlag;
			packet_ptr->termFlag = queue_arrayPtr->packets_array[queue_arrayPtr->head].termFlag;
			
			queue_arrayPtr->head = (queue_arrayPtr->head + 1) % QUEUE_ARRAY_SIZE;
			(queue_arrayPtr->items)--;

			//printf("deQueue_arrayQueue: Item removed successfully!\n");
			//fflush(stdout);

			//printQueue_arrayQueue(*queue_arrayPtr);

			return packet_ptr;
		}
	}

}

MessagePacket* rear_arrayQueue(MessageQueueArray queue_array){ /*----Returns the Last Packet of the Queue----*/

	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	if(isEmpty_arrayQueue(queue_array)) return (MessagePacket*) NULL;
	packet_ptr = (MessagePacket*) malloc(sizeof(MessagePacket));
	if(packet_ptr == NULL){
		printf("rear_arrayQueue: Failed to allocate memory for MessagePacket copy!\n");
		return (MessagePacket*) NULL;
	}
	clear_string_buffer(packet_ptr->message_buffer , MSG_SIZE);
	clear_string_buffer(packet_ptr->md5_hash , 34);
	strcpy(packet_ptr->message_buffer, queue_array.packets_array[queue_array.tail].message_buffer);
	strcpy(packet_ptr->md5_hash, queue_array.packets_array[queue_array.tail].md5_hash);
	//copyTo_string_buffer(queue_arrayPtr->packets_array[queue_arrayPtr->tail].message_buffer, packet_ptr->message_buffer, MSG_SIZE);
	//copyTo_string_buffer(queue_arrayPtr->packets_array[queue_arrayPtr->tail].md5_hash, packet_ptr->md5_hash, MSG_SIZE);
	packet_ptr->retryFlag = queue_array.packets_array[queue_array.tail].retryFlag;
	packet_ptr->termFlag = queue_array.packets_array[queue_array.tail].termFlag;

	return packet_ptr;
	
}

MessagePacket* front_arrayQueue(MessageQueueArray queue_array){ /*----Returns the first Packet of the Queue----*/

	MessagePacket* packet_ptr = (MessagePacket*) NULL;

	if(isEmpty_arrayQueue(queue_array)) return (MessagePacket*) NULL;
	packet_ptr = (MessagePacket*) malloc(sizeof(MessagePacket));
	if(packet_ptr == NULL){
		printf("front_arrayQueue: Failed to allocate memory for MessagePacket copy!\n");
		return (MessagePacket*) NULL;
	}
	clear_string_buffer(packet_ptr->message_buffer , MSG_SIZE);
	clear_string_buffer(packet_ptr->md5_hash , 34);
	strcpy(packet_ptr->message_buffer, queue_array.packets_array[queue_array.head].message_buffer);
	strcpy(packet_ptr->md5_hash, queue_array.packets_array[queue_array.head].md5_hash);
	//copyTo_string_buffer(queue_arrayPtr->packets_array[queue_arrayPtr->head].message_buffer, packet_ptr->message_buffer, MSG_SIZE);
	//copyTo_string_buffer(queue_arrayPtr->packets_array[queue_arrayPtr->head].md5_hash, packet_ptr->md5_hash, MSG_SIZE);
	packet_ptr->retryFlag = queue_array.packets_array[queue_array.head].retryFlag;
	packet_ptr->termFlag = queue_array.packets_array[queue_array.head].termFlag;

	return packet_ptr;
	
}

void printQueue_arrayQueue(MessageQueueArray queue_array){

	int i=0;

	if(isEmpty_arrayQueue(queue_array) == 1){
		printf("printQueue_arrayQueue: Queue is empty!\n");
		fflush(stdout);
		return;
	}

	printf("=============================printQueue_arrayQueue=======================================\n");
	fflush(stdout);

	do{
		printf("|%d| - |%s|\n", i+1, queue_array.packets_array[queue_array.head+i % QUEUE_ARRAY_SIZE].message_buffer);
		fflush(stdout);
		if(queue_array.head+i % QUEUE_ARRAY_SIZE == queue_array.tail) break;
		i++;
	} while(1);

	printf("===========================================================================================\n");
	fflush(stdout);

	return;

}
