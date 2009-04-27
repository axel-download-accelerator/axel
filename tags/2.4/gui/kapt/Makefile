include ../../Makefile.settings

all:

install:
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 0755 axel-kapt $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1/
	install -m 0644 axel-kapt.1 $(DESTDIR)$(MANDIR)/man1/
	mkdir -p $(DESTDIR)$(SHAREDIR)/applications/
	install -m 0644 axel-kapt.desktop $(DESTDIR)$(SHAREDIR)/applications/
