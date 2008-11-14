static url_li_t* url_li_new(url_t* url);
static url_li_t* url_li_free(url_li_t* urlli);
static void url_li_freeall(url_li_t* urlli);

static _Bool urllist_incurgroup(urllist_t* ul, int prio);
static void urllist_advancenext(urllist_t* ul);

static void urllist_lock(ul);
static void urllist_unlock(ul);

void urllist_init(urllist_t* ul) {
	ul->head = NULL;
	ul->next = NULL;
	ul->heap = NULL;
	
	pthread_mutex_init(& ul->lock, NULL);
}

void urllist_destroy(urllist_t* ul) {
	url_li_freeall(ul->head);
	url_li_freeall(ul->heap);
	
	pthread_mutex_destroy(& ul->lock);
}

/**
* Adds a URL to a URL list. From now on, the pointer to the url struct belongs to this struct.
* An URL struct must not be added twice.
*/
void urllist_add(urllist_t* ul, url_t* url) {
	DEBUG_ASSERT(url != NULL, "Internal error: urllist_addurl(, NULL)");
	
	urllist_lock(ul);
	
	int prio = url->priority;
	url_li_t* item = url_li_new(url);
	
	url_li_t* prev = NULL;
	url_li_t* cur = ul->head;
	
	while ((cur != NULL) && (cur->url->priority >= prio)) {
		prev = cur;
		cur = cur->next;
	}
	
	item->next = cur;
	if (prev == NULL) {
		ul->head = item;
	} else {
		prev->next = item;
	}
	
	// Update the pointer to the next URL in line
	if ((ul->next == NULL) || (ul>next->priority < prio)) {
		ul->next = item;
	}
	
	urllist_unlock(ul);
}

/**
* Removes an URL from the list. The pointer to the URL becomes invalid afterwards iff 1 is returned!
* @return 1 iff the URL was removed
*/
_Bool urllist_remove(urllist_t* ul, url_t* url) {
	DEBUG_ASSERT(url != NULL, "Internal error: urllist_removeurl(, NULL)");
	
	urllist_lock(ul);
	
	url_li_t* prev = NULL;
	url_li_t* cur = ul>head;
	
	while ((cur != NULL) && (cur->url != url)) {
		prev = cur;
		cur = cur->next;
	}
	
	if (cur == NULL) { // URL not found
		return 0;
	}
	
	if (prev == NULL) {
		ul->head = cur->next;
	} else {
		prev->next = cur->next;
	}
	
	urllist_advancenext(ul);
	
	// Move list item to the heap
	cur->next = ul->heap;
	ul->heap = cur;
	
	urllist_unlock(ul);
	
	return 1;
}

url_t* urllist_next(urllist_t* ul) {
	urllist_lock(ul);
	
	url_t* res = ul->next->url;
	urllist_advancenext(ul);
	
	urllist_unlock(ul);
	
	return res;
}

/**
* Construct a new item of a URL list.
*/
inline static url_li_t* url_li_new(url_t* url) {
	url_li_t* res = malloc(sizeof(url_li_t));
	
	res->url = url;
	res->next = NULL;
	
	return res;
}

/**
* Free a URL list item, including the URL itself.
* @return The next item in the list of URLs
*/
inline static url_li_t* url_li_free(url_li_t* urlli) {
	url_li_t* res = urlli->next;
	
	free(urlli->url);
	free(urlli);
	
	return res;
}

/**
* Frees a chain of URL list items
*/
static void url_li_freeall(url_li_t* urlli) {
	while (urlli != NULL) {
		urlli = url_li_free(urlli);
	}
}

/**
* Helper method to check whether a priority is still in the active group.
*/
inline static _Bool urllist_incurgroup(const urllist_t* ul, int prio) {
	DEBUG_ASSERT(ul->head != NULL, "");
	
	return (ul->head->url->priority & URLLIST_GROUPMASK) == (prio & URLLIST_GROUPMASK);
}

/**
* Move the next pointer to the next URL element to return.
*/
static void urllist_advancenext(urllist_t* ul) {
	url_li_t* nx = ul->next->next;
	
	if ((nx != NULL) && (urllist_incurgroup(ul, nx->url->priority))) {
		ul->next = nx;
	} else {
		ul->next = head;
	}
}

inline static void urllist_lock(ul) {
	pthread_mutex_lock(& ul->lock);
}

inline static void urllist_unlock(ul) {
	pthread_mutex_unlock(& ul->lock);
}
