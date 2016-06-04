# AXEL
**Light command line download accelerator for Linux and Unix**


<br><br>
**1. HELP THIS PROJECT**<br>
**2. WHAT IS AXEL?**<br>
**3. BUILDING FROM SOURCE**<br>
**4. BUILDING ON OS/X WITH HOMEBREW**<br>
**5. LICENSE**<br>



--------------------
1. HELP THIS PROJECT
--------------------

Axel needs your help. **If you are a programmer** and if you wants to
help a nice project, this is your opportunity.

My name is Eriberto and **I am not a C developer**. I imported Axel from
its old repository[1] to GitHub (the original homepage and developers
are inactive). After this, I applied all patches found in Debian project
and other places for this program. All my initial work was registered in
ChangeLog file (version 2.5 and later releases). I also maintain Axel
packaged in Debian[2].

If you are interested to help Axel, read the [CONTRIBUTING.md](CONTRIBUTING.md) file.

[1] https://alioth.debian.org/projects/axel<br>
[2] https://packages.qa.debian.org/a/axel.html<br>


----------------
2. WHAT IS AXEL?
----------------

Axel tries to accelerate the downloading process by using multiple
connections for one file, similar to DownThemAll and other famous
programs. It can also use multiple mirrors for one download.

Using Axel, you will get files faster from Internet. So, Axel can
speed up a download up to 60% (approximately, according to some tests).

Axel tries to be as light as possible, so it might be useful as a
wget clone (and other console based programs) on byte-critical systems.

Axel supports HTTP, HTTPS, FTP and FTPS protocols.

Axel was originally developed by Wilmer van der Gaast. Thanks for your
efforts. Over time, Axel got several contributions from people. Please,
see the files AUTHORS and CREDITS.


-----------------------
3. BUILDING FROM SOURCE
-----------------------

Run `./autogen.sh` to create the configure script, then proceed with the
instructions in [INSTALL](INSTALL). The basic actions for most users,
after ./autogen.sh, is running ./configure, make and make install.

To build without SSL/TLS support, use ./configure --without-openssl


---------------------------------
4. BUILDING ON OS/X WITH HOMEBREW
---------------------------------

Install the following homebrew packages:

  `homebrew install automake gettext openssl`

You'll need to provide some extra options to `autogen.sh` and `configure`
so they can find gettext and openssl.

```shell
GETTEXT=/usr/local/opt/gettext
OPENSSL=/usr/local/opt/openssl
PATH="$GETTEXT/bin:$PATH"
./autogen.sh -I$GETTEXT/share/aclocal/
CFLAGS="-I$GETTEXT/include -I$OPENSSL/include" LDFLAGS=-L$GETTEXT/lib ./configure
```

You can just run `make` as usual after these steps.


----------
5. LICENSE
----------

Axel is under GPL-2+ with OpenSSL exception.
