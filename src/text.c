/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Y Giridhar Appaji Nag
  Copyright 2008-2010 Philipp Hagemeister
  Copyright 2015-2017 Joao Eriberto Mota Filho
  Copyright 2016      Denis Denisov
  Copyright 2016      Mridul Malpotra
  Copyright 2016      Stephen Thirlwall
  Copyright 2017      Antonio Quartulli
  Copyright 2017      Ismael Luceno

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  In addition, as a special exception, the copyright holders give
  permission to link the code of portions of this program with the
  OpenSSL library under certain conditions as described in each
  individual source file, and distribute linked combinations including
  the two.

  You must obey the GNU General Public License in all respects for all
  of the code used other than OpenSSL. If you modify file(s) with this
  exception, you may extend this exception to your version of the
  file(s), but you are not obligated to do so. If you do not wish to do
  so, delete this exception statement from your version. If you delete
  this exception statement from all source files in the program, then
  also delete it here.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* Text interface */

#include "axel.h"

#include <sys/ioctl.h>


static void stop(int signal);
static char *size_human(long long int value);
static char *time_human(int value);
static void print_commas(long long int bytes_done);
static void print_alternate_output(axel_t *axel);
static void print_help();
static void print_version();
static int get_term_width();

int run = 1;

#define MAX_REDIR_OPT	256

#ifdef NOGETOPTLONG
#define getopt_long(a, b, c, d, e) getopt(a, b, c)
#else
static struct option axel_options[] = {
	/* name             has_arg flag  val */
	{"max-speed",       1,      NULL, 's'},
	{"num-connections", 1,      NULL, 'n'},
	{"max-redirect",    1,      NULL, MAX_REDIR_OPT},
	{"output",          1,      NULL, 'o'},
	{"search",          2,      NULL, 'S'},
	{"ipv4",            0,      NULL, '4'},
	{"ipv6",            0,      NULL, '6'},
	{"no-proxy",        0,      NULL, 'N'},
	{"quiet",           0,      NULL, 'q'},
	{"verbose",         0,      NULL, 'v'},
	{"help",            0,      NULL, 'h'},
	{"version",         0,      NULL, 'V'},
	{"alternate",       0,      NULL, 'a'},
	{"insecure",        0,      NULL, 'k'},
	{"no-clobber",      0,      NULL, 'c'},
	{"header",          1,      NULL, 'H'},
	{"user-agent",      1,      NULL, 'U'},
	{"timeout",         1,      NULL, 'T'},
	{NULL,              0,      NULL, 0}
};
#endif

/* For returning string values from functions */
static char string[MAX_STRING + 3];

int
main(int argc, char *argv[])
{
	char fn[MAX_STRING] = "";
	int do_search = 0;
	search_t *search;
	conf_t conf[1];
	axel_t *axel;
	int i, j, cur_head = 0, ret = 1;
	char *s;

/* Set up internationalization (i18n) */
#ifdef ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);
#endif

	if (!conf_init(conf)) {
		return 1;
	}

	opterr = 0;

	j = -1;
	while (1) {
		int option;

		option =
		    getopt_long(argc, argv, "s:n:o:S::46NqvhVakcH:U:T:",
				axel_options, NULL);
		if (option == -1)
			break;

		switch (option) {
		case 'U':
			strncpy(conf->user_agent, optarg,
				sizeof(conf->user_agent) - 1);
			break;
		case 'H':
			strncpy(conf->add_header[cur_head++], optarg,
				sizeof(conf->add_header[cur_head - 1]) - 1);
			break;
		case 's':
			if (!sscanf(optarg, "%i", &conf->max_speed)) {
				print_help();
				goto free_conf;
			}
			break;
		case 'n':
			if (!sscanf(optarg, "%hu", &conf->num_connections)) {
				print_help();
				goto free_conf;
			}
			break;
		case MAX_REDIR_OPT:
			if (!sscanf(optarg, "%i", &conf->max_redirect)) {
				print_help();
				return 1;
			}
			break;
		case 'o':
			strncpy(fn, optarg, sizeof(fn) - 1);
			fn[sizeof(fn) - 1] = '\0';
			break;
		case 'S':
			do_search = 1;
			if (optarg) {
				if (!sscanf(optarg, "%i", &conf->search_top)) {
					print_help();
					goto free_conf;
				}
			}
			break;
		case '6':
			conf->ai_family = AF_INET6;
			break;
		case '4':
			conf->ai_family = AF_INET;
			break;
		case 'a':
			conf->alternate_output = 1;
			break;
		case 'k':
			conf->insecure = 1;
			break;
		case 'c':
			conf->no_clobber = 1;
			break;
		case 'N':
			*conf->http_proxy = 0;
			break;
		case 'h':
			print_help();
			ret = 0;
			goto free_conf;
		case 'v':
			if (j == -1)
				j = 1;
			else
				j++;
			break;
		case 'V':
			print_version();
			ret = 0;
			goto free_conf;
		case 'q':
			close(1);
			conf->verbose = -1;
			if (open("/dev/null", O_WRONLY) != 1) {
				fprintf(stderr,
					_("Can't redirect stdout to /dev/null.\n"));
				goto free_conf;
			}
			break;
		case 'T':
			conf->io_timeout = strtoul(optarg, NULL, 0);
			break;
		default:
			print_help();
			goto free_conf;
		}
	}
	conf->add_header_count = cur_head;

	/* disable alternate output and verbosity when quiet is specified */
	if (conf->verbose < 0)
		conf->alternate_output = 0;
	else if (j > -1)
		conf->verbose = j;

	if (conf->num_connections < 1) {
		print_help();
		goto free_conf;
	}

	if (conf->max_redirect < 0) {
		print_help();
		return 1;
	}
#ifdef HAVE_SSL
	ssl_init(conf);
#endif				/* HAVE_SSL */

	if (argc - optind == 0) {
		print_help();
		goto free_conf;
	} else if (strcmp(argv[optind], "-") == 0) {
		s = malloc(MAX_STRING);
		if (!s)
			goto free_conf;

		if (scanf("%1024[^\n]s", s) != 1) {
			fprintf(stderr,
				_("Error when trying to read URL (Too long?).\n"));
			free(s);
			goto free_conf;
		}
	} else {
		s = argv[optind];
		if (strlen(s) > MAX_STRING) {
			fprintf(stderr,
				_("Can't handle URLs of length over %d\n"),
				MAX_STRING);
			goto free_conf;
		}
	}

	printf(_("Initializing download: %s\n"), s);
	if (do_search) {
		search = malloc(sizeof(search_t) * (conf->search_amount + 1));
		if (!search)
			goto free_conf;

		memset(search, 0, sizeof(search_t) * (conf->search_amount + 1));
		search[0].conf = conf;
		if (conf->verbose)
			printf(_("Doing search...\n"));
		i = search_makelist(search, s);
		if (i < 0) {
			fprintf(stderr, _("File not found\n"));
			goto free_conf;
		}
		if (conf->verbose)
			printf(_("Testing speeds, this can take a while...\n"));
		j = search_getspeeds(search, i);
		if (j < 0) {
			fprintf(stderr, _("Speed testing failed\n"));
			return 1;
		}

		search_sortlist(search, i);
		if (conf->verbose) {
			printf(_("%i usable servers found, will use these URLs:\n"),
			       j);
			j = min(j, conf->search_top);
			printf("%-60s %15s\n", "URL", _("Speed"));
			for (i = 0; i < j; i++)
				printf("%-70.70s %5i\n", search[i].url,
				       search[i].speed);
			printf("\n");
		}
		axel = axel_new(conf, j, search);
		free(search);
		if (!axel || axel->ready == -1) {
			print_messages(axel);
			goto close_axel;
		}
	} else if (argc - optind == 1) {
		axel = axel_new(conf, 0, s);
		if (!axel || axel->ready == -1) {
			print_messages(axel);
			goto close_axel;
		}
	} else {
		search = malloc(sizeof(search_t) * (argc - optind));
		if (!search)
			goto free_conf;

		memset(search, 0, sizeof(search_t) * (argc - optind));
		for (i = 0; i < (argc - optind); i++)
			strncpy(search[i].url, argv[optind + i],
				sizeof(search[i].url) - 1);
		axel = axel_new(conf, argc - optind, search);
		free(search);
		if (!axel || axel->ready == -1) {
			print_messages(axel);
			goto close_axel;
		}
	}
	print_messages(axel);
	if (s != argv[optind]) {
		free(s);
	}

	if (*fn) {
		struct stat buf;

		if (stat(fn, &buf) == 0) {
			if (S_ISDIR(buf.st_mode)) {
				size_t fnlen = strlen(fn);
				size_t axelfnlen = strlen(axel->filename);

				if (fnlen + 1 + axelfnlen + 1 > MAX_STRING) {
					fprintf(stderr,
						_("Filename too long!\n"));
					goto close_axel;
				}

				fn[fnlen] = '/';
				memcpy(fn + fnlen + 1, axel->filename,
				       axelfnlen);
				fn[fnlen + 1 + axelfnlen] = '\0';
			}
		}
		sprintf(string, "%s.st", fn);
		if (access(fn, F_OK) == 0 && access(string, F_OK) != 0) {
			fprintf(stderr, _("No state file, cannot resume!\n"));
			goto close_axel;
		}
		if (access(string, F_OK) == 0 && access(fn, F_OK) != 0) {
			printf(_("State file found, but no downloaded data. Starting from scratch.\n"));
			unlink(string);
		}
		strcpy(axel->filename, fn);
	} else {
		/* Local file existence check */
		i = 0;
		s = axel->filename + strlen(axel->filename);
		while (1) {
			sprintf(string, "%s.st", axel->filename);
			if (access(axel->filename, F_OK) == 0) {
				if (axel->conn[0].supported) {
					if (access(string, F_OK) == 0)
						break;
				}
			} else {
				if (access(string, F_OK))
					break;
			}
			sprintf(s, ".%i", i);
			i++;
		}
	}

	if (!axel_open(axel)) {
		print_messages(axel);
		goto close_axel;
	}
	print_messages(axel);
	axel_start(axel);
	print_messages(axel);

	if (conf->alternate_output) {
		putchar('\n');
	} else {
		if (axel->bytes_done > 0) {	/* Print first dots if resuming */
			putchar('\n');
			print_commas(axel->bytes_done);
		}
	}
	axel->start_byte = axel->bytes_done;

	/* Install save_state signal handler for resuming support */
	signal(SIGINT, stop);
	signal(SIGTERM, stop);

	while (!axel->ready && run) {
		long long int prev;

		prev = axel->bytes_done;
		axel_do(axel);

		if (conf->alternate_output) {
			if (!axel->message && prev != axel->bytes_done)
				print_alternate_output(axel);
		} else {
			/* The infamous wget-like 'interface'.. ;) */
			long long int done =
			    (axel->bytes_done / 1024) - (prev / 1024);
			if (done && conf->verbose > -1) {
				for (i = 0; i < done; i++) {
					i += (prev / 1024);
					if ((i % 50) == 0) {
						if (prev >= 1024)
							printf("  [%6.1fKB/s]",
							       (double)axel->bytes_per_second /
							       1024);
						if (axel->size == LLONG_MAX)
							printf("\n[ N/A]  ");
						else if (axel->size < 10240000)
							printf("\n[%3lld%%]  ",
							       min(100,
								   102400 * i /
								   axel->size));
						else
							printf("\n[%3lld%%]  ",
							       min(100,
								   i /
								   (axel->size /
								    102400)));
					} else if ((i % 10) == 0) {
						putchar(' ');
					}
					putchar('.');
					i -= (prev / 1024);
				}
				fflush(stdout);
			}
		}

		if (axel->message) {
			if (conf->alternate_output == 1) {
				/* clreol-simulation */
				putchar('\r');
				for (i = get_term_width(); i > 0; i--)
					putchar(' ');
				putchar('\r');
			} else {
				putchar('\n');
			}
			print_messages(axel);
			if (!axel->ready) {
				if (conf->alternate_output != 1)
					print_commas(axel->bytes_done);
				else
					print_alternate_output(axel);
			}
		} else if (axel->ready) {
			putchar('\n');
		}
	}

	strcpy(string + MAX_STRING / 2,
	       size_human(axel->bytes_done - axel->start_byte));

	printf(_("\nDownloaded %s in %s. (%.2f KB/s)\n"),
	       string + MAX_STRING / 2,
	       time_human(gettime() - axel->start_time),
	       (double)axel->bytes_per_second / 1024);

	ret = axel->ready ? 0 : 2;

 close_axel:
	axel_close(axel);
 free_conf:
	conf_free(conf);

	return ret;
}

/* SIGINT/SIGTERM handler */
void
stop(int signal)
{
	(void)signal;
	run = 0;
}

/* Convert a number of bytes to a human-readable form */
char *
size_human(long long int value)
{
	if (value < 1024)
		sprintf(string, _("%lld byte"), value);
	else if (value < 1024 * 1024)
		sprintf(string, _("%.1f Kilobyte"), (float)value / 1024);
	else if (value < 1024 * 1024 * 1024)
		sprintf(string, _("%.1f Megabyte"),
			(float)value / (1024 * 1024));
	else
		sprintf(string, _("%.1f Gigabyte"),
			(float)value / (1024 * 1024 * 1024));

	return string;
}

/* Convert a number of seconds to a human-readable form */
char *
time_human(int value)
{
	if (value == 1)
		sprintf(string, _("%i second"), value);
	else if (value < 60)
		sprintf(string, _("%i seconds"), value);
	else if (value < 3600)
		sprintf(string, _("%i:%02i minute(s)"), value / 60, value % 60);
	else
		sprintf(string, _("%i:%02i:%02i hour(s)"), value / 3600,
			(value / 60) % 60, value % 60);

	return string;
}

/* Part of the infamous wget-like interface. Just put it in a function
	because I need it quite often.. */
void
print_commas(long long int bytes_done)
{
	int i, j;

	printf("       ");
	j = (bytes_done / 1024) % 50;
	if (j == 0)
		j = 50;
	for (i = 0; i < j; i++) {
		if ((i % 10) == 0)
			putchar(' ');
		putchar(',');
	}
	fflush(stdout);
}

static void
print_alternate_output_progress(axel_t *axel, char *progress, int width,
				long long int done, long long int total,
				double now)
{
	for (int i = 0; i < axel->conf->num_connections; i++) {
		int offset = ((double)(axel->conn[i].currentbyte) / (total + 1)
			      * (width + 1));

		if (axel->conn[i].currentbyte < axel->conn[i].lastbyte) {
			if (now <= axel->conn[i].last_transfer
				   + axel->conf->connection_timeout / 2) {
				if (i < 10)
					progress[offset] = i + '0';
				else
					progress[offset] = i - 10 + 'A';
			} else
				progress[offset] = '#';
		}
		int end = ((double)(axel->conn[i].lastbyte) / (total + 1)
			   * (width + 1));
		for (int j = offset + 1; j < end; j++)
			progress[j] = ' ';
	}

	progress[width] = '\0';
	printf("\r[%3ld%%] [%s", min(100, (long)(done * 100. / total + .5)),
	       progress);
}

static void
print_alternate_output(axel_t *axel)
{
	long long int done = axel->bytes_done;
	long long int total = axel->size;
	double now = gettime();
	int width = get_term_width();
	char *progress;

	if (width < 40) {
		fprintf(stderr,
			_("Can't setup alternate output. Deactivating.\n"));
		axel->conf->alternate_output = 0;

		return;
	}

	width -= 30;
	progress = malloc(width + 1);
	if (!progress)
		return;

	memset(progress, '.', width);

	if (total != LLONG_MAX) {
		print_alternate_output_progress(axel, progress, width, done,
						total, now);
	} else {
		progress[width] = '\0';
		printf("\r[ N/A] [%s", progress);
	}

	if (axel->bytes_per_second > 1048576)
		printf("] [%6.1fMB/s]",
		       (double)axel->bytes_per_second / (1024 * 1024));
	else if (axel->bytes_per_second > 1024)
		printf("] [%6.1fKB/s]", (double)axel->bytes_per_second / 1024);
	else
		printf("] [%6.1fB/s]", (double)axel->bytes_per_second);

	if (total != LLONG_MAX && done < total) {
		int seconds, minutes, hours, days;
		seconds = axel->finish_time - now;
		minutes = seconds / 60;
		seconds -= minutes * 60;
		hours = minutes / 60;
		minutes -= hours * 60;
		days = hours / 24;
		hours -= days * 24;
		if (days)
			printf(" [%2dd%2d]", days, hours);
		else if (hours)
			printf(" [%2dh%02d]", hours, minutes);
		else
			printf(" [%02d:%02d]", minutes, seconds);
	}

	fflush(stdout);

	free(progress);
}

static int
get_term_width()
{
	struct winsize w;

	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	return w.ws_col;
}

void
print_help()
{
#ifdef NOGETOPTLONG
	printf(_("Usage: axel [options] url1 [url2] [url...]\n"
		 "\n"
		 "-s x\tSpecify maximum speed (bytes per second)\n"
		 "-n x\tSpecify maximum number of connections\n"
		 "-o f\tSpecify local output file\n"
		 "-S[n]\tSearch for mirrors and download from n servers\n"
		 "-4\tUse the IPv4 protocol\n"
		 "-6\tUse the IPv6 protocol\n"
		 "-H x\tAdd HTTP header string\n"
		 "-U x\tSet user agent\n"
		 "-N\tJust don't use any proxy server\n"
		 "-k\tDon't verify the SSL certificate\n"
		 "-c\tSkip download if file already exists\n"
		 "-q\tLeave stdout alone\n"
		 "-v\tMore status information\n"
		 "-a\tAlternate progress indicator\n"
		 "-h\tThis information\n"
		 "-T x\tSet I/O and connection timeout\n"
		 "-V\tVersion information\n"
		 "\n"
		 "Visit https://github.com/axel-download-accelerator/axel/issues\n"));
#else
	printf(_("Usage: axel [options] url1 [url2] [url...]\n"
		 "\n"
		 "--max-speed=x\t\t-s x\tSpecify maximum speed (bytes per second)\n"
		 "--num-connections=x\t-n x\tSpecify maximum number of connections\n"
		 "--max-redirect=x\t\tSpecify maximum number of redirections\n"
		 "--output=f\t\t-o f\tSpecify local output file\n"
		 "--search[=n]\t\t-S[n]\tSearch for mirrors and download from n servers\n"
		 "--ipv4\t\t\t-4\tUse the IPv4 protocol\n"
		 "--ipv6\t\t\t-6\tUse the IPv6 protocol\n"
		 "--header=x\t\t-H x\tAdd HTTP header string\n"
		 "--user-agent=x\t\t-U x\tSet user agent\n"
		 "--no-proxy\t\t-N\tJust don't use any proxy server\n"
		 "--insecure\t\t-k\tDon't verify the SSL certificate\n"
		 "--no-clobber\t\t-c\tSkip download if file already exists\n"
		 "--quiet\t\t\t-q\tLeave stdout alone\n"
		 "--verbose\t\t-v\tMore status information\n"
		 "--alternate\t\t-a\tAlternate progress indicator\n"
		 "--help\t\t\t-h\tThis information\n"
		 "--timeout=x\t\t-T x\tSet I/O and connection timeout\n"
		 "--version\t\t-V\tVersion information\n"
		 "\n"
		 "Visit https://github.com/axel-download-accelerator/axel/issues to report bugs\n"));
#endif
}

void
print_version()
{
	printf(_("\nAxel version " VERSION " (" ARCH ")\n"));
	printf("\nCopyright 2001-2007 Wilmer van der Gaast,");
	printf("\n          2007-2009 Giridhar Appaji Nag,");
	printf("\n          2008-2010 Philipp Hagemeister,");
	printf("\n          2015-2017 Joao Eriberto Mota Filho,");
	printf("\n          2016-2017 Stephen Thirlwall,");
	printf("\n          2017      Ismael Luceno,");
	printf("\n          2017      Antonio Quartulli,");
	printf(_("\n                    and others."));
	printf(_("\nPlease, see the CREDITS file.\n\n"));
}

/* Print any message in the axel structure */
void
print_messages(axel_t *axel)
{
	message_t *m;

	if (!axel)
		return;

	while ((m = axel->message)) {
		printf("%s\n", m->text);
		axel->message = m->next;
		free(m);
	}
}
