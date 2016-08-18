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

struct userec lookupuser;

void
add_sysheader(char *board, FILE *fp, char *title)
{
	time_t now;
	char buf[80];

	now = time(0);
	if (board != NULL) {
		sprintf(buf, "\033[1;41;33m发信人: %s (自动发信系统), 信区: %s",
			BBSID, board);
	} else {
		sprintf(buf, "\033[1;41;33m寄信人: %s (自动发信系统)", BBSID);
	}

	fprintf(fp, "%s%*s\033[m\n", buf, (int)(89 - strlen(buf)), " ");
	fprintf(fp, "标  题: %s\n", title);
	fprintf(fp, "发信站: %s (%24.24s)\n\n", BBSID, ctime(&now));

	if (board == NULL) {
		fprintf(fp, "来  源: %s\n\n", BBSHOST);
	}
}

void
add_syssign(FILE *fp)
{
	fprintf(fp, "\n--\n\033[m\033[1;31m※ 来源:．%s %s．[FROM: %s]\033[m\n",
		BBSNAME, BBSHOST, BBSHOST);
}

void
do_report(char *filename, char *s)
{
	char buf[512];
	time_t now;

	now = time(0);
	sprintf(buf, "%s %24.24s %s\n", "deliver", ctime(&now), s);
	file_append(filename, buf);
}

void
report(char *fmt, ...)
{
	va_list ap;
	char str[1024];

	va_start(ap, fmt);
	vsprintf(str, fmt, ap);
	va_end(ap);

	do_report("reclog/trace", str);
}

void
autoreport(char *title, char *str, char *board, int toboard, char *userid)
{
	FILE *se;
	char fname[STRLEN];

	report(title);

	if (userid != NULL && strcmp(userid, "guest")) {
		sprintf(fname, "%s/boards/.tmp/undeny.%05d", BBSHOME, getpid());
		if ((se = fopen(fname, "w")) != NULL) {
			add_sysheader(NULL, se, title);
			fprintf(se, "%s", str);
			add_syssign(se);
			fclose(se);
			postmail(fname, userid, title, "deliver", 0, NA);
			unlink(fname);
		}
	}

	if (toboard) {
		sprintf(fname, "%s/boards/.tmp/undeny.%05d", BBSHOME, getpid());
		if ((se = fopen(fname, "w")) != NULL) {
			add_sysheader(board, se, title);
			fprintf(se, "%s", str);
				add_syssign(se);
			fclose(se);
			postfile(fname, board, title, "deliver", FILE_MARKED | FILE_NOREPLY);
			unlink(fname);
		}
	}
}

void
securityreport(char *str)
{
	FILE *se;
	char fname[STRLEN];

	report(str);
	sprintf(fname, "%s/boards/.tmp/undeny.s.%05d", BBSHOME, getpid());
	if ((se = fopen(fname, "w")) != NULL) {
		add_sysheader("syssecurity", se, str);
		fprintf(se, "系统安全记录\n\033[1m原因：%s\033[m\n", str);
		fclose(se);
		postfile(fname, "syssecurity", str, "deliver", 0);
		unlink(fname);
	}
}

int
delfromdeny(char *board, char *uident, int flag)
{
	char repbuf[STRLEN], msgbuf[1024];
	int uid;

	if ((uident[0] == '\0') || ((uid = getuser(uident, &lookupuser)) == 0))
		return (-1);

	if (flag & D_FULLSITE) {
		lookupuser.userlevel |= PERM_POST;
		substitute_record(PASSFILE, &lookupuser, sizeof (struct userec), uid);

		sprintf(repbuf, "恢复 %s 的全站发文权利", uident);
		securityreport(repbuf);
		sprintf(msgbuf, "\n  %s 网友：\n\n"
				 "    因封禁时间已过，现恢复您在本站的发文权利。\n\n",
			uident);
		autoreport(repbuf, msgbuf, NULL, NA, uident);
	} else {
		sprintf(repbuf, "恢复 %s 在 %s 版的发文权利", uident, board);
		securityreport(repbuf);
		sprintf(msgbuf,
			"\n  \033[1;32m%s\033[m 网友：\n\n"
			"    因封禁时间已过，现恢复您在 \033[1;4;36m%s\033[m 版的发文权利。\n\n",
			uident, board);
		autoreport(repbuf, msgbuf, board, (flag & D_ANONYMOUS) ? NA : YEA,
			   uident);
	}
	return 0;
}

void
undeny(char *board, int anonymous)
{
	int fd, tfd, flag = 0;
	char buf[STRLEN];
	struct denyheader hdr;

	if (board == NULL) {
		flag = D_FULLSITE;
		sprintf(buf, "%s/boards", BBSHOME);
	} else {
		sprintf(buf, "%s/boards/%s", BBSHOME, board);
	}

	if (anonymous != 0)
		flag |= D_ANONYMOUS;

	if (chdir(buf) == -1)
		return;

	if (board == NULL) {
		if (chdir(buf) == -1)
			return;
	}

	if ((fd = open(".DENYLIST", O_RDONLY)) == -1)
		return;
	if ((tfd = creat(".UNDENY.tmp", 0644)) == -1)
		return;

	while (read(fd, &hdr, sizeof(hdr)) == sizeof(hdr)) {
		if (time(NULL) > hdr.undeny_time) {
			delfromdeny(board, hdr.blacklist, flag);
		} else {
			write(tfd, &hdr, sizeof(hdr));
		}
	}
	close(fd);
	close(tfd);
	rename(".UNDENY.tmp", ".DENYLIST");
}

int
main()
{
	int fd;
	struct boardheader hdr;

	undeny(NULL, NA);
	chdir(BBSHOME);
	if ((fd = open(".BOARDS", O_RDONLY)) == -1)
		return -1;

	while (read(fd, &hdr, sizeof(hdr)) == sizeof(hdr)) {
		if (hdr.filename[0])
			undeny(hdr.filename, (hdr.flag & ANONY_FLAG) ? YEA : NA);
	}

	close(fd);
	return 0;
}
