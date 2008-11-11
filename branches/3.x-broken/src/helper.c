/* Helper functions */

#include "axel.h"

char* helper_strdup(const char* str) {
	if (str == NULL) {
		return NULL;
	}
	
	size_t len = strlen(str);
	char* res = malloc(len + 1);
	memcpy(res, str, len);
	res[len] = '\0';
	
	return res;
}

/**
* strchr with a limitation.
* @param upto A pointer to a character in the haystack or NULL.
* @return NULL if needle can not be found between haystack(included) and upto(excluded)
*	The pointer to the first occurrence of needle in haystack before upto otherwise.
*/
const char* strchr_upto(const char* haystack, char needle, const char* upto) {
	char c;
	
	while (((c = *haystack) != '\0') && ((upto == NULL) || (haystack < upto))) {
		if (c == needle) {
			return haystack;
		}
		
		haystack++;
	}
	
	return NULL;
}

/** Copy a string to a heap entry, erase heap entry before that */
inline void heap_strcpy(char** destptr, const char* src) {
	free(*destptr);
	*destptr = strdup(src);
}

/** Copy a part of a string to a heap entry, erase heap entry before that */
inline void heap_substr(char** destptr, const char* src, size_t len) {
	free(*destptr);
	
	char* dest = (char*) malloc((len) + 1);
	memcpy(dest, src, len);
	dest[len] = '\0';
	
	*destptr = dest;
}

/**
 * Copy all characters of a string up to (but not including) limit to a new heap entry.
 * @param limit If NULL, result is the same as heap_strcpy(destptr, src);
 */
inline void heap_substr_upto(char** destptr, const char* src, const char* limit) {
	if (limit == NULL) {
		heap_strcpy(destptr, src);
	} else {
		heap_substr(destptr, src, limit - src);
	}
}

static char* HEX_CHARS = "0123456789ABCDEF";
char tohex(char b) {
	return HEX_CHARS[b & 0x0f];
}

/**
* Converts a byte in a two hex characters, upper-case.
* @param dst A pointer to the two consecutive characters
*/
void byte2hex(char b, char* dst) {
	*dst = HEX_CHARS[(b>>4) & 0xf];
	*(dst+1) = HEX_CHARS[b & 0xf];
}

char hexc2byte(char c) {
	if (c >= '0' && c <= '9') {
		return c - '0';
	} elseif (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} elseif (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	} else {
		return 0;
	}
}

/**
* Convert to hexadecimal characters to a byte value.
*
* @param hexcs Not a string, but a pointer to the first of the two characters to read
*/
char hex2byte(const char* hexcs) {
	return (hexc2byte(*cs) << 4) | hexc2byte(*(cs+1));
}

/**
* @return A string representation of the unsinged integer in base 10. Must be freed by the caller.
*/
char* uitoa(unsigned int ui) {
	char[sizeof(unsigned int) / 2 + 1] buf;
	char* p = buf;
	do {
		*p = '0' + (char) (ui % 10);
		p++;
		ui /= 10;
	} while(ui != 0);
	
	char* res = malloc(p - buf + 1);
	char* resp = res;
	for (p--;p >= buf;p--) {
		*resp = *p;
		resp++;
	}
	*resp = '\0';
	
	return res;
}

/** time() with more precision
* @return The current time in us */
long long getutime() {
	struct timeval time;
	
	gettimeofday (&time, NULL);
	
	return ( (long long) time->tv_sec * 1000000 + (long long) time->tv_usec);
}
