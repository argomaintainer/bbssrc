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

struct BCACHE *brdshm;
struct UCACHE *uidshm;
struct UTMPFILE *utmpshm;
struct FILESHM *welcomeshm;
struct FILESHM *goodbyeshm;
struct FILESHM *issueshm;
struct STATSHM *statshm;
struct ACSHM *movieshm;
struct ELSHM *endline_shm;

void
attach_err(int shmkey, char *name, int err)
{
	fprintf(stderr, "Error! %s error #%d! key = %x.\n", name, err, shmkey);
	exit(1);
}

int
search_shmkey(char *keyname)
{
	int i = 0;

	while (shmkeys[i].key != NULL) {
		if (strcmp(shmkeys[i].key, keyname) == 0)
			return shmkeys[i].value;
		i++;
	}

	return 0;
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

int
fill_shmfile(int mode, char *fname, char *shmkey)
{
	FILE *fffd;
	char *ptr;
	char buf[FILE_BUFSIZE];
	struct stat st;
	time_t ftime, now;
	int lines = 0, nowfn = 0, maxnum = 0;
	struct FILESHM *tmp;

	switch (mode) {
	case 1:
		maxnum = MAX_ISSUE;
		break;
	case 2:
		maxnum = MAX_GOODBYE;
		break;
	case 3:
		maxnum = MAX_WELCOME;
		break;
	}
	now = time(NULL);
	if (stat(fname, &st) < 0) {
		return 0;
	}
	ftime = st.st_mtime;
	tmp = attach_shm(shmkey, sizeof(struct FILESHM) * maxnum);
	switch (mode) {
	case 1:
		issueshm = tmp;
		break;
	case 2:
		goodbyeshm = tmp;
		break;
	case 3:
		welcomeshm = tmp;
		break;
	}

	if (abs(now - tmp[0].update) < 86400 && ftime < tmp[0].update) {
		return 1;
	}
	if ((fffd = fopen(fname, "r")) == NULL) {
		return 0;
	}
	while ((fgets(buf, FILE_BUFSIZE, fffd) != NULL) && nowfn < maxnum) {
		if (lines > FILE_MAXLINE)
			continue;
		if (!strncmp(buf, "@logout@", 8) || !strncmp(buf, "@login@", 7) || !strncmp(buf, "@issue@", 7)) {
			tmp[nowfn].fileline = lines;
			tmp[nowfn].update = now;
			nowfn++;
			lines = 0;
			continue;
		}
		ptr = tmp[nowfn].line[lines];
		memcpy(ptr, buf, sizeof (buf));
		lines++;
	}
	fclose(fffd);
	tmp[nowfn].fileline = lines;
	tmp[nowfn].update = now;
	nowfn++;
	tmp[0].max = nowfn;
	return 1;
}

int
fill_statshmfile(char *fname, int mode)
{
	FILE *fp;
	time_t ftime;
	char *ptr;
	char buf[FILE_BUFSIZE];
	struct stat st;
	time_t now;
	int lines = 0;

	if (stat(fname, &st) < 0) {
		return 0;
	}
	ftime = st.st_mtime;
	now = time(NULL);

	if (mode == 0 || statshm == NULL) {
		statshm = attach_shm("STAT_SHMKEY", sizeof (struct STATSHM) * 2);
	}
	if (abs(now - statshm[mode].update) < 86400 &&
	    ftime < statshm[mode].update) {
		return 1;
	}
	if ((fp = fopen(fname, "r")) == NULL) {
		return 0;
	}
	memset(&statshm[mode], 0, sizeof (struct STATSHM));
	while ((fgets(buf, FILE_BUFSIZE, fp) != NULL) && lines < FILE_MAXLINE) {
		ptr = statshm[mode].line[lines];
		memcpy(ptr, buf, sizeof (buf));
		lines++;
	}
	fclose(fp);
	statshm[mode].update = now;
	return 1;
}

void
show_shmfile(struct FILESHM *fh)
{
	int i;
	char buf[FILE_BUFSIZE];

	for (i = 0; i < fh->fileline; i++) {
		strcpy(buf, fh->line[i]);
		showstuff(buf /*, 0 */ );
	}
}

int
show_statshm(char *fh, int mode)
{
	int i;
	char buf[FILE_BUFSIZE];

	if (fill_statshmfile(fh, mode)) {
		if (mode == 0) {
			clear();

			for (i = 0; i <= 24; i++) {
				if (statshm[mode].line[i] == NULL)
					break;
				strlcpy(buf, statshm[mode].line[i], FILE_BUFSIZE);
				outs(buf);
			}
		}
		if (mode == 1)
			shmdt(statshm);
		return 1;
	}
	return 0;
}

void
show_goodbyeshm(void)
{
	int logouts;

	logouts = goodbyeshm[0].max;
	clear();
	show_shmfile(&goodbyeshm
		     [(currentuser.numlogins %
		       ((logouts <= 1) ? 1 : logouts))]);
	shmdt(goodbyeshm);
}

/* Rewrited cancel */
void
show_welcomeshm(void)
{
	int i;

	for (i = 0; i < welcomeshm[0].max; i++) {
		clear();
		show_shmfile(&welcomeshm[i]);
		pressanykey();
	}
	shmdt(welcomeshm);
}

/* Rewrited End. */

void
show_issue(void)
{
	int issues = issueshm[0].max;

	show_shmfile(&issueshm[(issues <= 1) ? 0 : ((time(NULL) / 180) % (issues))]);
	shmdt(issueshm);
}

void
deattach_shm(void)
{
	shmdt(brdshm);
	shmdt(uidshm);
	shmdt(statshm);
	shmdt(movieshm);
	shmdt(welcomeshm);
	shmdt(goodbyeshm);
	shmdt(endline_shm);
}

