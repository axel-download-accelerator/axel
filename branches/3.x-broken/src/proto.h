/* Protocol definitions						*/

/* Unsupported protocol */
#define PROTO_ERR 0

#define PROTO_HTTP	1
#define PROTO_HTTP_NAME "http"

#define PROTO_DEFAULT	PROTO_HTTP

#ifdef FTP
	#define PROTO_FTP	2
	#define PROTO_FTP_NAME	"ftp"
#endif

#ifdef SSL
	#define PROTO_SSL	128
	
	#define PROTO_HTTPS (PROTO_HTTP | PROTO_SSL)
	#define PROTO_HTTPS_NAME "https"
	
	#ifdef FTP
		#define PROTO_FTPS (PROTO_FTP | PROTO_SSL)
		#define PROTO_FTPS_NAME "ftps"
	#endif
	
	/* Check whether a given protocol id specifies a secured (xtpS) protocol */
	#define PROTO_IS_SSL(p) (((p) & PROTO_SSL) != 0)
	/* Protocol without SSL */
	#define PROTO_MAIN(p) ((p) & ~PROTO_SSL)
#endif

typedef struct {
	void* conn_data; /* Connection-specific data */
	/* TODO include a number of callbacks here, including a free-this-struct one */
} proto_t;

proto_t* proto_new(int protoid);

int proto_defport(int protoid);
int proto_getid(const char* protostr);
const char* proto_getname(int protoid);
