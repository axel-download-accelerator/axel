
### DEFINITIONS


CFGDIR=cfg/
CLIDIR=cli/
DOCDIR=doc/
I18NDIR=i18n/
OUTDIR=out/
SRCDIR=src/
TESTDIR=test/

include ${CFGDIR}Makefile.settings

.SUFFIXES: .po .mo

# Add your translation here..
MOFILES = nl.mo de.mo ru.mo



all: main test tar

fast: main

prepare:
	mkdir -p "${OUTDIR}"

clean:
	rm -rf "${OUTDIR}"

distclean: clean
	rm -rf "${CFGDIR}"

tar: main prepare
	version=$$(sed -n 's/#define AXEL_VERSION_STRING[ \t]*"\([^"]*\)"/\1/p' < axel.h) && \
	tar --create --transform "s#^#axel-$${version}/#" "--file=${OUTDIR}axel-$${version}.tar" --exclude-vcs -- "${SRCDIR}" "${TESTDIR}" "${I18NDIR}" "${DOCDIR}" configure Makefile axelrc.example axel.spec && \
	gzip --best < "${OUTDIR}axel-$${version}.tar" > "${OUTDIR}axel-$${version}.tar.gz" && \
	bzip2 --best < "${OUTDIR}axel-$${version}.tar" > "${OUTDIR}axel-$${version}.tar.bz2"


### MAIN PROGRAM

main: prepare compile link-cli

compile: compile-shared compile-cli

compile-shared: prepare
	mkdir -p "${OUTDIR}bin/"
	for file in ${SRCDIR}*.c; do \
		fo=$$(echo $${file} | sed -n 's;.*/\(.*\)\.c;\1\.o;p') ; \
		$(CC) -c "$${file}" -o "${OUTDIR}bin/$${fo}" $(CFLAGS) || exit $$? ; \
	done

compile-cli: prepare
	for file in ${CLIDIR}*.c; do \
		fo=$$(echo $${file} | sed -n 's;.*/\(.*\)\.c;\1\.o;p') ; \
		$(CC) -c "$${file}" -o "${OUTDIR}cli/$${fo}" $(CFLAGS) || exit $$? ; \
	done

link-cli: compile-cli
	$(CC) ${OUTDIR}bin/*.o ${OUTDIR}cli/*.o -o "${$OUTDIR}$(OUTFILE)" $(LFLAGS)
	ifdef STRIP
		$(STRIP) "${OUTDIR}$(OUTFILE)"
	endif

### Testing

test: test-compile
	"${OUTDIR}test/test-axel"

test-compile: prepare compile-shared
	mkdir -p "${OUTDIR}test"
	for file in ${TESTDIR}*.c; do \
		fo=$$(echo $${file} | sed -n 's;.*/\(.*\)\.c;\1\.o;p') ; \
		$(CC) -c "$${file}" -o "${OUTDIR}test/$${fo}" $(CFLAGS) $(CFLAGS_TEST) || exit $$? ; \
	done
	$(CC) ${OUTDIR}test/*.o ${OUTDIR}bin/*.o -o "${OUTDIR}test/test-axel" $(LFLAGS) $(LFLAGS_TEST)


### Install / Uninstall

install: install-bin install-etc install-man
uninstall: uninstall-bin uninstall-etc uninstall-man

ifdef I18N
all: $(MOFILES)
install: install-i18n
uninstall: uninstall-i18n
endif

install-man:
	mkdir -p $(DESTDIR)$(MANDIR)/man1/
	cp "${DOCDIR}axel.1" $(DESTDIR)$(MANDIR)/man1/axel.1

uninstall-man:
	rm -f $(MANDIR)/man1/axel.1

install-etc:
	mkdir -p $(DESTDIR)$(ETCDIR)/
	cp axelrc.example $(DESTDIR)$(ETCDIR)/axelrc

uninstall-etc:
	rm -f $(ETCDIR)/axelrc

install-bin:
	mkdir -p $(DESTDIR)$(BINDIR)/
	cp "${OUTDIR}$(OUTFILE)" $(DESTDIR)$(BINDIR)/$(OUTFILE)

uninstall-bin:
	rm -f $(BINDIR)/$(OUTFILE)


### I18N FILES

%.po:
	-@mv ${I18NDIR}$@ ${I18NDIR}$@.bak
	xgettext -k_ -o${I18NDIR}$@ ${I18NDIR}*.[ch]
	@if [ -e ${I18NDIR}$@.bak ]; then \
		echo -n Merging files...; \
		msgmerge -vo ${I18NDIR}$@.combo ${I18NDIR}$@.bak $@; \
		rm -f ${I18NDIR}$@ ${I18NDIR}$@.bak; \
		mv ${I18NDIR}$@.combo ${I18NDIR}$@; \
	fi

.po.mo: $@.po
	msgfmt -vo ${OUTDIR}i18n/$@ ${I18NDIR}$*.po

install-i18n:
	@echo Installing locale files...
	@for mofile in $(MOFILES); do \
		localename=$$(echo de.$$i | cut -d. -f1)
		mkdir -p $(DESTDIR)$(LOCALE)/$${localename}/LC_MESSAGES/; \
		cp ${I18NDIR}$${mofile} $(DESTDIR)$(LOCALE)/$${localename}/LC_MESSAGES/axel.mo; \
	done

uninstall-i18n:
	cd $(LOCALE); find . -name axel.mo -exec 'rm' '{}' ';'
