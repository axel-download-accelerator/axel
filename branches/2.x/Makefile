###########################
##   Makefile for Axel   ##
##                       ##
## Copyright 2001 Lintux ##
###########################


### DEFINITIONS

-include Makefile.settings

.SUFFIXES: .po .mo

# Add your translation here..
MOFILES = nl.mo de.mo ru.mo zh_cn.mo


all: $(OUTFILE)
install: install-bin install-etc install-man
uninstall: uninstall-bin uninstall-etc uninstall-man

ifdef I18N
all: $(MOFILES)
install: install-i18n
uninstall: uninstall-i18n
endif

clean:
	rm -f *.o $(OUTFILE) search core *.mo

distclean: clean
	rm -f Makefile.settings config.h

install-man:
	mkdir -p $(DESTDIR)$(MANDIR)/man1/
	cp axel.1 $(DESTDIR)$(MANDIR)/man1/axel.1
	mkdir -p $(DESTDIR)$(MANDIR)/zh_CN/
	cp axel_zh_CN.1 $(DESTDIR)$(MANDIR)/zh_CN/man1/axel.1

uninstall-man:
	rm -f $(MANDIR)/man1/axel.1
	rm -f $(MANDIR)/zh_CN/man1/axel.1

install-etc:
	mkdir -p $(DESTDIR)$(ETCDIR)/
	cp axelrc.example $(DESTDIR)$(ETCDIR)/axelrc

uninstall-etc:
	rm -f $(ETCDIR)/axelrc

### MAIN PROGRAM

$(OUTFILE): axel.o conf.o conn.o ftp.o http.o search.o tcp.o text.o
	$(CC) axel.o conf.o conn.o ftp.o http.o search.o tcp.o text.o -o $(OUTFILE) $(LFLAGS)
ifndef DEBUG
	-$(STRIP) $(OUTFILE)
endif

.c.o:
	$(CC) -c $*.c -o $*.o -Wall $(CFLAGS)

install-bin:
	mkdir -p $(DESTDIR)$(BINDIR)/
	cp $(OUTFILE) $(DESTDIR)$(BINDIR)/$(OUTFILE)

uninstall-bin:
	rm -f $(BINDIR)/$(OUTFILE)

tar: distclean
	x=`pwd | sed -e 's/\/.*\///'`; \
	cd ..; \
	tar czf $$x.tar.gz $$x


### I18N FILES

%.po:
	-@mv $@ $@.bak
	xgettext -k_ -o$@ *.[ch]
	@if [ -e $@.bak ]; then \
		echo -n Merging files...; \
		msgmerge -vo $@.combo $@.bak $@; \
		rm -f $@ $@.bak; \
		mv $@.combo $@; \
	fi

.po.mo: $@.po
	msgfmt -vo $@ $*.po

install-i18n:
	@echo Installing locale files...
	@for i in $(MOFILES); do \
		mkdir -p $(DESTDIR)$(LOCALE)/`echo $$i | cut -d. -f1`/LC_MESSAGES/; \
		cp $$i $(DESTDIR)$(LOCALE)/`echo $$i | cut -d. -f1`/LC_MESSAGES/axel.mo; \
	done

uninstall-i18n:
	cd $(LOCALE); find . -name axel.mo -exec 'rm' '{}' ';'
