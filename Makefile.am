AUTOMAKE_OPTIONS = dist-bzip2 dist-xz

# XXX Fix disagreement between aclocal and make
# (aclocal doesn't update based on mtime)
ACLOCAL = touch $@; @ACLOCAL@

SUBDIRS = src po

dist_man_MANS = man/axel.1
.txt.1:
	$(MAKE) -C $(@D) $(@F)

EXTRA_DIST = \
	CONTRIBUTING.md \
	CREDITS \
	doc/axelrc.example \
	man/axel.txt \
	man/Makefile

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

dist-hook:
	rm -rf $(distdir)/.git
	sed -i '/^am__aclocal/,/[^\\]$$/ s:[$$](top_srcdir)/[.]git/[^ ]* *::g' \
		$(distdir)/Makefile.in
	read _ RELDATE _ < $(top_srcdir)/VERSION; \
	TZ= find $(distdir) -exec touch -d "$$RELDATE" '{}' +

MSG = echo '==>'
ERR = ! echo '!!! ERROR:'
.PHONY: relcheck
relcheck:
	@$(MSG) 'Checking the state of the project...'
	@ m=`git status --porcelain=1 | wc -l`; \
	test "$$m" = 0 \
		&& $(MSG) 'the working tree is clean' \
		|| $(ERR) "$$m" 'modified files in the working tree'
	@test -f "$(top_srcdir)/.git/refs/tags/v$(VERSION)" \
		&& $(MSG) 'v$(VERSION) tag found' \
		|| $(ERR) 'v$(VERSION) tag missing'
	@$(MSG) Everything OK
