bin_PROGRAMS = axel
axel_SOURCES = \
	src/android-compat.h \
	src/axel.c \
	src/axel.h \
	src/sleep.h \
	src/conf.c \
	src/conf.h \
	src/conn.c \
	src/conn.h \
	src/ftp.c \
	src/ftp.h \
	src/http.c \
	src/http.h \
	src/search.c \
	src/search.h \
	src/ssl.c \
	src/ssl.h \
	src/tcp.c \
	src/tcp.h \
	src/text.c

AM_CFLAGS = $(PTHREAD_CFLAGS) $(WARN_CFLAGS) \
	-Wno-declaration-after-statement \
	-Wno-error=cast-align \
	-Wno-error=inline

axel_LDADD = $(LIBOBJS) $(SSL_LIBS) $(LIBINTL) $(PTHREAD_LIBS)
axel_CC = $(PTHREAD_CC)

AM_CPPFLAGS = -DLOCALEDIR=\""$(localedir)"\"
AM_CPPFLAGS += -D_BSD_SOURCE -D_DEFAULT_SOURCE
