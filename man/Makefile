P_DESC = "light command line download accelerator"

.SUFFIXES: .txt .1
.txt.1:
	read P_VERSION P_DATE < ../VERSION && \
	tmp=$$(mktemp tmpXXXXXX) && \
	> "$$tmp" \
	txt2man -d "$$(date -d"$$P_DATE" '+%B %_d, %Y')" \
		-t "$(<:.txt=)" -r "$(<:.txt=)-$$P_VERSION" -s 1 \
		-v $(P_DESC) $< && \
	mv "$$tmp" $@ || { rm -f "$$tmp"; exit 1; }
