/*
  Axel -- A lighter download accelerator for Linux and other Unices

  Copyright 2020    Jason

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

#include "config.h"

#include "axel.h"

/**
 * Load cookies from netscape HTTP cookie file.
 * https://curl.haxx.se/docs/http-cookies.html.
 *
 * @returns number of cookies.
 */
int
cookielist_loadfile(cookie_t *cookielist, FILE *fd)
{
    abuf_t *abuf;
    cookie_t *cookie = cookielist;
    size_t len, l;
    int count = 0;

    abuf = calloc(1, sizeof(abuf_t));
    if (!abuf)  {
        fprintf(stderr, _("Out of memory\n"));
        return 0;
    }

    if (abuf_setup(abuf, 1024) < 0) {
        fprintf(stderr, _("Out of memory\n"));
        goto nomem;
    }

    for (cookie_t *tmp = cookie; fgets(abuf->p, abuf->len, fd);) {
        len = strlen(abuf->p);
        if (len == abuf->len - 1) {
            for (;;) {
                if (abuf->len > MAX_COOKIE) {
                    printf(_("Unsupported cookie (Too long)\n"));
                    goto unsupported;
                }
                /* Realocate abuf */
                if(abuf_setup(abuf, abuf->len + COOKIE_CHUNK) < 0) {
                    fprintf(stderr, _("Out of memory\n"));
                    goto nomem;
                }
                /* Read more bytes, break when line ends */
                if (!fgets(abuf->p + len, COOKIE_CHUNK, fd))
                    break;
                l = strlen(abuf->p + len);
                len += l;
                if (l < COOKIE_CHUNK - 1) {
                    /* Resize abuf, this shall not fail */
                    abuf_setup(abuf, len);
                    break;
                }
            }
        }

        /* Ignore lines empty or start with "# " */
        if (abuf->p[0] == '\n' || (abuf->p[0] == '#' && abuf->p[1] == ' '))
            continue;

        /* Check if preallocated memory is used up */
        if (!cookie) {
            cookie = calloc(1, sizeof(cookie_t));
            if (!cookie) {
                fprintf(stderr, _("Out of memory\n"));
                goto nomem;
            }
            tmp->next = cookie;
        }
        /* Store values into cookie, ignore broken one */
#ifndef NDEBUG
        printf("Loading cookie line: %s\n", abuf->p);
#endif
        if (cookie_load(cookie, abuf) < 0)
            continue;

        tmp = cookie;
        cookie = tmp->next;

        count++;
    }

 unsupported:
 nomem:
    abuf_setup(abuf, ABUF_FREE);
    free(abuf);

    return count;
}

void
cookielist_header(abuf_t *abuf, const cookie_t *cookielist, int num)
{
    strlcpy(abuf->p, "Cookie:", abuf->len);
    const cookie_t *cookie = cookielist;
    // TODO simplify the code
    for (int i = 0; i < num; i++) {
        abuf_strcat(abuf, " ");
        abuf_strcat(abuf, cookie->name);
        abuf_strcat(abuf, "=");
        abuf_strcat(abuf, cookie->value);
        abuf_strcat(abuf, ";");
        cookie = cookie->next;
    }
}

inline void
cookie_free(cookie_t *cookie)
{
    free_if_exists(cookie->name);
    free_if_exists(cookie->value);
}

/* Free a cookie list. */
void
cookielist_free(cookie_t *cookielist, int num)
{
    cookie_t *tmp, *cur;
    tmp = cur = cookielist[COOKIE_PREALLOCATE_NUM - 1].next;

    num -= COOKIE_PREALLOCATE_NUM;
    if (num > 0) {
        for (int i = 0; i < num; i++) {
            tmp = tmp->next;
            cookie_free(cur);
            free(cur);
            cur = tmp;
        }
    }

    for (cur = cookielist;
         cur - cookielist < COOKIE_PREALLOCATE_NUM;
         cur++) {
        cookie_free(cur);
    }
}

int
cookie_strcpy(char *dst, const char *src, const char *space)
{
    int i;
    const char *s, *p;

    s = p = src;
    p = strpbrk(p, space);
    if (!p)
        return -1;
    i = p - s;
    dst = realloc(dst, i);
    if (!dst) {
        fprintf(stderr, _("Out of memory\n"));
        return -1;
    }
    memcpy(dst, s, i);
    dst[i] = 0;

    return 0;
}

int
cookie_setup(cookie_t *cookie)
{
    cookie->name = realloc(cookie->name, 0);
    if (!cookie->name)
        return -ENOMEM;
    cookie->value = realloc(cookie->value, 0);
    if (!cookie->value)
        return -ENOMEM;

    return 0;
}

/**
 * Load cookie from abuf
 * @returns 0 if OK, negative value on error.
 */
int
cookie_load(cookie_t *cookie, const abuf_t *abuf)
{
    const char space[] = " \t\n";
    char *s, *p;
    int i;

    /* Skipping unsupported fields */
    s = p = abuf->p;
    for (i = 0; (p = strpbrk(p, space)) && i < 5; i++, p = s) {
        s = p + strspn(p, space);
        if (!*s) {
            fprintf(stderr, _("Invalid cookie\n"));
            return -1;
        }
        *p = 0;
    }

    cookie_setup(cookie);

    /* Name field */
    if (cookie_strcpy(cookie->name, s, space) < 0)
        return -1;

    p = strpbrk(s, space);
    s = p + strspn(p, space);
    /* Value field */
    if (cookie_strcpy(cookie->value, s, space) < 0)
        return -1;

    return 0;
}
