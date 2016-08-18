/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU

    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
			Guy Vega, gtvega@seabass.st.usm.edu
			Dominic Tynes, dbtynes@seabass.st.usm.edu

    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
			Peng Piaw Foong, ppfoong@csie.ncu.edu.tw

    Firebird2000 Bulletin Board System
    Copyright (C) 1999-2001, deardragon, dragon@fb2000.dhs.org

    Puke Bulletin Board System
    Copyright (C) 2001-2002, Yu Chen, monster@marco.zsu.edu.cn
			     Bin Jie Lee, is99lbj@student.zsu.edu.cn

    Contains codes from YTHT & SMTH distributions of Firebird Bulletin
    Board System.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "libSystem.h"

#define SLIST_INCREMENT	20

slist *
slist_init(void)
{
	slist *sl;

	if ((sl = (slist *)malloc(sizeof(slist))) == NULL)
		return NULL;

	if ((sl->strs = (char **)malloc(SLIST_INCREMENT * sizeof(char *))) == NULL) {
		free(sl);
		return NULL;
	}

	sl->length = 0;
	sl->maxused = -1;
	sl->alloced = SLIST_INCREMENT;

	return sl;
}

void
slist_clear(slist *sl)
{
	int i;

	if (sl != NULL) {
		for (i = 0; i <= sl->maxused; i++)
			free(sl->strs[i]);
		sl->length = 0;
		sl->maxused = -1;
	}
}

void
slist_free(slist *sl)
{
	if (sl != NULL) {
		slist_clear(sl);
		free(sl->strs);
		free(sl);
	}
}

int
slist_add(slist *sl, const char *str)
{
	int i, idx = 0;
	char **tstrs, *tstr;

	if (sl == NULL)
		return -1;

	if (sl->length == sl->maxused + 1) {
		idx = sl->maxused + 1;
	} else {
		for (i = 0; i < sl->length; i++) {
			if (sl->strs[i] == NULL) {
				idx = i;
				break;
			}
		}
	}

	if (idx == sl->alloced) {
		if ((tstrs = (char **)realloc(sl->strs, (SLIST_INCREMENT + sl->length) * sizeof(char *))) == NULL)
			return -1;

		sl->strs = tstrs;
		sl->alloced += SLIST_INCREMENT;
	}

	if ((tstr = (char *)malloc(strlen(str) + 1)) == NULL)
		return -1;

	strcpy(tstr, str);
	sl->strs[idx] = tstr;
	sl->length++;
	if (idx > sl->maxused)
		sl->maxused = idx;
	return 0;
}

int
slist_remove(slist *sl, int idx)
{

#ifndef DISABLE_RANDOMACCESS
	int i;
#endif

	if (sl == NULL)
		return -1;

	if (idx < 0 || idx > sl->maxused)
		return -1;

	free(sl->strs[idx]);
	sl->strs[idx] = NULL;
	sl->length--;

#ifndef DISABLE_RANDOMACCESS
	for (i = idx + 1; i <= sl->maxused; i++)
		sl->strs[i - 1] = sl->strs[i];
	--sl->maxused;
#else
	if (idx == sl->maxused)
		sl->maxused = idx - 1;
#endif

	return 0;
}

int
slist_loadfromfile(slist *sl, const char *filename)
{
	FILE *fp;
	char buf[1024];

	if (sl == NULL)
		return -1;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;

	slist_clear(sl);
	while ((fgets(buf, 1024, fp)) != NULL) {
		if (buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0;

		if (buf[0] != 0) {
			if (sl == NULL) {
				if ((sl = slist_init()) == NULL)
					return -1;
			}

			slist_add(sl, buf);
		}
	}
	fclose(fp);
	return 0;
}

int
slist_savetofile(slist *sl, const char *filename)
{
	int i;
	FILE *fp;

	if (sl == NULL)
		return -1;

	if ((fp = fopen(filename, "w")) == NULL)
		return -1;

	for (i = 0; i <= sl->maxused; i++)
		if (sl->strs[i] != NULL)
			fprintf(fp, "%s\n", sl->strs[i]);

	fclose(fp);
	return 0;
}

char *
slist_next(slist *sl, int *cur)
{
	int i;

	if (sl == NULL || cur == NULL)
		return NULL;

	if ((i = *cur) < 0)
		i = -1;

	while (i <= sl->maxused) {
		if (sl->strs[++i] != NULL) {
			*cur = i;
			return sl->strs[i];
		}
	}

	return NULL;
}

char *
slist_prev(slist *sl, int *cur)
{
	int i;

	if (sl == NULL || cur == NULL || ((i = *cur) > sl->maxused))
		return NULL;

	while (i > 0) {
		if (sl->strs[--i] != NULL) {
			*cur = i;
			return sl->strs[i];
		}
	}

	return NULL;
}

int
slist_indexof(slist *sl, const char *str)
{
	int i;

	if (sl == NULL)
		return -1;

	for (i = 0; i < sl->length; i++)
		if (!strcasecmp(sl->strs[i], str))
			return i;

	return -1;
}
