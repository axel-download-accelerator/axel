/* Helper functions and macros */

#define min( a, b )		( (a) < (b) ? (a) : (b) )
#define max( a, b )		( (a) > (b) ? (a) : (b) )

void* safe_malloc(size_t size);
void* safe_realloc(void *ptr, size_t size);

char* helper_strdup(const char* str);
char* safe_strdup(const char* str);
const char* strchr_upto(const char* haystack, char needle, const char* upto);

void heap_strcpy(char** destptr, const char* src);
void heap_substr(char** destptr, const char* src, size_t len);
void heap_substr_upto(char** destptr, const char* src, const char* srclimit);

char tohex(char b);
void byte2hex(char b, char* dst);
char hexc2byte(char c);
char hex2byte(char* hexcs);

char* uitoa(unsigned int ui);

#define axel_time long long
axel_time getutime();

#ifdef DEBUG
	void debug_print(const char* msg);
	void debug_printf(const char* format, ...);
	
	#define DEBUG_ASSERT(assertion,msg) {if (!(assertion)) { debug_print(msg);}}
#else
	#define DEBUG_ASSERT(assertion,msg) ;
#endif
