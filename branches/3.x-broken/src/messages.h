/* Textual messages within axel */

enum message_relevance {
	chatter,
	status,
	warning,
	error,
	critical
};

struct {
	char* msg; // The message text, on the heap.
	_Bool onheap; // True iff the message is stored on the heap (and must be freed by this module)
	message_relevance rel;
	struct message_struct* next;
} message_struct;

typedef struct message_struct message_t;

