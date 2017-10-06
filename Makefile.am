AUTOMAKE_OPTIONS = dist-bzip2 dist-xz

SUBDIRS = src po

dist_man_MANS = man/axel.1

EXTRA_DIST = \
	CONTRIBUTING.md \
	CREDITS \
	doc/axelrc.example \
	man/axel.txt \
	man/generate-man.sh

distclean-local:
	-rm -rf autom4te.cache/ \
	m4/

	-rm -f *~ \
	aclocal.m4 \
	compile \
	config.* \
	configure \
	depcomp \
	install-sh \
	Makefile.in \
	missing \
	po/*.gmo \
	po/*.po~ \
	po/Makefile.in.in \
	po/Makevars.template \
	po/Rules-quot \
	po/axel.pot \
	po/stamp-po \
	src/Makefile.in
