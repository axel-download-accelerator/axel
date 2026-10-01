#include "axel.h"

int
main(void)
{
	conn_t conn = {0};
	char filename[MAX_STRING];

	if (!conn_set(&conn, "http://127.0.0.1/%2e%2e%2fpwned.txt"))
		return 1;

	conn_output_filename(&conn, filename, sizeof(filename));
	if (strcmp(filename, "pwned.txt") != 0) {
		fprintf(stderr, "expected pwned.txt, got %s\n", filename);
		return 1;
	}

	if (!conn_set(&conn, "http://127.0.0.1/%252e%252e%252fpwned.txt"))
		return 1;

	conn_output_filename(&conn, filename, sizeof(filename));
	if (strcmp(filename, "%2e%2e%2fpwned.txt") != 0) {
		fprintf(stderr, "expected %%2e%%2e%%2fpwned.txt, got %s\n",
			filename);
		return 1;
	}

	if (!conn_set(&conn, "http://127.0.0.1/path%2finner%2ffile.txt?token=1"))
		return 1;

	conn_output_filename(&conn, filename, sizeof(filename));
	if (strcmp(filename, "file.txt?token=1") != 0) {
		fprintf(stderr, "expected file.txt?token=1, got %s\n", filename);
		return 1;
	}

	if (!conn_set(&conn, "http://127.0.0.1/dir%2f"))
		return 1;

	conn_output_filename(&conn, filename, sizeof(filename));
	if (strcmp(filename, "dir") != 0) {
		fprintf(stderr, "expected dir, got %s\n", filename);
		return 1;
	}

	return 0;
}
