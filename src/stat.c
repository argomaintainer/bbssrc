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

extern char save_page_requestor[40];

int
count_useshell(struct user_info *uentp)
{
	static int count;

	if (uentp == NULL) {
		int c = count;

		count = 0;
		return c;
	}

	if (uentp->mode == SYSINFO
	    || uentp->mode == DICT
	    || uentp->mode == BBSNET || INBBSGAME(uentp->mode)
	    || uentp->mode == LOGIN)
		count++;
	return 1;
}

int
num_useshell(void)
{
	count_useshell(NULL);
	apply_ulist(count_useshell);
	return count_useshell(NULL);
}

int
count_active(struct user_info *uentp)
{
	static int count;

	if (uentp == NULL) {
		int c = count;

		count = 0;
		return c;
	}

	count++;
	return 1;
}

int
num_active_users(void)
{
	count_active(NULL);
	apply_ulist(count_active);
	return count_active(NULL);
}

int
count_user_logins(struct user_info *uentp)
{
	static int count;

	if (uentp == NULL) {
		int c = count;

		count = 0;
		return c;
	}
	if (!strcmp(uentp->userid, save_page_requestor))
		count++;
	return 1;
}

int
num_user_logins(char *uid)
{
	strcpy(save_page_requestor, uid);
	count_active(NULL);
	apply_ulist(count_user_logins);
	return count_user_logins(NULL);
}

int
count_visible_active(struct user_info *uentp)
{
	static int count;

	if (uentp == NULL) {
		int c = count;

		count = 0;
		return c;
	}
	if (isreject(uentp))
		return 0;
	if (!uentp->invisible || uentp->uid == usernum || HAS_PERM(PERM_SYSOP | PERM_SEECLOAK))
		count++;
	return 1;
}

int
num_visible_users(void)
{
	count_visible_active(NULL);
	apply_ulist(count_visible_active);
	return count_visible_active(NULL);
}

int
alcounter(struct user_info *uentp)
{
	static int vi_users, vi_friends;

	if (uentp == NULL) {
		count_friends = vi_friends;
		count_users = vi_users;
		vi_users = vi_friends = 0;
		return 1;
	}
	if (isreject(uentp))
		return 0;

	if (!uentp->invisible || uentp->uid == usernum || HAS_PERM(PERM_SYSOP | PERM_SEECLOAK)) {
		vi_users++;
		if (myfriend(uentp->uid))
			vi_friends++;
	}
	return 1;
}

/* count all online users and friends */
int
num_alcounter(void)
{
	static time_t last_time = 0;

	if (abs(time(NULL) - last_time) < 20)
		return 0;

	last_time = time(NULL);
	alcounter(NULL);
	apply_ulist(alcounter);
	alcounter(NULL);

	return 0;
}

int
count_multi(struct user_info *uentp)
{
	static int count;

	if (uentp == NULL) {
		int num = count;

		count = 0;
		return num;
	}
	if (uentp->uid == usernum)
		count++;
	return 1;
}

int
count_self(void)
{
	count_multi(NULL);
	apply_ulist(count_multi);
	return count_multi(NULL);
}

int
count_ip(char *fromhost)
{
	int i, j = 0, k = 0;
	char buf[STRLEN], *pstr, *temp = NULL;
	struct user_info uentp;
	FILE *fp;

	resolve_utmp();
	for (i = 0; i < USHM_SIZE; i++) {
		uentp = utmpshm->uinfo[i];
		if (!uentp.active || !uentp.pid)
			continue;
		if (!strcmp(uentp.from, fromhost))
			j++;
	}

	/* etc/ipcount文件包含特殊ip站点的连接数　*/
	if ((fp = fopen("etc/ipcount", "r")) == NULL) 
		return j;

	while (fgets(buf, STRLEN, fp) != NULL) {
		if (buf[0] == '#')
			continue;
		if (strstr(buf, fromhost) != NULL) {
			if ((pstr = strtok(buf, " \t")) != NULL)
				temp = strtok((char *) NULL, " \t");
			if (temp != NULL) {
				k = atoi(temp);
				if (k > 0 && k <= 1000)
					return (j + MAXPERIP - k);
				else
					return j;
			} else
				break;
		}
	}
	fclose(fp);
	return j;
}
