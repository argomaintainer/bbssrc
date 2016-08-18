/*
 * string.c			-- there's some useful function about string
 *
 * of SEEDNetBBS generation 1 (libtool implement)
 *
 * Copyright (c) 1998, 1999, Edward Ping-Da Chuang <edwardc@edwardc.dhs.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * CVS: $Id: string.c,v 1.7 2008-08-03 17:57:14 bbs Exp $
 */

#ifdef BBS
#include "bbs.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>		/* for time_t prototype */
#include "libSystem.h"

#define chartoupper(c)  ((c >= 'a' && c <= 'z') ? c+'A'-'a' : c)
#define chartolower(c)  ((c >= 'A' && c <= 'Z') ? c-'A'+'a' : c)
#define is_alpha(ch) (((ch) >= 'A' && (ch) <= 'Z') || ((ch) >= 'a' && (ch) <= 'z'))
#endif

#ifndef _tolower
#define _tolower(c)	__tolower(c)
#endif

void
strtolower(char *dst, char *src)
{
	for (; *src; src++)
		*dst++ = chartolower(*src);
	*dst = '\0';
}

void
strtoupper(char *dst, char *src)
{
	for (; *src; src++)
		*dst++ = chartoupper(*src);
	*dst = '\0';
}

int
killwordsp(char *str)
{
	char *ptr;

	ptr = str;
	while (*ptr == ' ')
		ptr++;
	if (*ptr != '\0') {
		int i = 0;

		while (1) {
			str[i] = *ptr;
			if (*ptr == '\0')
				break;
			ptr++;
			i++;
		}
		ptr = str + i - 1;
		while (*ptr == ' ')
			ptr--;
		*(ptr + 1) = '\0';
		return 1;
	} else
		str[0] = '\0';
	return 0;
}

void
my_ansi_filter(char *source)
{
	int len = strlen(source);
	char result[len + 1];
	int i, flag = 0, loc = 0;

	len = strlen(source);
	for (i = 0; i < len; i++) {
		if (source[i] == '') {
			flag = 1;
			continue;
		} else if (flag == 1 && is_alpha(source[i])) {
			flag = 0;
			continue;
		} else if (flag == 1) {
			continue;
		} else {
			result[loc++] = source[i];
		}
	}
	result[loc] = '\0';
	strlcpy(source, result, loc + 1);
}

char *
ansi_filter(char *source)
{
	char *result, ch[3];
	int i, flag = 0, len = strlen(source);

	result = (char *) malloc((len + 10) * sizeof (char));

	for (i = 0; i < len; i++) {
		if (source[i] == '') {
			flag = 1;
			continue;
		} else if (flag == 1 && is_alpha(source[i])) {
			flag = 0;
			continue;
		} else if (flag == 1) {
			continue;
		} else {
			sprintf(ch, "%c", source[i]);
			strcat(result, ch);
		}
	}

	return (char *) result;
}

char *
Cdate(time_t *clock)
{
	static char foo[22];
	struct tm *mytm = localtime(clock);

	strftime(foo, 22, "%D %T %a", mytm);
	return (foo);
}

char *
strstr2(char *s, char *s2)
{
	char *p;
	int len;

	if ((p = (char *)strcasestr(s, s2)) == NULL)
		return NULL;

	len = strlen(s2);
	for (; p[0]; p++) {
		if (!strncasecmp(p, s2, len))
			return p;
		if ((p[0] & 0x80) && (p[1] > 31))
			p++;
	}
	return NULL;
}

char *
strstr2n(char *s, char *s2, size_t size)
{
	char *p;
	int i, len;

	if (s2 == NULL || size <= 0)
		return 0;

	len = strlen(s2);
	size = size - len + 1;

	for (p = s, i = size; i; i--) {
		if (*p == *s2 && !memcmp(p, s2, len))
			goto found;
		p++;
	}

	return NULL;

found:

	for (p = s, i = size; i; i--, p++) {
		if (!strncasecmp(p, s2, len))
			return p;
		if ((p[0] & 0x80) && (p[1] > 31))
			i--, p++;
	}
	return NULL;
}

void
fixstr(char *str, char *fixlist, char ch)
{
	char *p = str;

	while (*p) {
		if (strchr(fixlist, *p)) {
			*p = ch;
		}
		p++;
	}
}

void
trim(char *str)
{
	char *ptr;

	ptr = str + strlen(str) - 1;
	while (ptr >= str) {
		if (*ptr != ' ' && *ptr != '\t') {
			*(ptr + 1) = '\0';
			break;
		}
		--ptr;
	}

	ptr = str;
	while (*ptr != '\0') {
		if (*ptr != ' ' && *ptr != '\t') {
			if (ptr != str)
				memmove(str, ptr, strlen(ptr) + 1);
			break;
		}
		++ptr;
	}
}

/* monster: following functions from OpenBSD are introduced here to avoid security problems */

/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';	/* NUL-terminate dst */
		while (*s++) ;
	}

	return (s - src - 1);	/* count does not include NUL */
}

/*
 * Appends src to string dst of size siz (unlike strncat, siz is the
 * full size of dst, not space left).  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz <= strlen(dst)).
 * Returns strlen(src) + MIN(siz, strlen(initial dst)).
 * If retval >= siz, truncation occurred.
 */
size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return (dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return (dlen + (s - src));	/* count does not include NUL */
}

/*
 * My personal strstr() implementation that beats most other algorithms.
 * Until someone tells me otherwise, I assume that this is the
 * fastest implementation of strstr() in C.
 * I deliberately chose not to comment it.  You should have at least
 * as much fun trying to understand it, as I had to write it :-).
 *
 * Stephen R. van den Berg, berg@pool.informatik.rwth-aachen.de
*/

typedef unsigned chartype;

char *
strcasestr(const char *phaystack, const char *pneedle)
{
	register const unsigned char *haystack, *needle;
	register chartype b, c;

	haystack = (const unsigned char *) phaystack;
	needle = (const unsigned char *) pneedle;

  	b = _tolower (*needle);
  	if (b != '\0') {
      		haystack--;				/* possible ANSI violation */
      	do {
		c = *++haystack;
	  	if (c == '\0')
	    		goto ret0;
	} while (_tolower (c) != (int) b);

      	c = _tolower (*++needle);
      	if (c == '\0')
		goto foundneedle;
      	++needle;
      	goto jin;

      	for (;;) {
	  	register chartype a;
	  	register const unsigned char *rhaystack, *rneedle;

	  	do {
	      		a = *++haystack;
	      		if (a == '\0')
				goto ret0;
	      		if (_tolower (a) == (int) b)
				break;
	      		a = *++haystack;
	      		if (a == '\0')
				goto ret0;
		shloop:
	      		;
	    	} while (_tolower (a) != (int) b);

jin:	  
		a = *++haystack;
		if (a == '\0')
			goto ret0;

		if (_tolower (a) != (int) c)
	    		goto shloop;

		rhaystack = haystack-- + 1;
		rneedle = needle;
		a = _tolower (*rneedle);

		if (_tolower (*rhaystack) == (int) a)
			do {
				if (a == '\0')
		  			goto foundneedle;
				++rhaystack;
				a = _tolower (*++needle);
				if (_tolower (*rhaystack) != (int) a)
		  			break;
				if (a == '\0')
		  			goto foundneedle;
				++rhaystack;
				a = _tolower (*++needle);
	      		} while (_tolower (*rhaystack) == (int) a);

	  		needle = rneedle;		/* took the register-poor approach */

	  		if (a == '\0')
	    			break;
		}
    	}
foundneedle:
  	return (char*) haystack;
ret0:
  	return 0;
}



/*
  Author:	Pudding
  Date:		2004.10.12
*/

inline int
inset(char *set, char c)
{
	for (; *set != '\0' && *set != c; set++);
	return (!(*set == '\0'));
}

/*
  Search for the section sect in th string str
  Sections are delimited by char set delim
*/

char*
strsect(char *str, char *sect, char *delim)
{
	char *ptr = str;
	int len;
	int sect_len = strlen(sect);

	while (*ptr != '\0') {
		while (inset(delim, *ptr)) ptr++;
		len = 0;
		while (ptr[len] != '\0' && !inset(delim, ptr[len])) len++;
		if (len == sect_len && strncmp(ptr, sect, len) == 0) return ptr;
		ptr += len;
	}
	return NULL;				/* Section not found if we come here */
}
