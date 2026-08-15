EXTRA_DIST += \
	doc/axelrc.example \
	doc/axel.txt

dist_man_MANS = doc/axel.1

PACKAGE_DESC = lightweight command line download accelerator
doc_reldate = @RELDATE_PRETTY@

.txt.1:
	$(MKDIR_P) $(@D)
	tmp=$$(mktemp tmpXXXXXX) && \
	> "$$tmp" \
	$(TXT2MAN) -t "$$(basename "$<" .txt)" -s 1 \
		-d "$(doc_reldate)" \
		-P "$(PACKAGE_NAME)" \
		-r "$(PACKAGE_NAME)-$(PACKAGE_VERSION)" \
		-v "$(PACKAGE_DESC)" $< &&\
	mv "$$tmp" $@ || { rm -f "$$tmp"; exit 1; }
