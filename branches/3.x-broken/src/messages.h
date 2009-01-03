/* Store textual messages in axel */

enum message_relevance {
	debug,
	chatter,
	status,
	warning,
	error,
	critical
};

struct {
	char* str; // The message text, on the heap.
	_Bool onheap; // True iff msg is stored on the heap (and must be freed by this module)
	message_relevance rel;
	struct message_struct* next;
} message_struct;
typedef struct message_struct message_t;

void message_new_safe(message_t* msg, char* str, _Bool onheap, message_relevance rel);
void message_free(message_t* msg);

typedef struct {
	message_t* head;
	message_t* tail;
} msgq_t;

void msgq_init(msgq_t* mq);
void msgq_destroy(msgq_t* mq);
message_t* msgq_dequeue(msgq_t* mq);
void msgq_enqueue(msgq_t* mq, message_t* msg);


