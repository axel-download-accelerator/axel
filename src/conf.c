/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2001-2007 Wilmer van der Gaast
  Copyright 2008      Philipp Hagemeister
  Copyright 2008      Y Giridhar Appaji Nag
  Copyright 2016      Ivan Gimenez
  Copyright 2016      Stephen Thirlwall
  Copyright 2017      Antonio Quartulli
  Copyright 2017-2019 Ismael Luceno
  Copyright 2019      Terry

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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* Configuration handling file */

#include "config.h"
#include "axel.h"

/* Some nifty macro's.. */
#define MATCH if (0);
#define KEY(name)			\
	else if (!strcmp(key, #name))	\
		dst = &conf->name;

static int parse_interfaces(conf_t *conf, char *s);

#ifdef __GNUC__
__attribute__((format(scanf, 2, 3)))
#endif /* __GNUC__ */
static int
axel_fscanf(FILE *fp, const char *format, ...)
{
	va_list params;
	int ret;

	va_start(params, format);
	ret = vfscanf(fp, format, params);
	va_end(params);

	ret = !(ret == EOF && ferror(fp));
	if (!ret) {
		fprintf(stderr, _("I/O error while reading config file: %s\n"),
			strerror(errno));
	}

	return ret;
}

static int
parse_progress_style(conf_t *conf, const char *value)
{
	if (!strcasecmp(value, "auto")) {/* no-op */}
	else if (!strcasecmp(value, "alternative"))
		conf->progress_style = AXEL_PROGRESS_STYLE_ALTERNATIVE;
	else if (!strcasecmp(value, "classic"))
		conf->progress_style = AXEL_PROGRESS_STYLE_CLASSIC;
	else if (!strcasecmp(value, "percent"))
		conf->progress_style = AXEL_PROGRESS_STYLE_PERCENTAGE;
	else
		fprintf(stderr, _("Unknown progress bar style \"%s\"\n"), value);
	return 1;
}

static int
parse_protocol(conf_t *conf, const char *value)
{
	if (strcasecmp(value, "ipv4") == 0)
		conf->ai_family = AF_INET;
	else if (strcasecmp(value, "ipv6") == 0)
		conf->ai_family = AF_INET6;
	else {
		fprintf(stderr, _("Unknown protocol \"%s\"\n"), value);
		return 0;
	}

	return 1;
}

int
conf_loadfile(conf_t *conf, const char *file)
{
	int line = 0, ret = 1;
	FILE *fp;
	char s[MAX_STRING], key[MAX_STRING];

	fp = fopen(file, "r");
	if (fp == NULL)
		return 1;	/* Not a real failure */

	while (!feof(fp)) {
		char *tmp, *value = NULL;
		void *dst;

		line++;

		*s = 0;

		if (!(ret = axel_fscanf(fp, "%100[^\n#]s", s)))
			break;
		if (!(ret = axel_fscanf(fp, "%*[^\n]s")))
			break;
		if ((fgetc(fp) != '\n') && !feof(fp)) {	/* Skip newline */
			fprintf(stderr, "Expected newline\n");
			goto error;
		}
		tmp = strchr(s, '=');
		if (tmp == NULL)
			continue;	/* Probably empty? */

		sscanf(s, "%[^= \t]s", key);

		/* Skip the "=" and any spaces following it */
		while (isspace(*++tmp));	/* XXX isspace('\0') is false */
		value = tmp;
		/* Get to the end of the value string */
		while (*tmp && !isspace(*tmp))
			tmp++;
		*tmp = '\0';

		/* String options */
		MATCH
			KEY(default_filename)
			KEY(http_proxy)
			KEY(no_proxy)
		else
			goto num_keys;

		/* Save string option */
		strlcpy(dst, value, MAX_STRING);
		continue;

		/* Numeric options */
 num_keys:
		MATCH
			KEY(strip_cgi_parameters)
			KEY(save_state_interval)
			KEY(connection_timeout)
			KEY(reconnect_delay)
			KEY(max_redirect)
			KEY(buffer_size)
			KEY(max_speed)
			KEY(verbose)
			KEY(insecure)
			KEY(no_clobber)
			KEY(location_trusted)
			KEY(search_timeout)
			KEY(search_threads)
			KEY(search_amount)
			KEY(search_top)
		else
			goto long_num_keys;

		/* Save numeric option */
		*((int *)dst) = atoi(value);
		continue;

		/* Long numeric options */
 long_num_keys:
		MATCH
			KEY(max_speed)
		else
			goto other_keys;

		/* Save numeric option */
		*((unsigned long long *)dst) = strtoull(value, NULL, 10);
		continue;

 other_keys:
		/* Option defunct but shouldn't be an error */
		if (strcmp(key, "speed_type") == 0)
			continue;
		else if (strcmp(key, "interfaces") == 0) {
			if (parse_interfaces(conf, value))
				continue;
		} else if (strcmp(key, "progress_style") == 0) {
			if (parse_progress_style(conf, value))
				continue;
		} else if (strcmp(key, "use_protocol") == 0) {
			if (parse_protocol(conf, value))
				continue;
		} else if (strcmp(key, "num_connections") == 0) {
			int num = atoi(value);

			if (num <= USHRT_MAX) {
				conf->num_connections = num;
				continue;
			}

			fprintf(stderr,
				_("Requested too many connections, max is %i\n"),
				USHRT_MAX);
		} else if (!strcmp(key, "user_agent")) {
			conf_hdr_make(conf->add_header[HDR_USER_AGENT],
				      "User-Agent", DEFAULT_USER_AGENT);
			continue;
		} else if (!strcmp(key, "netrc")) {
			conf_netrc_set(conf, value);
			continue;
		}
#if 0
		/* FIXME broken code */
		get_config_number(add_header_count);
		for (int i = 0; i < conf->add_header_count; i++)
			get_config_string(add_header[i]);
#endif

error:
		fprintf(stderr, _("Error in %s line %i.\n"), file, line);
		ret = 0;
		break;
	}

	fclose(fp);
	return ret;
}

/* The headers a user gives with -H are meant for the host they named.  A
 * cookie or an authorization line is a credential, and a redirect can point
 * anywhere, so those stop travelling once the download leaves that host --
 * otherwise a shortened or hijacked link walks off with the session. */
bool
conf_header_is_private(const char *header)
{
	static const char *const names[] = {
		"cookie",
		"authorization",
		"proxy-authorization",
	};

	for (size_t i = 0; i < sizeof(names) / sizeof(*names); i++) {
		size_t len = strlen(names[i]);

		/* The name, then whatever space was typed, then the colon */
		if (strncasecmp(header, names[i], len) == 0 &&
		    header[len + strspn(header + len, " \t")] == ':')
			return true;
	}

	return false;
}

/* May those headers follow a redirect from one place to another?
 *
 * They were given for the host the user named, so a redirect to any other
 * one leaves them behind.  The port is not part of the question -- a cookie
 * is not scoped to one -- but leaving TLS is, since that would put the
 * credential on the wire in the clear.  A step up to TLS is the same host,
 * better protected, and keeps them. */
bool
conf_credentials_may_follow(int from_proto, const char *from_host,
			    int to_proto, const char *to_host)
{
	if (strcasecmp(from_host, to_host) != 0)
		return false;

	return !PROTO_IS_SECURE(from_proto) || PROTO_IS_SECURE(to_proto);
}

bool
conf_has_private_headers(const conf_t *conf)
{
	for (int i = 0; i < conf->add_header_count; i++)
		if (conf_header_is_private(conf->add_header[i]))
			return true;

	return false;
}

int
conf_init(conf_t *conf)
{
	char *s2;
	int i;

	/* Set defaults */
	memset(conf, 0, sizeof(conf_t));
	strlcpy(conf->default_filename, "default",
		sizeof(conf->default_filename));
	*conf->http_proxy = 0;
	*conf->no_proxy = 0;
	conf->strip_cgi_parameters = 1;
	conf->save_state_interval = 10;
	conf->connection_timeout = 45;
	conf->reconnect_delay = 20;
	conf->num_connections = 4;
	conf->max_redirect = MAX_REDIRECT;
	conf->io_timeout = DEFAULT_IO_TIMEOUT;
	conf->buffer_size = 5120;
	conf->max_speed = 0;
	conf->verbose = 1;
	conf->insecure = 0;
	conf->no_clobber = 0;

	conf->search_timeout = 10;
	conf->search_threads = 3;
	conf->search_amount = 15;
	conf->search_top = 3;

	conf->ai_family = AF_UNSPEC;

	conf_hdr_make(conf->add_header[HDR_USER_AGENT],
		      "User-Agent", DEFAULT_USER_AGENT);
	conf->add_header_count = HDR_count_init;

	conf->interfaces = calloc(1, sizeof(*conf->interfaces));
	if (!conf->interfaces)
		return 0;

	conf->interfaces->next = conf->interfaces;

	/* Detect if stdout is a tty, set the default indicator to alternate.
	 * Otherwise, keep it to original. */
	conf->progress_style = isatty(STDOUT_FILENO)
		? AXEL_PROGRESS_STYLE_ALTERNATIVE
		: AXEL_PROGRESS_STYLE_CLASSIC;

	if ((s2 = getenv("http_proxy")) || (s2 = getenv("HTTP_PROXY")))
		strlcpy(conf->http_proxy, s2, sizeof(conf->http_proxy));

	if (!conf_loadfile(conf, ETCDIR "/axelrc"))
		return 0;

	if ((s2 = getenv("HOME")) != NULL) {
		char s[MAX_STRING];
		int ret;

		ret = snprintf(s, sizeof(s), "%s/.axelrc", s2);
		if (ret >= (int)sizeof(s)) {
			fprintf(stderr, _("HOME env variable too long\n") );
			return 0;
		}

		if (!conf_loadfile(conf, s))
			return 0;
	}

	/* Convert no_proxy to a 0-separated-and-00-terminated list.. */
	for (i = 0; conf->no_proxy[i]; i++)
		if (conf->no_proxy[i] == ',')
			conf->no_proxy[i] = 0;
	conf->no_proxy[i + 1] = 0;

	return 1;
}

/**
 * Select the .netrc file to take credentials from.
 *
 * An empty path selects the default location, and a NULL one disables
 * the lookup altogether.
 */
void
conf_netrc_set(conf_t *conf, const char *path)
{
	netrc_free(conf->netrc);
	conf->netrc = path ? netrc_init(path) : NULL;
}

/* release resources allocated by conf_init() */
void
conf_free(conf_t *conf)
{
	netrc_free(conf->netrc);
	free(conf->interfaces);
}

/**
 * Fill in the credentials to use for a connection.
 *
 * Credentials obtained from somewhere else, e.g. the URL itself, are
 * kept; otherwise the configured authentication sources are consulted,
 * and auto-login is used as a last resort.
 */
void
conf_auth_setup(conf_t *conf, int proto, const char *host,
		char *user, size_t user_len, char *pass, size_t pass_len)
{
	if (*user || *pass || (conf && conf->untrusted_host))
		return;

	netrc_parse(conf ? conf->netrc : NULL, host, user, user_len,
		    pass, pass_len);
	if (*user)
		return;

	if (PROTO_IS_FTP(proto)) {
		/* Dash the password: Save traffic by trying
		   to avoid multi-line responses */
		strlcpy(user, "anonymous", user_len);
		strlcpy(pass, "mailto:axel@axel.project", pass_len);
	}
}

static
int
parse_interfaces(conf_t *conf, char *s)
{
	char *s2;
	axel_if_t *iface;

	iface = conf->interfaces->next;
	while (iface != conf->interfaces) {
		axel_if_t *i;

		i = iface->next;
		free(iface);
		iface = i;
	}
	free(conf->interfaces);

	if (!*s) {
		conf->interfaces = calloc(1, sizeof(*conf->interfaces));
		if (!conf->interfaces)
			return 0;

		conf->interfaces->next = conf->interfaces;
		return 1;
	}

	conf->interfaces = iface = malloc(sizeof(*iface));
	if (!conf->interfaces)
		return 0;

	while (1) {
		while ((*s == ' ' || *s == '\t') && *s)
			s++;
		for (s2 = s; *s2 != ' ' && *s2 != '\t' && *s2; s2++) ;
		*s2 = 0;
		if (*s < '0' || *s > '9')
			get_if_ip(iface->text, sizeof(iface->text), s);
		else
			strlcpy(iface->text, s, sizeof(iface->text));
		s = s2 + 1;
		if (*s) {
			iface->next = malloc(sizeof(*iface));
			if (!iface->next)
				return 0;

			iface = iface->next;
		} else {
			iface->next = conf->interfaces;
			break;
		}
	}

	return 1;
}
