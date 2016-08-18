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
#include "libBBS.h"

void regmaillog(char *user)
{
	FILE *fp;
	time_t now;

	if ((fp = fopen("reclog/regmail.log", "a+")) != NULL) {
		now = time(NULL);
		fprintf(fp, "%24.24s %s registered successfully\n", ctime(&now), user);
		fclose(fp);
	}
}

/* 检查注册信回信标题是否合法 */
int verify_title(char *title, char *ident, char *code)
{
	char *ptr, *ptr2;

	/*
	 *	Sample Title:
	 *
	 *	Re: @Ident@[-Regtime:Code-]deliver mail check
	 */

	if ((ptr = strchr(title, '@')) == NULL)
		return -1;
	if ((ptr2 = strchr(ptr + 1, '@')) == NULL)
		return -1;
	if (ptr2 - ptr - 1 > IDLEN)
		return -1;
	strlcpy(ident, ptr + 1, ptr2 - ptr);

	if ((ptr = strstr(ptr2 + 1, "[-")) == NULL)
		return -1;
	if ((ptr2 = strstr(ptr + 1, "-]")) == NULL)
		return -1;
	if (ptr2 - ptr - 1 > STRLEN)
		return -1;
	strlcpy(code, ptr + 2, ptr2 - ptr - 1);

	return 0;
}

int verify_code(char *title, char *ident)
{
	FILE *fp;
	char code[STRLEN], realcode[STRLEN];
	char fname[STRLEN];

	if (verify_title(title, ident, code) == -1)
		return -1;

	sethomefile(fname, ident, "mailcheck");
	if ((fp = fopen(fname, "r")) == NULL)
		return -1;

	if ((fgets(realcode, sizeof(realcode), fp)) == NULL) {
		fclose(fp);
		return -1;
	}
	if (realcode[strlen(realcode) - 1] == '\n')
		realcode[strlen(realcode) - 1] = 0;

	fclose(fp);

	return (strcmp(code, realcode) == 0) ? 0 : -1;
}

void write_reginfo(int uid, struct userec urec)
{
	FILE *fp;
	char fname[STRLEN];
	time_t now;

	sethomefile(fname, urec.userid, "register");
	if ((fp = fopen(fname, "w")) == NULL)
		return;

	now = time(NULL);
	fprintf(fp, "usernum: %d, %24.24s\n", uid, ctime(&now));
	fprintf(fp, "userid: %s\n", urec.userid);
	fprintf(fp, "realname: %s\n", urec.realname);
	fprintf(fp, "email: %s\n", urec.email);
	fclose(fp);
}

int send_notice(char *uident, char *fname, char *title)
{
	FILE *fp, *fin;
	char tname[STRLEN], maildir[STRLEN], buf[256];
	time_t now;

	if ((fin = fopen(fname, "r")) == NULL)
		return -1;

	sprintf(tname, "mail/.tmp/mailcheck.%d\n", getpid());
	if ((fp = fopen(tname, "w")) == NULL) {
		fclose(fin);
		return -1;
	}

	now = time(NULL);
	sprintf(buf, "\033[1;41;33m寄信人: %s (自动发信系统)", BBSID);
	fprintf(fp, "%s%*s\033[m\n", buf, (int)(89 - strlen(buf)), " ");
	fprintf(fp, "标  题: %s\n", title);
	fprintf(fp, "发信站: %s (%24.24s)\n", BBSID, ctime(&now));
	fprintf(fp, "来  源: %s\n\n", BBSHOST);

	while (fgets(buf, sizeof(buf), fin))
		fputs(buf, fp);

	fprintf(fp, "\n--\n\033[m\033[1;31m※ 来源:．%s %s．[FROM: %s]\033[m\n",
		BBSNAME, BBSHOST, BBSHOST);

	fclose(fin);
	fclose(fp);

	create_maildir(uident);
	postmail(tname, uident, title, BBSID, 0, NA);
	unlink(tname);
	return 0;
}

int mailcheck(char *title, char *email)
{
	int uid;
	char ident[IDLEN + 2], fname[STRLEN];
	struct userec urec;

	if (verify_code(title, ident) == -1)
		return -1;
	if ((uid = getuser(ident, &urec)) == 0)
		return -1;

	urec.lastjustify = time(NULL);
	urec.userlevel |= PERM_DEFAULT;
	strlcpy(urec.reginfo, email, sizeof(urec.reginfo));
	if (substitute_record(PASSFILE, &urec, sizeof(struct userec), uid) != 0)
		return -1;

	sethomefile(fname, ident, "mailcheck");
	unlink(fname);
	sethomefile(fname, ident, ".regpass");
	unlink(fname);

	send_notice(ident, "etc/smail",  "欢迎加入本站行列");
	write_reginfo(uid, urec);
	regmaillog(ident);

	return 0;
}
