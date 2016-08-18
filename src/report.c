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

extern struct userec currentuser;

#ifdef MSGQUEUE

int msqid;
static struct bbsmsg msg = { mtype: BBSMSGTYPE };

void
do_report(int msgtype, char *str)
{
	msg.msgtype = msgtype;
	strlcpy(msg.message, str, sizeof(msg.message));
	msgsnd(msqid, &msg, sizeof(msg), 0);
}


/* freestyler: 添加一个支持变长参数版本 */
void
do_report2(int msgtype, char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(msg.message, sizeof(msg.message), fmt, ap);
	va_end(ap);

	msg.msgtype = msgtype;
	msgsnd(msqid, &msg, sizeof(msg), 0);
}


void
report(char *fmt, ...)
{
	va_list ap;
	char buf[210];
	time_t now;

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	now = time(NULL);
	msg.msgtype = LOG_TRACE;
	sprintf(msg.message, "%s %24.24s %s\n", currentuser.userid, ctime(&now), buf);
	msgsnd(msqid, &msg, sizeof(msg), 0);
}

#else

void
do_report(char *filename, char *str)
{
	char buf[512];
	time_t now;

	now = time(NULL);
	sprintf(buf, "%s %24.24s %s\n", currentuser.userid, ctime(&now), str);
	file_append(filename, buf);
}

/* freestyler: 添加一个支持变长参数版本 */
void
do_report2(char* filename, char *fmt, ...)
{
	va_list ap;
	char 	buf[1024];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

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

#endif

void
b_report(char *str)
{
	char buf[STRLEN];

	sprintf(buf, "%s %s", currboard, str);
	report(buf);
}

void
autoreport(char *title, char *str, int toboard, char *userid, char *attachfile)
{
	FILE *se, *fp;
	char fname[PATH_MAX + 1], buf[1024];
	int savemode, savedigestmode;

	savemode = uinfo.mode;
	report(title);
	snprintf(fname, sizeof(fname), "tmp/%s.%s.%05d", BBSID, currentuser.userid, uinfo.pid);
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "%s", str);
		if (attachfile != NULL && (fp = fopen(attachfile, "r")) != NULL) {
                        while (fgets(buf, sizeof(buf), fp) != NULL)
                                fputs(buf, se);	
			fclose(fp);
		}
		add_syssign(se);
		fclose(se);

		if (userid != NULL) {
			mail_sysfile(fname, userid, title);
		}

		if (toboard) {
			savedigestmode = digestmode;
			digestmode = NA;
			local_article = NA;
			postfile(fname, currboard, title, 1);
			digestmode = savedigestmode;
		}
		unlink(fname);
		modify_user_mode(savemode);
	}
}

void
do_securityreport(char *str, struct userec *userinfo, int fullinfo, char *addinfo)
{
	FILE *se;
	char fname[STRLEN];
	int savemode, savedigestmode;

	savemode = uinfo.mode;
	report(str);
	sprintf(fname, "tmp/security.%s.%05d", currentuser.userid, uinfo.pid);
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "系统安全记录\n\033[1m原因：%s\033[m\n", str);
		if (addinfo)
			fprintf(se, "%s\n", addinfo);
		if (fullinfo) {
			fprintf(se, "\n以下是个人资料：");
			/* Rewrite by cancel at 01/09/16 */
			/* 修改了getuinfo()，加上了第二个参数 */
			getuinfo(se, userinfo);
		} else {
			getdatestring(userinfo->lastlogin);
			fprintf(se, "\n以下是部分个人资料：\n");
			fprintf(se, "最近光临日期 : %s\n", datestring);
			fprintf(se, "最近光临机器 : %s\n", userinfo->lasthost);
		}
		fclose(se);
		savedigestmode = digestmode;
		digestmode = NA;
		postfile(fname, "syssecurity", str, 2);
		digestmode = savedigestmode;
		unlink(fname);
		modify_user_mode(savemode);
	}
}

void
securityreport(char *str)
{
	do_securityreport(str, &currentuser, NA, NULL);
}

void
securityreport2(char *str, int fullinfo, char *addinfo)
{
	do_securityreport(str, &currentuser, fullinfo, addinfo);
}

/* Add by cancel at 01/09/13 : 详细report修改使用者权限 */
void
sec_report_level(char *str, unsigned int oldlevel, unsigned int newlevel)
{
	FILE *se;
	char fname[STRLEN], buf[STRLEN];
	int savemode, i;

	savemode = uinfo.mode;
	report(str);
	sprintf(fname, "tmp/security.%s.%05d", currentuser.userid, uinfo.pid);
	if ((se = fopen(fname, "w")) != NULL) {
		fprintf(se, "系统安全记录\n\033[1m原因：修改 %s 的权限\033[m\n\n", str);
		fprintf(se, "修改后 %s 的权限如下，红色部分表示修改过\n", str);
		oldlevel ^= newlevel;
		for (i = 0; i < 16; i++) {
			fprintf(se, "%s%c. %-30s %2s\033[m    ",
				((oldlevel >> i) & 1 ? "\033[1;31m" : ""), 'A' + i,
				permstrings[i],
				((newlevel >> i) & 1 ? "是" : "×"));
			i += 16;
			if (i >= NUMPERMS) {
				i -= 16;
				fprintf(se, "\n");
				continue;
			}
			fprintf(se, "%s%c. %-30s %2s\033[m\n",
				((oldlevel >> i) & 1 ? "\033[1;31m" : ""), 'A' + i,
				permstrings[i],
				((newlevel >> i) & 1 ? "是" : "×"));
			i -= 16;
		}
		fprintf(se, "\n\n");
		fclose(se);
		sprintf(buf, "修改 %s 的权限", str);
		postfile(fname, "syssecurity", buf, 2);
		unlink(fname);
		modify_user_mode(savemode);
	}
}

void
log_usies(char *mode, char *msg)
{
	char buf[256], *fmt;
	time_t now;

	now = time(NULL);
	fmt = currentuser.userid[0] ? "%24.24s %s %-12s %s\n" : "%24.24s %s %s%s\n";
	sprintf(buf, fmt, ctime(&now), mode, currentuser.userid, msg);

#ifdef MSGQUEUE
	do_report(LOG_USIES, buf);
#else
	file_append("reclog/usies", buf);
#endif
}
