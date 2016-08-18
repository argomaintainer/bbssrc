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

typedef struct {
	char *match;
	char *replace;
} logout;

int
countlogouts(char *filename)
{
	FILE *fp;
	char buf[256];
	int count = 0;

	if ((fp = fopen(filename, "r")) == NULL)
		return 0;

	while (fgets(buf, 255, fp) != NULL) {
		if (strstr(buf, "@logout@") || strstr(buf, "@login@"))
			count++;
	}
	fclose(fp);		/* add by quickmouse 01/03/09 */
	return count + 1;
}

void
user_display(char *filename, int number, int mode)
{
	FILE *fp;
	char buf[256];
	int count = 1;

	clear();
	move(1, 0);
	if ((fp = fopen(filename, "r")) == NULL)
		return;

	while (fgets(buf, 255, fp) != NULL) {
		if (strstr(buf, "@logout@") || strstr(buf, "@login@")) {
			count++;
			continue;
		}
		if (count == number) {
			if (mode == YEA)
				showstuff(buf /*, 0 */ );
			else {
				outs(buf);
			}
		} else if (count > number)
			break;
		else
			continue;
	}
	refresh();
	fclose(fp);
	return;
}

void
countdays(int *year, int *month, int *day, time_t now)
{
	struct tm *GoodTime;
	time_t tmptime;

	GoodTime = localtime(&now);
	GoodTime->tm_year = *year - 1900;
	GoodTime->tm_mon = *month - 1;
	GoodTime->tm_mday = *day;
	GoodTime->tm_hour = 0;
	GoodTime->tm_min = 0;
	tmptime = mktime(GoodTime);
	*year = (tmptime - now) / 86400;
	*month = (tmptime - now - *year * 86400) / 3600;
	*day = (tmptime - now - *year * 86400 - *month * 3600) / 60;
}

char *
horoscope(int month, int day)
{
	char *name[12] = {
		"摩羯", "水瓶", "双鱼", "牡羊", "金牛", "双子",
		"巨蟹", "狮子", "处女", "天秤", "天蝎", "射手"
	};

	switch (month) {
	case 1:
		if (day < 21)
			return (name[0]);
		else
			return (name[1]);
	case 2:
		if (day < 19)
			return (name[1]);
		else
			return (name[2]);
	case 3:
		if (day < 21)
			return (name[2]);
		else
			return (name[3]);
	case 4:
		if (day < 21)
			return (name[3]);
		else
			return (name[4]);
	case 5:
		if (day < 21)
			return (name[4]);
		else
			return (name[5]);
	case 6:
		if (day < 22)
			return (name[5]);
		else
			return (name[6]);
	case 7:
		if (day < 23)
			return (name[6]);
		else
			return (name[7]);
	case 8:
		if (day < 23)
			return (name[7]);
		else
			return (name[8]);
	case 9:
		if (day < 23)
			return (name[8]);
		else
			return (name[9]);
	case 10:
		if (day < 24)
			return (name[9]);
		else
			return (name[10]);
	case 11:
		if (day < 23)
			return (name[10]);
		else
			return (name[11]);
	case 12:
		if (day < 22)
			return (name[11]);
		else
			return (name[0]);
	}
	return ("不详");
}

#ifndef NOEXP
char *
cexp(int exp)
{
	if (exp < 0)
		return GLY_CEXP0;
	if (exp <= 100 )
		return GLY_CEXP1;
	if (exp <= 450 )
		return GLY_CEXP2;
	if (exp <= 850 )
		return GLY_CEXP3;
	if (exp <= 1500 )
		return GLY_CEXP4;
	if (exp <= 2500 )
		return GLY_CEXP5;
	if (exp <= 3000 )
		return GLY_CEXP6;
	if (exp <= 5000 )
		return GLY_CEXP7;

	return GLY_CEXP8;
}

char *
cnumposts(int num)
{
	if(num <= 0 )
		return GLY_CPOST0;
	if(num <= 500)
		return GLY_CPOST1;
	if(num <= 1500)
		return GLY_CPOST2;
	if(num <= 4000)
		return GLY_CPOST3;
	if(num <= 10000)
		return GLY_CPOST4;

	return GLY_CPOST5;
}

char *
cperf(int perf)
{
	if (perf < 0)
		return GLY_CPERF0;
	if (perf <= 5)
		return GLY_CPERF1;
	if (perf <= 12)
		return GLY_CPERF2;
	if (perf <= 35)
		return GLY_CPERF3;
	if (perf <= 50)
		return GLY_CPERF4;
	if (perf <= 90)
		return GLY_CPERF5;
	if (perf <= 140)
		return GLY_CPERF6;
	if (perf > 140 && perf <= 200)
		return GLY_CPERF7;

	return GLY_CPERF8;
}

int
countexp(struct userec *urec)
{
	int exp;

	if (!strcmp(urec->userid, "guest"))
		return -9999;

	exp = urec->numposts + urec->numlogins / 5 +
	      (time(NULL) - urec->firstlogin) / 86400 + urec->stay / 3600;

	return (exp > 0) ? exp : 0;
}

int
countperf(struct userec *urec)
{
	int perf, reg_days;

	if (!strcmp(urec->userid, "guest"))
		return -9999;

	if ((reg_days = (time(NULL) - urec->firstlogin) / 86400 + 1) <= 0)
		return 0;

	perf = ((float)urec->numposts / (float)urec->numlogins +
		(float)urec->numlogins / (float)reg_days) * 10;

	return (perf > 0) ? perf : 0;
}
#endif

/*
void showstuff(buf, limit)
char    buf[256];
int     limit;
*/
void
showstuff(char *buf)
{
	int i, matchfrg, strlength, cnt;
	static char numlogins[10], numposts[10], nummails[10], rgtday[30],
	       lasttime[30], lastjustify[30], thistime[30], stay[10],
	       alltime[20], star[7];
	static int inited = NA;
	char buf2[256], *ptr = NULL, *ptr2 = NULL;
	time_t now;

#ifndef NOEXP
	static char nexp[10], ccexp[20], nperf[10], ccperf[20];
	int uexp, uperf;
#endif

	static logout loglst[] = {
		{ "userid", 	currentuser.userid },
		{ "username", 	currentuser.username },
		{ "realname", 	currentuser.realname },
		{ "address", 	currentuser.address },
		{ "email", 	currentuser.email },
		{ "termtype", 	currentuser.termtype },
		{ "realemail", 	currentuser.reginfo },
		{ "rgtday", 	rgtday },
		{ "login", 	numlogins },
		{ "post",	numposts },
		{ "mail", 	nummails },
		{ "lastlogin", 	lasttime },
		{ "lasthost", 	currentuser.lasthost },
		{ "lastjustify", lastjustify },
		{ "now", 	thistime },
		{ "bbsname", 	BoardName },
		{ "stay", 	stay },
		{ "alltime", 	alltime },
		{ "star", 	star },
		{ "pst", 	numposts },
		{ "log", 	numlogins },
		{ "bbsip", 	BBSIP },
		{ "bbshost", 	BBSHOST },
		{ "version", 	BBSVERSION },
#ifndef NOEXP
		{ "exp",	nexp },
		{ "cexp",	ccexp },
		{ "perf",	nperf },
		{ "cperf",	ccperf },
#endif
		{ NULL, 	NULL },
	};

	if (strchr(buf, '$') == NULL) {
		//if (!limit)
		outs(buf);
		//else
		//      prints("%.82s", buf);
		return;
	}

	/* for ansimore3() */
	if (currentuser.numlogins > 0) {
		if (inited == NA) {
			/* monster: 这些数据在上站期间基本不会改变，初始化一次足矣 */
			inited = YEA;
			snprintf(alltime, sizeof(alltime), "%d小时%d分钟", 
				currentuser.stay / 3600, (currentuser.stay / 60) % 60);
			getdatestring(currentuser.firstlogin);
			strlcpy(rgtday, datestring, sizeof(rgtday));
			getdatestring(currentuser.lastlogin);
			strlcpy(lasttime, datestring, sizeof(lasttime));
			getdatestring(currentuser.lastjustify);
			snprintf(lastjustify, sizeof(lastjustify), "%24.24s", datestring);
			snprintf(numlogins, sizeof(numlogins), "%d", currentuser.numlogins);
			snprintf(star, sizeof(star), "%s座", horoscope(currentuser.birthmonth, currentuser.birthday));
		}

		now = time(NULL);
		getdatestring(now);
		strlcpy(thistime, datestring, sizeof(thistime));
		snprintf(numposts, sizeof(numposts), "%d", currentuser.numposts);
		snprintf(nummails, sizeof(nummails), "%d", currentuser.nummails);
		snprintf(stay, sizeof(stay), "%d", (now - login_start_time) / 60);

#ifndef NOEXP
		uexp = countexp(&currentuser);
		snprintf(nexp, sizeof(nexp), "%d", uexp);
		strlcpy(ccexp, cexp(uexp), sizeof(ccexp));

		uperf = countperf(&currentuser);
		snprintf(nperf, sizeof(nperf), "%d", uperf);
		strlcpy(ccperf, cperf(uperf), sizeof(ccperf));
#endif
	}

	ptr2 = buf;
	while (ptr2 != NULL && ptr2[0] != 0) {
		if ((ptr = strchr(ptr2, '$')) != NULL) {
			matchfrg = 0;
			*ptr = '\0';
			outs(ptr2);
			ptr += 1;
			for (i = 0; loglst[i].match != NULL; i++) {
				if (strstr(ptr, loglst[i].match) == ptr) {
					strlength = strlen(loglst[i].match);
					ptr2 = ptr + strlength;
					for (cnt = 0; *(ptr2 + cnt) == ' '; cnt++) 
						;

					snprintf(buf2, sizeof(buf2), "%-*.*s", cnt ? strlength + cnt : strlength + 1, strlength + cnt, loglst[i].replace);
					outs(buf2);
					ptr2 += (cnt ? (cnt - 1) : cnt);
					matchfrg = 1;
					break;
				}
			}
			if (!matchfrg) {
				prints("$");
				ptr2 = ptr;
			}
		} else {
			//if (!limit)
			outs(ptr2);
			//else
			//      prints("%.82s", ptr2);
			break;
		}
	}
}
