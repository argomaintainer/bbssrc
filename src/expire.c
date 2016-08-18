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

/*-------------------------------------------------------*/
/* util/expire.c        ( NTHU CS MapleBBS Ver 2.36 )    */
/*-------------------------------------------------------*/
/* target : 自动砍信工具程式                             */
/* create : 95/03/29                                     */
/* update : 95/12/15                                     */
/*-------------------------------------------------------*/
/* syntax : expire [day] [max] [min]                     */
/*-------------------------------------------------------*/
/* 移植到 FireBird      by Nighthawk                     */
/*-------------------------------------------------------*/

#include "bbs.h"

#define DEF_DAYS        81
#define DEF_MAXP        2000
#define DEF_MINP        200

#define EXPIRE_CONF     BBSHOME"/etc/expire.ctl"

char *bpath = BBSHOME "/boards";

struct life {
	char bname[BFNAMELEN];	/* board ID */
	int days;		/* expired days */
	int maxp;		/* max post */
	int minp;		/* min post */
};
typedef struct life life;

int safewrite(int fd, void *buf, int size);

void
expire(life *brd)
{
	struct fileheader head;
	struct stat state;
	char lockfile[128], tmpfile[128], bakfile[128];
	char fpath[128], index[128], *fname;
	int total;
	int fd, fdr, fdw, done, keep;
	int duetime, ftime;

	printf("%s\n", brd->bname);

#ifdef  VERBOSE
	if (brd->days < 1) {
		printf(":Err: expire time must more than 1 day.\n");
		return;
	} else if (brd->maxp < 100) {
		printf(":Err: maxmum posts number must more than 100.\n");
		return;
	}
#endif

	snprintf(index, sizeof(index), "%s/%s/.DIR", bpath, brd->bname);
	snprintf(lockfile, sizeof(lockfile), "%s.lock", index);
	if ((fd = open(lockfile, O_RDWR | O_CREAT | O_APPEND, 0644)) == -1)
		return;
	f_exlock(fd);

	strcpy(fpath, index);
	fname = (char *) strrchr(fpath, '.');

	duetime = time(NULL) - brd->days * 24 * 60 * 60;
	done = 0;
	if ((fdr = open(index, O_RDONLY, 0)) > 0) {
		fstat(fdr, &state);
		total = state.st_size / sizeof (head);
		snprintf(tmpfile, sizeof(tmpfile), "%s.new", index);
		unlink(tmpfile);
		if ((fdw =
		     open(tmpfile, O_WRONLY | O_CREAT | O_EXCL, 0644)) > 0) {
			while (read(fdr, &head, sizeof head) == sizeof head) {
				done = 1;
				ftime = head.filetime;
				if (head.owner[0] == '-')
					keep = 0;
				else if (head.flag & FILE_MARKED ||
					 total <= brd->minp)
					keep = 1;
				else if (ftime < duetime || total > brd->maxp)
					keep = 0;
				else
					keep = 1;

				if (keep) {
					if (safewrite
					    (fdw, (char *) &head,
					     sizeof head) == -1) {
						done = 0;
						break;
					}
				} else {
					strcpy(fname, head.filename);
					unlink(fpath);
					printf("\t%s\n", fname);
					total--;
				}
			}
			close(fdw);
		}
		close(fdr);
	}
	if (done) {
		snprintf(bakfile, sizeof(bakfile), "%s.old", index);
		if (rename(index, bakfile) != -1)
			rename(tmpfile, index);
	}
	f_unlock(fd);
	close(fd);
}

int
main(int argc, char **argv)
{
	FILE *fin;
	int number;
	size_t count;
	life db, table[MAXBOARD], *key;
	struct dirent *de;
	DIR *dirp;
	char *ptr, *bname, buf[256];

	db.days = ((argc > 1) &&
		   (number = atoi(argv[1])) > 0) ? number : DEF_DAYS;
	db.maxp = ((argc > 2) &&
		   (number = atoi(argv[2])) > 0) ? number : DEF_MAXP;
	db.minp = ((argc > 3) &&
		   (number = atoi(argv[3])) > 0) ? number : DEF_MINP;

	/* --------------- */
	/* load expire.ctl */
	/* --------------- */

	count = 0;
	if ((fin = fopen(EXPIRE_CONF, "r")) != NULL) {
		while (fgets(buf, 256, fin)) {
			if (buf[0] == '#')
				continue;

			bname = (char *) strtok(buf, " \t\r\n");
			if (bname && *bname) {
				ptr = (char *) strtok(NULL, " \t\r\n");
				if (ptr && (number = atoi(ptr)) > 0) {
					key = &(table[count++]);
					strcpy(key->bname, bname);
					key->days = number;
					key->maxp = db.maxp;
					key->minp = db.minp;

					ptr = (char *) strtok(NULL, " \t\r\n");
					if (ptr && (number = atoi(ptr)) > 0) {
						key->maxp = number;

						ptr =
						    (char *) strtok(NULL,
								    " \t\r\n");
						if (ptr &&
						    (number = atoi(ptr)) > 0) {
							key->minp = number;
						}
					}
				}
			}
		}
		fclose(fin);
	}
	if (count > 1) {
		qsort(table, count, sizeof (life), (const void *) strcasecmp);
	}
	/* ---------------- */
	/* visit all boards */
	/* ---------------- */

	if (!(dirp = opendir(bpath))) {
		printf(":Err: unable to open %s\n", bpath);
		return -1;
	}
	while ((de = readdir(dirp))) {
		ptr = de->d_name;
		if (ptr[0] > ' ' && ptr[0] != '.') {
			if (count)
				key =
				    (life *) bsearch(ptr, table, count,
						     sizeof (life),
						     (const void *) strcasecmp);
			else
				key = NULL;
			if (!key)
				key = &db;
			strcpy(key->bname, ptr);
			expire(key);
		}
	}
	closedir(dirp);
	return 0;
}

/* 返回filename中第id个记录到rptr, id是从下标1开始计算 */
int
get_record(char *filename, void *rptr, int size, int id)
{
	int fd;

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;
	if (lseek(fd, (off_t) (size * (id - 1)), SEEK_SET) == -1) {
		close(fd);
		return -1;
	}
	if (read(fd, rptr, size) != size) {
		close(fd);
		return -1;
	}
	if (read(fd, rptr, size) != size) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

int
safewrite(int fd, void *buf, int size)
{
	int cc, sz = size, origsz = size;
	char *bp = buf;

	do {
		cc = write(fd, bp, sz);
		if ((cc < 0) && (errno != EINTR)) {
			return -1;
		}
		if (cc > 0) {
			bp += cc;
			sz -= cc;
		}
	}
	while (sz > 0);
	return origsz;
}

int
append_record(char *filename, void *record, int size)
{
	int fd;

	if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1) {
		return -1;
	}
	f_exlock(fd);
	lseek(fd, 0, SEEK_END);
	safewrite(fd, record, size);
	f_unlock(fd);
	close(fd);
	return 0;
}

