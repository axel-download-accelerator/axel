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


// These functions are only called from axel's core
void axel_message(const axel_t* axel, int verbosity, const char* message);
void axel_message_fmt(const axel_t *axel, int verbosity, const char *format, ... );
void axel_message_heap(const axel_t* axel, int verbosity, const char* message);
