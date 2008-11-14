/**
A list of URLs with an associated priority (the higher, the sooner this URL is requested).
Priorities are grouped in ... groups. Any two URLs are in a common group iff their priorities are identical except for the last eight bits.

All URLs are owned by this struct.
*/

#define URLLIST_GROUPMASK (~(URL_PRIO_GROUPSIZE - 1))

// An element of the list
struct url_li_struct {
	url_t* url;
	struct url_li_struct* next; // NULL for end of list
};
typedef struct url_li_struct url_li_t;

typedef struct {
	url_li_t* head; // The URL with the highest priority
	url_li_t* next; // The next URL to serve by urllist_next.
	url_li_t* heap; // Keeps track of all removed URLs
	
	pthread_mutex_t lock;
} urllist_t;

void urllist_init(urllist_t* ul);
void urllist_add(urllist_t* ul, url_t* url);
_Bool urllist_remove(urllist_t* ul, url_t* url);
url_t* urllist_next(urllist_t* ul);
void urllist_destroy(urllist_t* ul);

