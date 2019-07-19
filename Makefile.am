AUTOMAKE_OPTIONS = foreign dist-bzip2 dist-xz

# XXX Fix disagreement between aclocal and make
# (aclocal doesn't update based on mtime)
ACLOCAL = touch $@; @ACLOCAL@

SUBDIRS = po

EXTRA_DIST = \
	scripts/pocheck.awk \
	README.md \
	CONTRIBUTING.md \
	CREDITS

include doc/Makefile.am

distclean-local:
	-rm -rf autom4te.cache
	-rm -f  *~ po/*~ po/*.gmo src/*~

dist-hook:
	rm -rf $(distdir)/.git
	read _ RELDATE _ < $(top_srcdir)/VERSION; \
	TZ= find $(distdir) -exec touch -d "$$RELDATE" '{}' +

MSG = echo '==>'
ERR = ! echo '!!! ERROR:'
.PHONY: relcheck
relcheck:
	@$(MSG) 'Checking for non-UTF8 translations...'
	@$(AWK) -f $(top_srcdir)/scripts/pocheck.awk $(top_srcdir)/po/*.po
	@$(MSG) 'Checking the state of the project...'
	@ m=`git status --porcelain=1 | wc -l`; \
	test "$$m" = 0 \
		&& $(MSG) 'the working tree is clean' \
		|| $(ERR) "$$m" 'modified files in the working tree'
	@test -f "$(top_srcdir)/.git/refs/tags/v$(VERSION)" \
		&& $(MSG) 'v$(VERSION) tag found' \
		|| $(ERR) 'v$(VERSION) tag missing'
	@$(MSG) Everything OK


include src/Makefile.am
