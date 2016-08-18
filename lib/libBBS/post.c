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

#define __NO_PROTOTYPE__

#include "bbs.h"
#include "libBBS.h"

struct UCACHE *uidshm = NULL;
struct BCACHE *brdshm = NULL;
struct boardheader *bcache = NULL;
int numboards = -1;

void
attach_err(int shmkey, char *name, int err)
{
	fprintf(stderr, "Error! %s error #%d! key = %x.\n", name, err, shmkey);
	exit(1);
}

int
search_shmkey(char *keyname)
{
	int i = 0, found = 0;

	while (shmkeys[i].key != NULL) {
		if (strcmp(shmkeys[i].key, keyname) == 0) {
			found = shmkeys[i].value;
			break;
		}
		i++;
	}

	return found;
}

void *
attach_shm(char *shmstr, int shmsize)
{
	void *shmptr;
	int shmkey, shmid;

	shmkey = search_shmkey(shmstr);
	shmid = shmget(shmkey, shmsize, 0);
	if (shmid < 0) {
		shmid = shmget(shmkey, shmsize, IPC_CREAT | 0644);
		if (shmid < 0)
			attach_err(shmkey, "shmget", errno);
		shmptr = (void *) shmat(shmid, NULL, 0);
		if (shmptr == (void *) -1)
			attach_err(shmkey, "shmat", errno);
		memset(shmptr, 0, shmsize);
	} else {
		shmptr = (void *) shmat(shmid, NULL, 0);
		if (shmptr == (void *) -1)
			attach_err(shmkey, "shmat", errno);
	}
	return shmptr;
}

void
resolve_boards(void)
{
	if (brdshm == NULL) {
		brdshm = attach_shm("BCACHE_SHMKEY", sizeof (*brdshm));
	}
	numboards = brdshm->number;
	bcache = brdshm->bcache;
}

struct boardheader *
getbcache(char *bname)
{
	int i;

	resolve_boards();
	for (i = 0; i < numboards; i++) {
		if (!strncmp(bname, bcache[i].filename, STRLEN)) {
			return &bcache[i];
		}
	}
	return NULL;
}

int
get_lastpost(char *board, unsigned int *lastpost, unsigned int *total)
{
	struct fileheader fh;
	struct stat st;
	char filename[STRLEN];
	int fd, atotal;

	sprintf(filename, "%s/boards/%s/.DIR", BBSHOME, board);

	if (stat(filename, &st) == -1 || st.st_size == 0 ||
	    (fd = open(filename, O_RDONLY)) < 0)
		return 0;

	atotal = st.st_size / sizeof (fh);
	*total = atotal;
	lseek(fd, (off_t) (atotal - 1) * sizeof (fh), SEEK_SET);
	if (read(fd, &fh, sizeof (fh)) > 0)
		*lastpost = fh.filetime;
	close(fd);
	return 0;
}

int
update_lastpost(char *board)
{
	struct boardheader *bptr;

	if ((bptr = getbcache(board)) == NULL)
		return -1;

	return get_lastpost(bptr->filename, &bptr->lastpost, &bptr->total);
}

void
resolve_ucache(void)
{
	if (uidshm == NULL) {
		uidshm = attach_shm("UCACHE_SHMKEY", sizeof (*uidshm));
	}
}

int
getuser(char *userid, struct userec *lookupuser)
{
	int i;

	resolve_ucache();
	for (i = 0; i < uidshm->number; i++) {
		if (!strncmp(userid, uidshm->userid[i], IDLEN + 1)) {
			get_record(PASSFILE, lookupuser, sizeof(struct userec), i + 1);
			return (lookupuser->userid[0] != 0) ? (i + 1) : 0;
		}
	}

	return 0;
}

int
cmpfnames(void *userid_ptr, void *uv_ptr)
{
	char *userid = (char *)userid_ptr;
	struct override *uv = (struct override *)uv_ptr;

	return !strcmp(userid, uv->id);
}

int
postfile_internal(char *filename, char *basedir, char *title, char *owner, int flag)
{
	int fd, now;
	char fname[80], fullname[250];
	char *content;
	struct stat stat_buffer;
	struct fileheader header;

	memset(&header, 0, sizeof (header));
	strcpy(header.title, title);
	strcpy(header.owner, owner);
	header.flag = flag;

	if ((fd = open(filename, O_RDONLY)) == -1)
		return (-1);

	if (fstat(fd, &stat_buffer) != 0) {
		close(fd);
		return (-1);
	}

	content = mmap(0, stat_buffer.st_size, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);
	if (content == NULL)
		return (-2);

	now = time(0);
	do {
		sprintf(fname, "M.%d.A", now);
		sprintf(fullname, "%s/%s", basedir, fname);
		now++;
	} while (dashf(fullname));

	if ((fd = open(fullname, O_CREAT | O_EXCL | O_WRONLY, 0644)) == -1)
		return -3;

	write(fd, content, stat_buffer.st_size);
	munmap(content, stat_buffer.st_size);
	close(fd);

	strlcpy(header.filename, fname, STRLEN);
	header.id = now;

	sprintf(fullname, "%s/.DIR", basedir);
	header.filetime = time(NULL);
	return append_record(fullname, &header, sizeof (struct fileheader));
}

int
getmailboxsize(unsigned int userlevel)
{
	if (userlevel & (PERM_SYSOP))
		return 5000;
	if (userlevel & (PERM_LARGEMAIL))
		return 3600;
	if (userlevel & (PERM_BOARDS))
		return 2400;
	if (userlevel & (PERM_LOGINOK))
		return 1200;
	return 5;
}

int
getmailsize(char *userid, char *basedir)
{
	struct fileheader fcache;
	struct stat st;
	char dirfile[STRLEN], mailfile[STRLEN];
	int fd, msize, total = 0, i, count;

	setmailpath(dirfile, userid);
	if ((fd = open(dirfile, O_RDWR)) != -1) {
		f_exlock(fd);
		fstat(fd, &st);
		count = st.st_size / sizeof (fcache);

		for (i = 0; i < count; i++) {
			if (lseek(fd, (off_t) (sizeof (fcache) * i), SEEK_SET) == -1)
				break;

			if (read(fd, &fcache, sizeof (fcache)) != sizeof (fcache))
				break;

			msize = fcache.size;

			if (msize <= 0) {
				sprintf("%s/%s", basedir, fcache.filename);
				if (stat(mailfile, &st) != -1) {
					msize = st.st_size;
					if (msize == 0) msize = 1;
				} else {
					msize = 1;
				}

				fcache.size = msize;
				if (lseek(fd, (off_t) (sizeof (fcache) * i), SEEK_SET) == -1)
					break;

				if (safewrite(fd, &fcache, sizeof (fcache)) != sizeof (fcache))
					break;
			}
			total += msize;
		}
		f_unlock(fd);
		close(fd);
	}

	return total / 1024 + 1;
}

extern char* currboard;
extern void update_total_tody(char*);

int
postfile(char *filename, char *board, char *title, char *owner, int flag)
{
	int ret;
	char basedir[STRLEN];

	sprintf(basedir, "%s/boards/%s", BBSHOME, board);
	ret = postfile_internal(filename, basedir, title, owner, flag);
	update_lastpost(board);
	update_total_today(currboard);	
	return ret;
}

int
postmail(char *filename, char *user, char *title, char *owner, int flag, int check_permission)
{
	char basedir[STRLEN], filepath[STRLEN];
	struct userec lookupuser;
	struct override fh;

	sprintf(basedir, "%s/mail/%c/%s", BBSHOME, mytoupper(user[0]), user);

	/* check premissions */
	if (check_permission) {
		if (getuser(user, &lookupuser) == 0)
			return 1;

		if (!(lookupuser.userlevel & PERM_READMAIL))
			return 3;
		if (lookupuser.userlevel & PERM_SUICIDE)
			return 6;

		if (strcmp(user, owner)) {
			sethomefile(filepath, user, "maildeny");
			if (search_record(filepath, &fh, sizeof(fh), cmpfnames, owner))
				return 5;
		}

		if (getmailboxsize(lookupuser.userlevel) * 2 < getmailsize(user, basedir))
			return 4;
	}

	return postfile_internal(filename, basedir, title, owner, flag);
}
