#ifndef AXEL_URL_H

/* URL fields */
enum {
	URL_USER,  URL_USER_END,
	URL_PASS,  URL_PASS_END,
	URL_HOST,  URL_HOST_END,
	URL_DIR,   URL_DIR_END,
	URL_FNAME, URL_FNAME_END,
	URL_PORT,
	URL_NFIELDS,
};

const char *parse_url(int *proto, const char **fields, const char *url);

#endif /* AXEL_URL_H */
