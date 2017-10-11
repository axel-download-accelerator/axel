END { exit broken }
BEGIN { OFS=":" }
/^$/ || /^"Content-Type:/ {
	if (!/charset=(UTF|utf)-8/) {
		print FILENAME, FNR, $0
		++broken
	}
	nextfile
}
