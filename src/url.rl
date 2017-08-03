/* url_parser.rl -- URL parser for Axel
 *
 * Copyright 2017 Ismael Luceno <ismael@linux.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include "axel.h"
#include "url.h"

#define PROTO_DEFAULT PROTO_HTTP

%%{
machine url;

scheme =
	( "http" >{ _proto = PROTO_PROTO_HTTP; }
	| "ftp"  >{ _proto = PROTO_PROTO_FTP; }
	) ("s"   >{ _proto |= PROTO_SECURE; })?
	"://";

pct_encoded = "%" xdigit xdigit;
unreserved  = alnum | "-" | [._~];
sub_delims  = [!$&'()*+,;=];
pchar       = unreserved | pct_encoded | sub_delims | ":" | "@";

fname       = pchar* >{ f[URL_FNAME] = p; } %{ f[URL_FNAME+1] = p; };
dir         = "/"+ >{ f[URL_DIR] = p; } (pchar+ "/"+)* %{ f[URL_DIR+1] = p; };
path        = dir fname;
username    = (unreserved|pct_encoded|sub_delims)+
	>{ f[URL_USER] = p; } %{ f[URL_USER+1] = p; };
password    = (unreserved|pct_encoded|sub_delims)*
	>{ f[URL_PASS] = p; } %{ f[URL_PASS+1] = p; };
userinfo    = username (":" password)?;
dec_octet   = "0"* (digit{1,2} | "1" digit{2} | "2" ([0-4] digit | "5" [0-5]));
ipv4addr    = (dec_octet "."){3} dec_octet;
ipv6addr    = "[" ((xdigit+ ":"){7} | (xdigit+ ":"){,6} ":") xdigit+ "]";
hnlabel     = (alnum | "-")+;
proto_label = "www." >{ if (-1 == _proto) _proto = PROTO_PROTO_HTTP; }
	    | "ftp." >{ if (-1 == _proto) _proto = PROTO_PROTO_FTP; };
hostname    = proto_label? (hnlabel ".")* hnlabel "."?;
host        = (hostname | ipv4addr | ipv6addr)
	>{ f[URL_HOST] = p; } %{ f[URL_HOST+1] = p; };
port        = digit+ >{ f[URL_PORT] = p; };
hostport    = host (":" port)?;
main       := (scheme? (userinfo "@")? hostport path?) $!{ return p; };

}%%


const char *
parse_url(int *proto, const char **f, const char *p)
{
	const char *pe, *eof = p;
	int cs;
	int _proto = -1;

	pe = p + strlen(p);

	memset(f, 0, sizeof(*f) * URL_NFIELDS);

%%{
	write data noerror noentry nofinal;
	write init;
	write exec;
}%%
	*proto = _proto != -1 ? _proto : PROTO_DEFAULT;
	return NULL;
}
