/* Textual messages within axel */

message_t* message_new_safe(char* str, _Bool onheap, message_relevance rel) {
	message_t* res = malloc_safe(sizeof(message_t));
	
	res->str = str;
	res->onheap = onheap;
	res->rel = rel;
	
	return res;
}

void message_free(message_t* msg) {
	if (msg->onheap) {
		free(msg->str);
	}
	
	free(msg);
}

void msgq_init(msgq_t* mq) {
	msl->head = NULL;
	msl->tail = NULL;
}

void msgq_destroy(msgq_t* mq) {
	message_t* msg;
	while ((msg = msgq_dequeue(msl)) != NULL) {
		message_free(msg);
	}
}

message_t* msgq_dequeue(msgq_t* mq) {
	message_t* res = mq->head;
	
	if (res != NULL) {
		mq->head = res->next;
		
		if (res->next == NULL) {
			mq->tail = NULL;
		}
	}
	
	return res;
}

void msgq_enqueue(msgq_t* mq, message_t* msg) {
	msg->next = NULL;
	
	if (mq->tail == NULL) {
		mq->tail = msg;
		mq->head = msg;
	} else {
		mq->tail->next = msg;
		mq->tail = msg;
	}
}
