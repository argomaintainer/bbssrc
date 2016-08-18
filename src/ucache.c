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

int usernumber = 0;

int
fillucache(void *p, int uid)
{
	struct userec *uentp = p;

	if (usernumber < MAXUSERS) {
		strlcpy(uidshm->userid[usernumber], uentp->userid, sizeof(uidshm->userid[usernumber]));
		++usernumber;
	}
	return 0;
}

void
resolve_ucache(void)
{
	int fd;

	if (uidshm == NULL) {
		uidshm = attach_shm("UCACHE_SHMKEY", sizeof (*uidshm));
	}

/*
 *	if (stat(FLUSH, &st) < 0) {
 *		ftime = time(NULL) - 86400;
 *	} else {
 *		ftime = st.st_mtime;
 *	}
 */

	if (uidshm->updating != YEA && uidshm->uptime + 86400 < time(NULL)) {
		if ((fd = filelock("ucache.lock", NA)) > 0) {
			uidshm->updating = YEA;
			log_usies("CACHE", "reload ucache");
			usernumber = 0;
			apply_record(PASSFILE, fillucache, sizeof (struct userec));
			uidshm->number = usernumber;
			uidshm->uptime = time(NULL);
			close(fd);
			uidshm->updating = NA;
		}
	}
}

void
setuserid(int num, char *userid)
{
	if (num > 0 && num <= MAXUSERS) {
		if (num > uidshm->number)
			uidshm->number = num;
		strlcpy(uidshm->userid[num - 1], userid, sizeof(uidshm->userid[num - 1]));
	}
}

int
searchnewuser(void)
{
	int num, i;

	resolve_ucache();
	num = uidshm->number;
	for (i = 0; i < num; i++)
		if (uidshm->userid[i][0] == '\0')
			return i + 1;
	if (num < MAXUSERS)
		return (num + 1);
	return 0;
}


void
getuserid(char *userid, unsigned short uid)
{
	resolve_ucache();
	strcpy(userid, uidshm->userid[uid - 1]);
}

int
searchuser(char *userid)
{
	int i;
	resolve_ucache();
	for (i = 0; i < uidshm->number; i++)
		if (!strncasecmp(userid, uidshm->userid[i], IDLEN + 1))
			return i + 1;
	return 0;
}

/* 返回userid在PASSFILE的id(下标1开始).并填充lookuser 
 * 返回0, 表示没找到userid记录  */
int
getuser(char *userid, struct userec *lookupuser)
{
	int uid;

	if ((uid = searchuser(userid)) == 0)
		return 0;

	if (lookupuser == NULL)
		return uid;

	return (get_record(PASSFILE, lookupuser, sizeof(struct userec), uid) == -1 || lookupuser->userid[0] == '\0') ? 0 : uid;
}

char *
u_namearray(char (*buf)[IDLEN + 2], int *pnum, char *tag)
{
	struct UCACHE *reg_ushm = uidshm;
	char *ptr, tmp;
	int n, total;
	char tagbuf[STRLEN];
	int ch, num = 0;

	resolve_ucache();
	if (*tag == '\0') {
		*pnum = reg_ushm->number;
		return reg_ushm->userid[0];
	}
	for (n = 0; tag[n] != '\0'; n++) {
		tagbuf[n] = mytoupper(tag[n]);
	}
	tagbuf[n] = '\0';
	ch = tagbuf[0];
	total = reg_ushm->number;
	for (n = 0; n < total; n++) {
		ptr = reg_ushm->userid[n];
		tmp = *ptr;
		if (tmp == ch || tmp == ch - 'A' + 'a')
			if (chkstr(tag, tagbuf, ptr))
				strcpy(buf[num++], ptr);
	}
	*pnum = num;
	return buf[0];
}

void
resolve_utmp(void)
{
	if (utmpshm == NULL) {
		utmpshm = attach_shm("UTMP_SHMKEY", sizeof (*utmpshm));
	}
}

int
getnewutmpent(struct user_info *up)
{
	int utmpfd;
	struct user_info *uentp;
	time_t now;
	int i, n, num[2];

	resolve_utmp();
	if (utmpshm == NULL)
		return -1;

	if ((utmpfd = open(ULIST, O_RDWR | O_CREAT, 0600)) == -1)
		return -1;
	f_exlock(utmpfd); /* 锁住文件*/

	if (utmpshm->max_login_num < count_users)
		utmpshm->max_login_num = count_users;

	now = time(NULL);
	for (i = 0; i < USHM_SIZE; i++) { /* 查找空闲的slot*/
		uentp = &(utmpshm->uinfo[i]);
		if ((!uentp->active || !uentp->pid) && uentp->deactive_time + 60 < now)
			break;
	}

	/* failure if no entry is free */
	if (i >= USHM_SIZE) {
		close(utmpfd);		// f_unlock(utmpfd);
		return -2;
	}

	utmpshm->uinfo[i] = *up;
	if (now > utmpshm->uptime + 60) {
		num[0] = num[1] = 0;
		utmpshm->uptime = now;
		for (n = 0; n < USHM_SIZE; n++) {
			uentp = &(utmpshm->uinfo[n]);
			if (uentp->active) {
				/* 使用进程号1来表示无进程的状态 */
				if (uentp->pid != 1 && kill(uentp->pid, 0) == -1) { /* 进程不存在*/
					/* memset(uentp, 0, sizeof(struct user_info)); */
					uentp->active = NA;
					uentp->deactive_time = now;
					uentp->pid = 0;
				/* 无进程的状态设为15分钟超时 */					
				} else if (uentp->pid == 1 && now > uentp->idle_time + 15 * 60) {
					uentp->active = NA;
					uentp->deactive_time = now;
					uentp->pid = 0;
				} else {
					num[(uentp->invisible == YEA) ? 1 : 0]++;
				}
			}
		}
		utmpshm->usersum = allusers();
		n = USHM_SIZE - 1;
		while (n > 0 && utmpshm->uinfo[n].active == 0)
			n--;
		ftruncate(utmpfd, 0);
		write(utmpfd, utmpshm->uinfo, (n + 1) * sizeof(struct user_info)); /* 把uinfo写经ULIST */
	}
	close(utmpfd);		// f_unlock(utmpfd);
	return i + 1;
}

int
apply_ulist(int (*fptr)(struct user_info *u))
{
	int i;

	resolve_utmp();
	for (i = 0; i < USHM_SIZE - 1; i++) {
		if (utmpshm->uinfo[i].active == 0 || utmpshm->uinfo[i].pid == 0)
			continue;

		if ((*fptr)(&(utmpshm->uinfo[i])) == QUIT)
			return QUIT;
	}
	return 0;
}

int
apply_ulist_address(int (*fptr) (/* ??? */))
{
	int i, max;

	resolve_utmp();
	max = USHM_SIZE - 1;
	while (max > 0 && utmpshm->uinfo[max].active == 0)
		max--;
	for (i = 0; i <= max; i++) {
		if ((*fptr) (&(utmpshm->uinfo[i])) == QUIT)
			return QUIT;
	}
	return 0;
}

int
search_ulist(struct user_info *uentp, int (*fptr)(int, struct user_info *), int farg)
{
	int i;

	resolve_utmp();
	for (i = 0; i < USHM_SIZE; i++) {
		*uentp = utmpshm->uinfo[i];
		if ((*fptr) (farg, uentp))
			return i + 1;
	}
	return 0;
}

int
search_ulistn(struct user_info *uentp, int (*fptr)(int, struct user_info *), int farg, int unum)
{
	int i, j;

	j = 1;
	resolve_utmp();
	for (i = 0; i < USHM_SIZE; i++) {
		*uentp = utmpshm->uinfo[i];
		if ((*fptr) (farg, uentp)) {
			if (j == unum)
				return i + 1;
			else
				j++;
		}
	}
	return 0;
}

int
t_search_ulist(struct user_info *uentp, int (*fptr)(int, struct user_info *), int farg, int show, int doTalk)
{
	int i, num;
	char col[14];

	resolve_utmp();
	num = 0;
	for (i = 0; i < USHM_SIZE; i++) {
		*uentp = utmpshm->uinfo[i];
		if ((*fptr) (farg, uentp)) {
			if (!uentp->active || !uentp->pid || isreject(uentp))
				continue;
			if ((uentp->invisible == 0)
			    || (uentp->uid == usernum)
			    || ((uentp->invisible == 1)
			    && (HAS_PERM(PERM_SYSOP | PERM_SEECLOAK)))) {
				num++;
			} else {
				continue;
			}
			if (!show)
				continue;
			if (num == 1)
				prints("目前 %s 状态如下：\n", uentp->userid);
			if (uentp->invisible)
				strcpy(col, "[隐]\033[1;36m");
			else if (uentp->mode == POSTING)
				strcpy(col, "\033[1;32m");
			else if (uentp->mode == BBSNET ||
				 INBBSGAME(uentp->mode))
				strcpy(col, "\033[1;33m");
			else
				strcpy(col, "\033[1m");
			if (doTalk) {
				prints("(%d) 状态：%s%-10s\033[m，来自：%.20s\n",
				       num, col, modetype(uentp->mode),
				       (uentp->hideip != 'H' ||
					hisfriend(uentp)) ? uentp->
				       from : "子虚空间");
			} else {
				prints("%s%-10s\033[m ", col,
				       modetype(uentp->mode));
				if ((num) % 5 == 0)
					outc('\n');
			}
		}
	}
	if (show)
		outc('\n');
	return num;
}

void
update_ulist(struct user_info *uentp, int uent)
{
	resolve_utmp();
	if (uent > 0 && uent <= USHM_SIZE) {
		utmpshm->uinfo[uent - 1] = *uentp;
	}
}

void
update_utmp(void)
{
	update_ulist(&uinfo, utmpent);
}

/* added by djq 99.7.19*/
/* function added by douglas 990305
 set uentp to the user who is calling me
 solve the "one of 2 line call sb. to five" problem
*/

int
who_callme(struct user_info *uentp, int (*fptr)(int , struct user_info *), int farg, int me)
{
	int i;

	resolve_utmp();
	for (i = 0; i < USHM_SIZE; i++) {
		*uentp = utmpshm->uinfo[i];
		if ((*fptr) (farg, uentp) && uentp->destuid == me)
			return i + 1;
	}
	return 0;
}

