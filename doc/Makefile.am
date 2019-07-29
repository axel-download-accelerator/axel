EXTRA_DIST += \
	doc/axelrc.example \
	doc/axel.txt

dist_man_MANS = doc/axel.1

PACKAGE_DESC = lightweight command line download accelerator

.txt.1:
	read _ P_DATE _ < $(top_srcdir)/VERSION && \
	tmp=$$(mktemp tmpXXXXXX) && \
	> "$$tmp" \
	txt2man -t "$(<:.txt=)" -s 1 \
		-d "$$(date -d"$$P_DATE" '+%B %_d, %Y')" \
		-P "$(PACKAGE_NAME)" \
		-r "$(PACKAGE_NAME)-$(PACKAGE_VERSION)" \
		-v "$(PACKAGE_DESC)" $< &&\
	mv "$$tmp" $@ || { rm -f "$$tmp"; exit 1; }
