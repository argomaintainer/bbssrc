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

int
offline(void)
{
	int i, oldlevel, silent;
	char buf[STRLEN], lastword[640] = { '\0' };

	modify_user_mode(OFFLINE);
	clear();

	if (HAS_ORGPERM(PERM_SYSOP) || HAS_ORGPERM(PERM_BOARDS) ||
	    HAS_ORGPERM(PERM_ADMINMENU) || HAS_ORGPERM(PERM_SEEULEVELS)) {
		move(1, 0);
		prints("\n\n您有重任在身, 不能随便自杀啦!!\n");
		pressreturn();
		clear();
		return 0;
	}

	if (count_self() > 1) {
		move(1, 0);
		prints("\n\n请您关闭了其它窗口再执行此命令!!\n");
		pressreturn();
		clear();
		return 0;
	}

	/* modified by Gophy at 1999.5.4 */
	if (currentuser.numlogins < 20 || !HAS_ORGPERM(PERM_POST)) {	
		move(1, 0);
		prints("\n\n对不起, 您还未够资格执行此命令!!\n");
		prints("请写信给 SYSOP 说明自杀原因, 谢谢。\n");
		pressreturn();
		clear();
		return 0;
	}

	if (confirm_userident("自杀") == NA)
		return 0;

	move(1, 0);
	prints("\033[1;5;31m警告\033[0;1;31m: 自杀后, 您将无法再用此帐号进入本站！！");
	move(3, 0);
	prints("\033[1;32m但帐号要在 3 天后才会删除。好难过喔 :( .....\033[m");
	move(5, 0);

	i = 0;
	if (askyn("真是舍不得你，你走之前有什么话想说么", NA, NA) == YEA) {
		strcpy(lastword, ">\n>");
		buf[0] = '\0';
		for (i = 0; i < 8; i++) {
			getdata(i + 6, 0, ": ", buf, 77, DOECHO, YEA);
			if (buf[0] == '\0')
				break;
			strcat(lastword, buf);
			strcat(lastword, "\n>");
		}
		if (i == 0) {
			lastword[0] = '\0';
		} else {
			strcat(lastword, "\n\n");
		}
		move(i + 8, 0);
		if (i == 0) {
			prints("哎，你还是什么都不愿意说，是不是还有心思未了？");
		} else if (i <= 4) {
			prints("看着你憔悴的脸，我心都碎了 ... ");
		} else {
			prints("我会记得你的，朋友，我也知道你的离开也是没有办法的事，好走了");
		}
		silent = (i > 0);
	} else {
		silent = askyn("你真的想静静的离开这烦嚣的尘世", NA, NA);
	}

	move(i + 10, 0);
	if (askyn("你确定要离开这个大家庭", NA, NA) == 1) {
		clear();
		oldlevel = currentuser.userlevel;
		currentuser.userlevel &= 0x3F;
		currentuser.userlevel ^= PERM_SUICIDE;
		substitute_record(PASSFILE, &currentuser, sizeof(struct userec), usernum);
		mail_info(lastword, oldlevel, silent);
		modify_user_mode(OFFLINE);
		// kick_user(&uinfo);
		// exit(0);
		abort_bbs();
	}
	return 0;
}

/* Rewrite by cancel at 01/09/16 */
void
getuinfo(FILE *fn, struct userec *userinfo)
{
	int num;
	char buf[40];

	fprintf(fn, "\n他的代号     : %s\n", userinfo->userid);
	fprintf(fn, "他的昵称     : %s\n", userinfo->username);
	fprintf(fn, "真实姓名     : %s\n", userinfo->realname);
	fprintf(fn, "居住住址     : %s\n", userinfo->address);
	fprintf(fn, "电子邮件信箱 : %s\n", userinfo->email);
	fprintf(fn, "注册信息     : %s\n", userinfo->reginfo);
	fprintf(fn, "帐号注册地址 : %s\n", userinfo->ident);
	getdatestring(userinfo->firstlogin);
	fprintf(fn, "帐号建立日期 : %s\n", datestring);
	getdatestring(userinfo->lastlogin);
	fprintf(fn, "最近光临日期 : %s\n", datestring);
	fprintf(fn, "最近光临机器 : %s\n", userinfo->lasthost);
	fprintf(fn, "上站次数     : %d 次\n", userinfo->numlogins);
	fprintf(fn, "文章数目     : %d\n", userinfo->numposts);
	fprintf(fn, "上站总时数   : %d 小时 %d 分钟\n",
		userinfo->stay / 3600, (userinfo->stay / 60) % 60);
	strcpy(buf, "bTCPRp#@XWBA#VS-DOM-F012345678");
	for (num = 0; num < 30; num++)
		if (!(userinfo->userlevel & (1 << num)))
			buf[num] = '-';
	buf[num] = '\0';
	fprintf(fn, "使用者权限   : %s\n\n", buf);
}

/* Rewrite End. */

void
mail_info(char *lastword, int userlevel, int silent)
{
	FILE *fn;
	time_t now;
	char filename[PATH_MAX + 1];

	now = time(NULL);
	getdatestring(now);
	snprintf(filename, sizeof(filename), "tmp/suicide.%s", currentuser.userid);
	if ((fn = fopen(filename, "w")) != NULL) {
		fprintf(fn,
			"\033[1m%s\033[m 已经在 \033[1m%s\033[m 登记自杀了\n以下是他的资料，请保留: \n",
			currentuser.userid, datestring);
		getuinfo(fn, &currentuser);
		fclose(fn);
		postfile(filename, "syssecurity", "登记自杀通知(3天后生效)...", 2);
		unlink(filename);
	}
	snprintf(filename, sizeof(filename), "suicide/suicide.%s", currentuser.userid);
	if (NA == silent) {
		unlink(filename);
		return;
	}
	if ((fn = fopen(filename, "w")) != NULL) {
		fprintf(fn, "发信人: %s (%s), 信区: ID\n", currentuser.userid,
			currentuser.username);
		fprintf(fn, "标  题: %s 的临别赠言\n", currentuser.userid);
		fprintf(fn, "发信站: %s (%24.24s), 站内信件\n\n", BoardName,
			ctime(&now));

		fprintf(fn, "大家好,\n\n");
		fprintf(fn, "我是 %s (%s)。我己经离开这里了。\n\n",
			currentuser.userid, currentuser.username);
		getdatestring(currentuser.firstlogin);
		fprintf(fn,
			"自 %14.14s 至今，我已经来此 %d 次了，在这总计 %d 分钟的网络生命中，\n",
			datestring, currentuser.numlogins,
			currentuser.stay / 60);
		fprintf(fn, "我又如何会轻易舍弃呢？但是我得走了...  点点滴滴－－尽在我心中！\n\n");
		if (lastword != NULL && lastword[0] != '\0') 
			fprintf(fn, "%s", lastword);
		fprintf(fn,
			"朋友们，请把 %s 从你们的好友名单中拿掉吧。因为我己经决定离开这里了!\n\n",
			currentuser.userid);
		fprintf(fn, "或许有朝一日我会回来的。 珍重!! 再见!!\n\n\n");
		getdatestring(now);
		fprintf(fn, "%s 于 %s 留.\n\n", currentuser.userid, datestring);
		fprintf(fn, "--\n\033[1;%dm※ 来源:. %s %s. [FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST,
			currentuser.lasthost);
		fclose(fn);

		// postfile(filename, "notepad", "登记自杀留言...", 2);
		// unlink(filename);
	}
}
