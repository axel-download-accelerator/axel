# AXEL

#### Axel - Lightweight CLI download accelerator

## 1. Help this project ##

Axel needs your help. **If you are a programmer** and want to
help a nice project, this is your opportunity.

Axel was imported from its old repository[1] to GitHub (the original
homepage and developers are inactive).

If you are interested in helping Axel, please read the
[CONTRIBUTING.md](CONTRIBUTING.md) file.

Additionally, there is a group to discuss and to coordinate the
development process[3]. You can also find other developers in the
`#axel` channel on freenode.

[1]: https://alioth.debian.org/projects/axel
[2]: https://tracker.debian.org/pkg/axel
[3]: https://groups.google.com/forum/#!forum/axel-accelerator-dev

## 2. What is Axel? ##

Axel tries to accelerate the download process by using multiple
connections per file, and can also balance the load between
different servers.

Axel tries to be as light as possible, so it might be useful on
byte-critical systems.

Axel supports HTTP, HTTPS, FTP and FTPS protocols.

Thanks to the original developer of Axel, Wilmer van der Gaast, and everyone else who has contributed to it.

## Building from source ##

Release tarballs contain a pre-generated buildsystem, but if you need to
edit/patch it, or you're building from a copy of the repository, then you may
need to run `autoreconf -i` to generate it. Further instructions are provided in
the [INSTALL](INSTALL) file. The basic actions for most users are:

    ./configure && make && make install

To build without SSL/TLS support, use `./configure --without-ssl`.

### Dependencies for release tarballs ###

* `gettext` (or `gettext-tiny`)
* `pkg-config`

Optional:

* `libssl` (OpenSSL, LibreSSL or compatible) -- for SSL/TLS support.

### Extra dependencies for building from snapshots ###

* `autoconf-archive`
* `autoconf`
* `automake`
* `autopoint`
* `txt2man`

### Building on Ubuntu from Git ###

#### Packages ####

* `build-essential`
* `autoconf`
* `autoconf-archive`
* `automake`
* `autopoint`
* `gettext`
* `libssl-dev`
* `pkg-config`

### Build instructions ###

	$ autoreconf -fiv
	$ ./configure && make && sudo make install

## Mac OS X ##
### Install with Homebrew ###

    brew install axel

### Building ##

Install the following homebrew packages:

	brew install autoconf-archive automake gettext openssl

You'll need to provide some extra options to autotools so it can find gettext
and openssl.

	GETTEXT=/usr/local/opt/gettext
	OPENSSL=/usr/local/opt/openssl
	PATH="$GETTEXT/bin:$PATH"

	autoreconf -fiv -I$GETTEXT/share/aclocal/

	CFLAGS="-I$GETTEXT/include -I$OPENSSL/include" \
	LDFLAGS=-L$GETTEXT/lib ./configure

You can just run `make` as usual after these steps.

## Related projects ##

* [aria2](https://github.com/aria2/aria2)
* [hget](https://github.com/huydx/hget)
* [lftp](https://github.com/lavv17/lftp)
* [nugget](https://github.com/maxogden/nugget)
* [pget](https://github.com/Code-Hex/pget)

## License ##

Axel is licensed under GPL-2+ with the OpenSSL exception.
