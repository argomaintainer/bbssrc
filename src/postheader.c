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

extern int noreply;
extern int mailtoauthor;

void
check_title(char *title)
{
	if (killwordsp(title) == 0)
		return;
	if (strncmp(title, "Re: ", 4) != 0)
		return;
	title[0] = 'r';
}

int
post_header(struct postheader *header)
{
	int anonyboard = 0;
	char r_prompt[20], mybuf[256], ans[4];
	char titlebuf[TITLELEN];
	struct boardheader *bp;
	static int anonymous = 1;   //Added by cancel

#ifdef RNDSIGN
	int oldset = 0, rnd_sign = 0;
#endif

	if (currentuser.signature > numofsig || currentuser.signature < 0)
		currentuser.signature = 1;

#ifdef RNDSIGN
	if (numofsig > 0) {
		if (DEFINE(DEF_RANDSIGN)) {
			oldset = currentuser.signature;
			srand((unsigned) time(NULL));
			currentuser.signature = (rand() % numofsig) + 1;
			rnd_sign = 1;
		} else {
			rnd_sign = 0;
		}
	}
#endif

	if (header->reply_mode) {
		strcpy(titlebuf, header->title);
		header->include_mode = 'S';
	} else
		titlebuf[0] = '\0';
	bp = getbcache(currboard);
	if (header->postboard)
		anonyboard = bp->flag & ANONY_FLAG;
	header->chk_anony = (anonyboard) ? anonymous : 0;
	while (1) {
		if (header->reply_mode) {
			snprintf(r_prompt, sizeof(r_prompt), "引言模式 [\033[1m%c\033[m]",
				header->include_mode);
		}
		move(t_lines - 4, 0);
		clrtobot();
		prints("\033[m%s \033[1m%s\033[m      %s    %s%s\n",
		       (header->postboard) ? "发表文章于" : "收信人：",
		       header->ds,
		       (anonyboard) ? (header->chk_anony ==
				       1 ? "\033[1m要\033[m使用匿名" :
				       "\033[1m不\033[m使用匿名") : "",
		       (header->
			postboard) ? ((noreply) ? "[本文\033[1;33m不可以\033[m回复" :
				      "[本文\033[1;33m可以\033[m回复") : "",
		       (header->postboard &&
			header->reply_mode) ? ((mailtoauthor) ?
				       ",且\033[1;33m发送\033[m本文至作者信箱]" :
				       ",且\033[1;33m不发送\033[m本文至作者信箱]")
		       : (header->postboard) ? "]" : "");
		prints("使用标题: \033[1m%-50s\033[m\n",
		       (header->title[0] ==
			'\0') ? "[正在设定主题]" : header->title);
#ifdef RNDSIGN
		prints("使用第 \033[1m%d\033[m 个签名档     %s %s",
		       currentuser.signature,
		       (header->reply_mode) ? r_prompt : "",
		       (rnd_sign == 1) ? "[随机签名档]" : "");
#else
		prints("使用第 \033[1m%d\033[m 个签名档     %s",
		       currentuser.signature,
		       (header->reply_mode) ? r_prompt : "");
#endif
		if (titlebuf[0] == '\0') {
			move(t_lines - 1, 0);

			if (header->postboard == YEA || strcmp(header->title, "没主题"))
				strcpy(titlebuf, header->title);

			getdata(t_lines - 1, 0, "标题: ", titlebuf, 55, DOECHO, NA);
			my_ansi_filter(titlebuf);

			if (strcmp(titlebuf, header->title))
				check_title(titlebuf);

			if (titlebuf[0] == '\0') {
				if (header->title[0] != '\0') {
					titlebuf[0] = ' ';
					continue;
				} else
					return NA;
			}
			strcpy(header->title, titlebuf);
			continue;
		}
		move(t_lines - 1, 0);
#ifdef RNDSIGN
		snprintf(mybuf, sizeof(mybuf), "\033[1;32m0\033[m~\033[1;32m%d/V/X\033[m 选/看/随机签名档%s \033[1;32mT\033[m标题%s%s%s，\033[1;32mQ\033[m放弃: ",
#else
		snprintf(mybuf, sizeof(mybuf), "\033[1;32m0\033[m~\033[1;32m%d/V\033[m 选/看签名档%s \033[1;32mT\033[m标题%s%s%s，\033[1;32mQ\033[m放弃: ",
#endif
			numofsig, (header->reply_mode) ?
			"，\033[1;32mS\033[m/\033[1;32mY\033[m/\033[1;32mN\033[m/\033[1;32mR\033[m/\033[1;32mA\033[m引言模式"
			: "", (anonyboard) ? "，\033[1;32mL\033[m匿名" : "",
			(header->postboard) ? "，\033[1;32mU\033[m属性" : "",
			(header->postboard && header->reply_mode) ? "，\033[1;32mM\033[m寄信" : "");
		getdata(t_lines - 1, 0, mybuf, ans, sizeof(ans), DOECHO, YEA);
		if (ans[0] >= '0' && ans[0] <= '9') {
			int sig;

			sig = atoi(ans);
			if (sig <= numofsig && sig >= 0)
				currentuser.signature = sig;
		} else if (ans[0] == 'Q' || ans[0] == 'q') {
			return -1;
		} else if (header->reply_mode &&
			   (ans[0] == 'Y' || ans[0] == 'y' || ans[0] == 'N' || ans[0] == 'n' ||
			    ans[0] == 'A' || ans[0] == 'a' || ans[0] == 'R' || ans[0] == 'r' ||
			    ans[0] == 'S' || ans[0] == 's')) {
			header->include_mode = mytoupper(ans[0]);
		} else if (ans[0] == 'T' || ans[0] == 't') {
			titlebuf[0] = '\0';
		} else if ((ans[0] == 'L' || ans[0] == 'l') && anonyboard) {
			header->chk_anony = (header->chk_anony == 1) ? 0 : 1;
			anonymous = header->chk_anony;
		} else if ((ans[0] == 'U' || ans[0] == 'u') && header->postboard) {
			noreply = ~noreply;
		} else if ((ans[0] == 'M' || ans[0] == 'm') && header->postboard &&
			   header->reply_mode) {
			mailtoauthor = ~mailtoauthor;
		} else if (ans[0] == 'V' || ans[0] == 'v') {
			setuserfile(mybuf, "signatures");
			if (askyn("预设显示前三个签名档, 要显示全部吗", NA, YEA)
			    == YEA)
				ansimore(mybuf, NA);
			else {
				clear();
				ansimore2(mybuf, NA, 0, 18);
			}
#ifdef RNDSIGN
		} else if (ans[0] == 'X' || ans[0] == 'x') {
			if (rnd_sign == 0 && numofsig != 0) {
				oldset = currentuser.signature;
				srand((unsigned) time(NULL));
				currentuser.signature = (rand() % numofsig) + 1;
				rnd_sign = 1;
			} else if (rnd_sign == 1 && numofsig != 0) {
				rnd_sign = 0;
				currentuser.signature = oldset;
			}
			ans[0] = ' ';
#endif
		} else {
			if (header->title[0] == '\0')
				return NA;
			else
				return YEA;
		}
	}
}

