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

#include "bbs.h"

int
cmp_aid(const void *hdr1_ptr, const void *hdr2_ptr)
{
	struct fileheader *hdr1 = (struct fileheader *)hdr1_ptr;
	struct fileheader *hdr2 = (struct fileheader *)hdr2_ptr;

	if (hdr1->id > hdr2->id)
		return 1;
	if (hdr1->id < hdr2->id)
		return -1;

	return strcmp(hdr1->filename, hdr2->filename);
}

void
mark_thread(struct fileheader *tdir, int num)
{
	int i;

	if (num <= 0)
		return;

	tdir[0].reserved[0] = THREAD_BEGIN;

	if (num > 1) {
		for (i = 1; i < num - 1; i++) {
			if (tdir[i - 1].id == tdir[i].id && tdir[i].id != 0) {
				tdir[i].reserved[0] = (tdir[i].id == tdir[i + 1].id) ?
							THREAD_OTHER : THREAD_END;
			} else {
				tdir[i].reserved[0] = THREAD_BEGIN;
			}
		}
		tdir[num - 1].reserved[0] = (tdir[num - 2].id == tdir[num - 1].id) ?
					     THREAD_END : THREAD_BEGIN;
	}
}

int
make_thread(char *board, int force_refresh)
{
	int fd, result = 0;
	char dname[PATH_MAX + 1], tname[PATH_MAX + 1];
	struct stat st1, st2;
	struct fileheader *dir, *tdir;

	snprintf(dname, sizeof(dname), "boards/%s/"DOT_DIR, board);
	snprintf(tname, sizeof(tname), "boards/%s/"THREAD_DIR, board);

	if ((fd = open(dname, O_RDONLY)) == -1)
		return -1;

	if (fstat(fd, &st1) == -1) {
		close(fd);
		return -1;
	}

	if (force_refresh == NA && stat(tname, &st2) != -1) {
		if (st2.st_mtime > st1.st_mtime) {
			close(fd);
			return 0;
		}
	}

	dir = mmap(NULL, st1.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);

	if (dir == MAP_FAILED) {
		return -1;
	}

	 if ((tdir = malloc(st1.st_size)) != NULL) {
		TRY
			/* read all the content from .DIR and sort it in memory */
			memcpy(tdir, dir, st1.st_size);
			qsort(tdir, st1.st_size / sizeof(struct fileheader),
			      sizeof(struct fileheader), cmp_aid);
			mark_thread(tdir, st1.st_size / sizeof(struct fileheader));

			if ((fd = open(tname, O_WRONLY | O_CREAT | O_TRUNC, 0644)) > 0) {
				write(fd, tdir, st1.st_size);
				close(fd);
			}
		CATCH
			result = -1;
		END
		free(tdir);
	} else {
		result = -1;
	}

	munmap(dir, st1.st_size);
	return result;
}
