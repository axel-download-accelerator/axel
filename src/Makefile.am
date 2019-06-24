AM_CFLAGS = $(WARN_CFLAGS) -Wno-declaration-after-statement

bin_PROGRAMS = axel
axel_SOURCES = \
	axel.c \
	axel.h \
	sleep.h \
	conf.c \
	conf.h \
	conn.c \
	conn.h \
	ftp.c \
	ftp.h \
	http.c \
	http.h \
	search.c \
	search.h \
	ssl.c \
	ssl.h \
	tcp.c \
	tcp.h \
	text.c

axel_CFLAGS = $(PTHREAD_CFLAGS) $(AM_CFLAGS)
axel_LDADD = $(SSL_LIBS) $(PTHREAD_LIBS)
axel_CC = $(PTHREAD_CC)

AM_CPPFLAGS = -DLOCALEDIR=\""$(localedir)"\"
AM_CPPFLAGS += -D_BSD_SOURCE -D_DEFAULT_SOURCE
