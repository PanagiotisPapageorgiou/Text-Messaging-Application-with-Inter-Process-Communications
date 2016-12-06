#define MSG_SIZE 256
#define OFF 0
#define RESEND 1
#define OK 2
#define TERM -6
#define NSEMS_APP_ENC 6
#define NSEMS_ENC_CHAN 7
#define NSEMS_SETUP 10
#define PERMS 0600
#define SEMKEY_SETUP (key_t) 54414
#define APP 1
#define ENC 2
#define CHAN 3
#define DOWN -1
#define UP 1
#define QUEUE_ARRAY_SIZE 10
#define INPUT_SEM_MUTEX 0
#define INPUT_SEM_EMPTY 1
#define INPUT_SEM_FULL 2
#define OUTPUT_SEM_MUTEX 3
#define OUTPUT_SEM_EMPTY 4
#define OUTPUT_SEM_FULL 5
#define CONFIRM_SEM 6

#define UNINITIALISED -523

/*----Basic Packet Data Structure----*/
typedef struct MessagePacket{
	char message_buffer[MSG_SIZE]; //The message
	int retryFlag; //Code showing Success/Failure
	int termFlag; //Special flag used to indicate termination
	char md5_hash[34]; //The hash code of this message
} MessagePacket;

/*----Queue Implementation with Array for MessagePackets (used in Shared Memory Segments)----*/
typedef struct MessageQueueArray{
	int head, tail, items;
	MessagePacket packets_array[QUEUE_ARRAY_SIZE];
} MessageQueueArray;

/*----Shared Memory Segments contain a MessageQueue for Input(sending) and one for Output(receiving)----*/
typedef struct SharedMemorySegment{
	MessageQueueArray inputQueue;
	MessageQueueArray outputQueue;
} SharedMemorySegment;

int clear_string_buffer(char*,int);
int copyTo_string_buffer(char*,char*,int);
MessagePacket* createPacket(char*, char*, int, int);

/*----Array Queue----*/
int init_arrayQueue(MessageQueueArray*);
int clear_arrayQueue(MessageQueueArray*);
int isEmpty_arrayQueue(MessageQueueArray);
int isFull_arrayQueue(MessageQueueArray);
int enQueue_arrayQueue(MessageQueueArray*, MessagePacket);
MessagePacket* deQueue_arrayQueue(MessageQueueArray*);
MessagePacket* rear_arrayQueue(MessageQueueArray);
MessagePacket* front_arrayQueue(MessageQueueArray);
void printQueue_arrayQueue(MessageQueueArray);
