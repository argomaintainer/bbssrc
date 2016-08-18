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

/* monster: 检查日期的有效性 */
int
valid_day(int year, int month, int day)
{
	if (day < 1)
		return NA;

	if (month == 2) {
		if ((year % 4 != 0) || ((year % 400 != 0) && (year % 100 == 0))) {
			if (day >= 29)
				return NA;
		} else {
			if (day > 29)
				return NA;
		}
	}

	if ((month < 8 && month % 2 == 1) || (month >= 8 && month % 2 == 0)) {
		if (day > 31)
			return NA;
	} else {
		if (day >= 31)
			return NA;
	}

	return YEA;
}


//Added by cancel at 02.03.02
int
cmpregrec(void *username_ptr, void *rec_ptr)
{
	char *username = (char *)username_ptr;
	struct new_reg_rec *rec = (struct new_reg_rec *)rec_ptr;

	return (!strcmp(username, rec->userid)) ? YEA : NA;
}

void
display_userinfo(struct userec *u)
{
	int num;
	time_t now;
#if 0
#ifdef REG_EXPIRED
#ifndef AUTOGETPERM
	time_t nextreg;
#endif
#endif
#endif
	move(2, 0);
	clrtobot();
	now = time(NULL);
	set_safe_record();
	prints("您的代号     : %-14s", u->userid);
	prints("昵称 : %-20s", u->username);
	prints("     性别 : %s", (u->gender == 'M' ? "男" : "女"));
	prints("\n真实姓名     : %-40s", u->realname);
	prints("  出生日期 : %d/%d/%d", u->birthmonth, u->birthday, u->birthyear + 1900);
	prints("\n居住住址     : %-38s\n", u->address);

/* 	{
 *     		int tyear, tmonth, tday;
 *
 *		tyear = u->birthyear + 1900;
 *      	tmonth = u->birthmonth;
 *     		tday = u->birthday;
 *     		countdays(&tyear, &tmonth, &tday, now);
 *     		prints("累计生活天数 : %d\n", abs(tyear));
 *  	}
 */

	prints("电子邮件信箱 : %s\n", u->email);
	prints("注册信息     : %s\n", u->reginfo);
	if(HAS_PERM(PERM_ADMINMENU))
		    prints("帐号注册地址 : %s\n", u->ident);
	prints("最近光临机器 : %-22s", u->lasthost);
	prints("终端机形态 : %s\n", u->termtype);
	getdatestring(u->firstlogin);
	prints("帐号建立日期 : %s[距今 %d 天]\n",
	       datestring, (now - (u->firstlogin)) / 86400);
	getdatestring(u->lastlogin);
	prints("最近光临日期 : %s[距今 %d 天]\n",
	       datestring, (now - (u->lastlogin)) / 86400);
#if 0
#ifndef AUTOGETPERM
#ifndef REG_EXPIRED
	getdatestring(u->lastjustify);
	prints("身份确认日期 : %s\n",
	       (u->lastjustify == 0) ? "未曾注册" : datestring);
#else
	if (u->lastjustify == 0)
		prints("身份确认     : 未曾注册\n");
	else {
		prints("身份确认     : 已完成，有效期限: ");
		nextreg = u->lastjustify + REG_EXPIRED * 86400;
		getdatestring(nextreg);
		prints("%14.14s[%s]，还有 %d 天\n",
			datestring, datestring + 23, (nextreg - now) / 86400);
		if (NULL != sysconf_str("REG_EXPIRED_LOCK")) {
			outs("\033[1;31m本站处于半年注册锁定期，暂不需要半年注册\033[0m\n");
		}
	}
#endif
#endif
#endif
/* Added by betterman 06.08.31 */
	prints("身份确认     : %s\n",	(u->userlevel & PERM_WELCOME ) ? "已完成" : "尚未完成");

	prints("文章数目     : %d\n", u->numposts);
	prints("私人信箱     : %d 封\n", u->nummails);
	prints("上站次数     : %d 次      ", u->numlogins);
	prints("上站总时数   : %d 天 %d 小时 %d 分钟\n",
	       u->stay / 86400, (u->stay / 3600) % 24, (u->stay / 60) % 60);
	strcpy(genbuf, "bTCPRD#@XWBA#VS-DOM-F0s2345678");
	for (num = 0; num < strlen(genbuf); num++)
		if (!(u->userlevel & (1 << num)))
			genbuf[num] = '-';
	prints("使用者权限   : %s\n", genbuf);
	prints("\n");
	if (u->userlevel & PERM_SYSOP) {
		prints("  您是本站的站长, 感谢您的辛勤劳动.\n");
	} else if (u->userlevel & PERM_BOARDS) {
		prints("  您是本站的版主, 感谢您的付出.\n");
	} else if (u->userlevel & PERM_LOGINOK) {
		prints("  您的注册程序已经完成, 欢迎加入本站.\n");
	} else if (u->lastlogin - u->firstlogin < 3 * 86400) {
		prints("  新手上路, 请阅读 Announce 讨论区.\n");
	} else {
		prints("  注册尚未成功, 请参考本站进站画面说明.\n");
	}
}

void
check_uinfo(struct userec *u, int MUST)
{
	int changeIT = 0, changed = 0, pos = 2;
	char ans[5];

	while (1) {		// 检查昵称
		changeIT = MUST || (strlen(u->username) < 2);
		/* || (strstr(u->username, "  "))||(strstr(u->username, "　")) */
		if (!changeIT) {
			if (changed) {
				pos++;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		getdata(pos, 0, "请输入您的昵称 (Enter nickname): ", u->username, sizeof(u->username), DOECHO, YEA);
		strlcpy(uinfo.username, u->username, sizeof(uinfo.username));
		update_utmp();
	}

	while (1) {		// 检查真实姓名
		changeIT = MUST || (strlen(u->realname) < 4) || (strstr(u->realname, "  ")) || (strstr(u->realname, "　"));
		if (!changeIT) {
			if (changed) {
				pos += 2;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		move(pos, 0);
		prints("请输入您的真实姓名 (Enter realname):\n");
		getdata(pos + 1, 0, "> ", u->realname, sizeof(u->realname), DOECHO, YEA);
		trim(u->realname);
	}

	while (1) {		// 检查通讯地址
		changeIT = MUST || (strlen(u->address) < 10) || (strstr(u->address, "  ")) || (strstr(u->address, "　"));
		if (!changeIT) {
			if (changed) {
				pos += 2;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		move(pos, 0);
		prints("请输入您的通讯地址 (Enter home address)：\n");
		getdata(pos + 1, 0, "> ", u->address, sizeof(u->address), DOECHO, YEA);
		trim(u->address);
	}

	while (1) {		// 检查邮件地址
		changeIT = MUST || invalid_email(u->email);
		if (!changeIT) {
#ifdef MAILCHECK
			if (changed) {
				pos += 4;
				changed = 0;
			}
#else
			if (changed) {
				pos += 3;
				changed = 0;
			}
#endif
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		move(pos, 0);
		prints("电子信箱格式为: \033[1;37muserid@your.domain.name\033[m\n");
#ifdef MAILCHECK
#ifndef CHECK_ZSUMAIL
		prints("\033[32m本站已经提供\033[33m邮件注册\033[32m功能, 您可以通过邮件快速地通过注册认证.\033[m\n");
#endif
#endif
		prints("请输入电子信箱：\n");
#ifdef MAILCHECK
#ifdef CHECK_ZSUMAIL
		getdata(pos + 2, 0, "> ", u->email, sizeof(u->email), DOECHO, YEA);
#else
		getdata(pos + 3, 0, "> ", u->email, sizeof(u->email), DOECHO, YEA);
#endif
#else
		getdata(pos + 2, 0, "> ", u->email, sizeof(u->email), DOECHO, YEA);
#endif
		trim(u->email);
	}

	while (1) {		// 检查性别
		changeIT = MUST || (strchr("MF", u->gender) == NULL);
		if (changeIT) {
			getdata(pos, 0, "请输入您的性别: M.男 F.女 [M]: ", ans, 2, DOECHO, YEA);
			u->gender = toupper((unsigned int)ans[0]);
			if (ans[0] == '\0')
				u->gender = 'M';
			pos++;
		} else {
			break;
		}
	}

	while (1) {		// 检查出生年
		changeIT = MUST || (u->birthyear < 20) || (u->birthyear > 98);
		if (!changeIT) {
			if (changed) {
				pos++;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		getdata(pos, 0, "请输入您的生日年份(四位数): ", ans, 5, DOECHO, YEA);
		if (atoi(ans) < 1920 || atoi(ans) > 1998) {
			MUST = 1;
			continue;
		}
		u->birthyear = atoi(ans) - 1900;
	}

	while (1) {		// 检查出生月
		changeIT = MUST || (u->birthmonth < 1) || (u->birthmonth > 12);
		if (!changeIT) {
			if (changed) {
				pos++;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}
		getdata(pos, 0, "请输入您的生日月份: ", ans, 3, DOECHO, YEA);
		u->birthmonth = atoi(ans);
	}

	while (1) {		// 检查出生日
		changeIT = MUST ||
		    (valid_day(u->birthyear, u->birthmonth, u->birthday) == 0);

		if (!changeIT) {
			if (changed) {
				pos++;
				changed = 0;
			}
			break;
		} else {
			MUST = 0;
			changed = 1;
		}

		getdata(pos, 0, "请输入您的出生日: ", ans, 3, DOECHO, YEA);
		u->birthday = atoi(ans);
	}
}

int
onekeyactivation(struct userec *lookupuser, int usernum)
{
	int fore_user; /* 区分新旧用户 */
	struct denyheader dh;

	fore_user = (lookupuser->userlevel != PERM_BASIC);

	lookupuser->lastjustify = time(NULL);
	lookupuser->userlevel |= PERM_DEFAULT;
#ifdef AUTHHOST
	lookupuser->userlevel |= PERM_WELCOME;
#endif
   if (search_record("boards/.DENYLIST", &dh, sizeof(struct denyheader), 
							denynames, lookupuser->userid)) 
		lookupuser->userlevel &= ~PERM_POST;
	if (substitute_record(PASSFILE, lookupuser, sizeof(*lookupuser), usernum) == -1) {
     		presskeyfor("系统错误，请联系系统维护员\n");
		return DIRCHANGED;
	}

	if(fore_user){ /* 旧用户 */
		mail_sysfile("etc/Activa_fore_users", lookupuser->userid, "恭喜，今后您可在各地畅游Argo");
	}else{ /* 新用户 */
		mail_sysfile("etc/smail", lookupuser->userid, "欢迎加入本站行列");
	}

	snprintf(genbuf, sizeof(genbuf), "让 %s 通过身分确认.", lookupuser->userid);
	do_securityreport(genbuf, lookupuser, YEA, NULL);  //Henry: 修正形参不一致
	return DIRCHANGED;
}

int
uinfo_query(struct userec *u, int real, int unum)
{
	struct userec newinfo;
	char ans[3], buf[STRLEN], genbuf[STRLEN];
	char src[PATH_MAX + 1], dst[PATH_MAX + 1];
	int i, mailchanged = NA, fail = 0;
	time_t now;
	struct tm *tmnow;

	/* monster: 拥有SYSOP权的ID资料不能被修改 */
	if (strcmp(currentuser.userid, "SYSOP")) {
		if ((strcmp(u->userid, currentuser.userid)) && (u->userlevel & PERM_SYSOP) && real) {
			getdata(t_lines - 1, 0, "请选择 (0)结束 [0]: ", ans, 2, DOECHO, YEA);
			return 0;
		}
	}

	memcpy(&newinfo, u, sizeof (currentuser));
	getdata(t_lines - 1, 0, real ?
		"请选择 (0)结束 (1)修改资料 (2)设定密码 (3)改 ID (4)更改系统称号 (5)激活ID [0]: "
		: "请选择 (0)结束 (1)修改资料 (2)设定密码 (3)选签名档 [0]: ",
		ans, 2, DOECHO, YEA);
	clear();
	refresh();
	now = time(NULL);
	tmnow = localtime(&now);

	i = 3;
	move(i++, 0);
	if (ans[0] != '3' || real)
		prints("使用者代号: %s\n", u->userid);
	switch (ans[0]) {
	case '1':
		move(1, 0);
		outs("请逐项修改,直接按 <ENTER> 代表使用 [] 内的资料。\n");
		snprintf(genbuf, sizeof(genbuf), "昵称 [%s]: ", u->username);
		do {
			getdata(5, 0, genbuf, buf, NICKNAMELEN, DOECHO, YEA);
			if (!buf[0])
				break;
		} while (strlen(buf) < 2);
		if (buf[0]) {
			strlcpy(newinfo.username, buf, sizeof(newinfo.username));
		}

		snprintf(genbuf, sizeof(genbuf), "真实姓名 [%s]: ", u->realname);
		getdata(6, 0, genbuf, buf, NAMELEN, DOECHO, YEA);
		if (buf[0]) {
			strlcpy(newinfo.realname, buf, sizeof(newinfo.realname));
		}

		snprintf(genbuf, sizeof(genbuf), "居住地址 [%s]: ", u->address);
		do {
			getdata(7, 0, genbuf, buf, STRLEN - 10, DOECHO, YEA);
			if (!buf[0])
				break;
		} while (strlen(buf) < 10);
		if (buf[0]) {
			strlcpy(newinfo.address, buf, sizeof(newinfo.address));
		}

		snprintf(genbuf, sizeof(genbuf), "电子信箱 [%s]: ", u->email);
		do {
			getdata(8, 0, genbuf, buf, 48, DOECHO, YEA);
		} while (buf[0] & invalid_email(buf));

		if (buf[0]) {
#ifdef MAILCHECK
#ifdef MAILCHANGED
			if (!strcmp(u->userid, currentuser.userid))
				mailchanged = YEA;
#endif
#endif
			strlcpy(newinfo.email, buf, sizeof(newinfo.email));
		}

		snprintf(genbuf, sizeof(genbuf), "终端机形态 [%s]: ", u->termtype);
		getdata(9, 0, genbuf, buf, 16, DOECHO, YEA);
		if (buf[0]) {
			strlcpy(newinfo.termtype, buf, sizeof(newinfo.termtype));
		}

		snprintf(genbuf, sizeof(genbuf), "出生年 [%d]: ", u->birthyear + 1900);
		do {
			getdata(10, 0, genbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] && (atoi(buf) <= 1920 || atoi(buf) >= 1998));
		if (buf[0]) {
			newinfo.birthyear = atoi(buf) - 1900;
		}

		snprintf(genbuf, sizeof(genbuf), "出生月 [%d]: ", u->birthmonth);
		do {
			getdata(11, 0, genbuf, buf, 3, DOECHO, YEA);
		} while (buf[0] && (atoi(buf) < 1 || atoi(buf) > 12));
		if (buf[0]) {
			newinfo.birthmonth = atoi(buf);
		}

		snprintf(genbuf, sizeof(genbuf), "出生日 [%d]: ", u->birthday);
		do {
			getdata(12, 0, genbuf, buf, 3, DOECHO, YEA);
		} while (valid_day(newinfo.birthyear, newinfo.birthmonth, newinfo.birthday) == NA);
		if (buf[0]) {
			newinfo.birthday = atoi(buf);
		}

		snprintf(genbuf, sizeof(genbuf), "性别 M.男 F.女 [%c]: ", u->gender);
		for (;;) {
			getdata(13, 0, genbuf, buf, 2, DOECHO, YEA);

			if (buf[0] == 'M' || buf[0] == 'm') {
				newinfo.gender = 'M';
			} else if (buf[0] == 'F' || buf[0] == 'f') {
				newinfo.gender = 'F';
			} else if (buf[0] != '\0') {
				continue;
			}
				
			break;
		}

		i = 14;
		if (real) {
			snprintf(genbuf, sizeof(genbuf), "上线次数 [%d]: ", u->numlogins);
			getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
			if (buf[0] != '\0' && atoi(buf) >= 0)
				newinfo.numlogins = atoi(buf);

			snprintf(genbuf, sizeof(genbuf), "发表文章数 [%d]: ", u->numposts);
			getdata(i++, 0, genbuf, buf, 10, DOECHO, YEA);
			if (buf[0] != '\0' && atoi(buf) >= 0)
				newinfo.numposts = atoi(buf);
		}
		break;
	case '2':
		if (!real) {
			getdata(i++, 0, "请输入原密码: ", buf, PASSLEN, NOECHO, YEA);
			if (*buf == '\0' || !checkpasswd2(buf, &currentuser)) {
				prints("\n\n很抱歉, 您输入的密码不正确。\n");
				fail++;
				break;
			}
		}
		getdata(i++, 0, "请设定新密码: ", buf, PASSLEN, NOECHO, YEA);
		if (buf[0] == '\0') {
			prints("\n\n密码设定取消, 继续使用旧密码\n");
			fail++;
			break;
		}
		strlcpy(genbuf, buf, PASSLEN);
		getdata(i++, 0, "请重新输入新密码: ", buf, PASSLEN, NOECHO,
			YEA);
		if (strncmp(buf, genbuf, PASSLEN)) {
			prints("\n\n新密码确认失败, 无法设定新密码。\n");
			fail++;
			break;
		}
		setpasswd(buf, &newinfo);
		break;
	case '3':
		if (!real) {
			snprintf(genbuf, sizeof(genbuf), "目前使用签名档 [%d]: ", u->signature);
			getdata(i++, 0, genbuf, buf, 16, DOECHO, YEA);
			if (atoi(buf) > 0)
				newinfo.signature = atoi(buf);
		} else {
			struct user_info uin;

			if (!strcmp(u->userid, currentuser.userid)) {
				outs("\n对不起，不能更改自己的 ID。");
				fail++;
			} else if (t_search_ulist(&uin, t_cmpuids, unum, NA, NA) != 0) {
				outs("\n对不起，该用户目前正在线上。");
				fail++;
			} else if (!strcmp(u->userid, "SYSOP")) {
				outs("\n对不起，您不可以修改 SYSOP 的 ID。");
				fail++;
			} else {
				getdata(i++, 0, "新的使用者代号: ", genbuf, IDLEN + 1, DOECHO, YEA);
				killwordsp(genbuf);
				if (genbuf[0] != '\0' && strcmp(genbuf, u->userid)) {
					if (strcasecmp(genbuf, u->userid) && getuser(genbuf, NULL)) {
						outs("\n对不起! 已经有同样 ID 的使用者\n");
						fail++;
					} else {
						int temp;
						char new_passwd[9];

						strlcpy(newinfo.userid, genbuf, sizeof(newinfo.userid));

						/* monster: 随机生成一个密码给用户 */
						srandom(time(NULL));
						new_passwd[0] = 0;
						for (temp = 0; temp < 8; temp++)
							new_passwd[temp] = (random() % 95) + 33;
						prints("  新的用户密码: %s\n", new_passwd);
						setpasswd(new_passwd, &newinfo);
					}
				} else {
					outs("\n使用者代号没有改变\n");
					fail++;
				}
			}
		}
		break;
	case '4':		/* monster: 修改用户身份/称号 */
		clear();
		if (!real)
			return 0;
		prints("更改使用者系统称号\n\n设定使用者 '%s' 的系统称号\n\n", u->userid);
		newinfo.usertitle = setperms(newinfo.usertitle, "系统称号", NUMTITLES, showtitleinfo);
		break;
/* freestyler: 1键激活 */
	case '5':
		if( !real)
			return 0;
		clear();
		prints("激活 %s", u->userid);

		if (askyn("确定要改变吗", NA, YEA) == YEA)  
			onekeyactivation(u, unum);
	default:
		clear();
		return 0;
	}
	if (fail != 0) {
		pressreturn();
		clear();
		return 0;
	}
	if (askyn("确定要改变吗", NA, YEA) == YEA) {
		if (real) {
			char secu[STRLEN];

			snprintf(secu, sizeof(secu), "修改 %s 的基本资料或密码。", u->userid);
			do_securityreport(secu, u, YEA, NULL);
		}
		if (strcmp(u->userid, newinfo.userid)) {
			snprintf(src, sizeof(src), "mail/%c/%s", mytoupper(u->userid[0]), u->userid);
			snprintf(dst, sizeof(dst), "mail/%c/%s", mytoupper(newinfo.userid[0]), newinfo.userid);
			rename(src, dst);
			sethomepath(src, u->userid);
			sethomepath(dst, newinfo.userid);
			rename(src, dst);
			sethomefile(src, u->userid, "register");
			unlink(src);
			sethomefile(src, u->userid, "register.old");
			unlink(src);
			setuserid(unum, newinfo.userid);
		}
		if (!strcmp(u->userid, currentuser.userid)) {
			strlcpy(uinfo.username, newinfo.username, sizeof(uinfo.username));
			#ifdef NICKCOLOR
			renew_nickcolor(newinfo.usertitle);
			#endif
			update_utmp();
		}
#ifdef MAILCHECK
#ifdef MAILCHANGED
		if ((mailchanged == YEA) && !HAS_PERM(PERM_SYSOP)) {
			if (!invalidaddr(newinfo.email)
			    && !invalid_email(newinfo.email)
			    && strstr(newinfo.email, BBSHOST) == NULL

#ifdef CHECK_ZSUMAIL
			    && (strstr(newinfo.email, "@student.sysu.edu.cn")
				|| strstr(newinfo.email, "@mail.sysu.edu.cn")
				|| strstr(newinfo.email, "@mail2.sysu.edu.cn")
				|| strstr(newinfo.email, "@mail3.sysu.edu.cn"))
			    
#endif
			) {
				strlcpy(u->email, newinfo.email, sizeof(u->email));
				send_regmail(u);
			} else {
				move(t_lines - 4, 0);
				prints("您所填的电子邮件地址 【\033[1;33m%s\033[m】\n"
				       "恕不受本站承认，系统不会投递注册信，请把它修正好...",
				       newinfo.email);
				pressanykey();
				return 0;
			}
		}
#endif
#endif
		memcpy(u, &newinfo, sizeof(currentuser));
#ifdef MAILCHECK
#ifdef MAILCHANGED
		if ((mailchanged == YEA) && !HAS_PERM(PERM_SYSOP)) {
			newinfo.userlevel &= ~(PERM_LOGINOK | PERM_PAGE | PERM_MESSAGE | PERM_SENDMAIL);
			sethomefile(src, newinfo.userid, "register");
			sethomefile(dst, newinfo.userid, "register.old");
			rename(src, dst);
		}
#endif
#endif
//Added by cancel at 01.12.30
		if (!real) {
			set_safe_record();
			newinfo.numposts = currentuser.numposts;
			newinfo.userlevel = currentuser.userlevel;
			newinfo.numlogins = currentuser.numlogins;
			newinfo.stay = currentuser.stay;
			newinfo.usertitle = currentuser.usertitle;
		}
		substitute_record(PASSFILE, &newinfo, sizeof (newinfo), unum);
	}
	clear();
	return 0;
}

int
x_info(void)
{
	if (!guestuser) {
		modify_user_mode(GMENU);
		display_userinfo(&currentuser);
		uinfo_query(&currentuser, 0, usernum);
	}
	return 0;
}

#ifndef AUTHHOST
void
getfield(int line, char *info, char *desc, char *buf, int len)
{
	char prompt[STRLEN];

	move(line, 0);
	prints("  原先设定: %-20.20s \033[1;32m(%s)\033[m", (buf[0] == '\0') ? "(未设定)" : buf, info);
	snprintf(prompt, sizeof(prompt), "  %s: ", desc);
	getdata(line + 1, 0, prompt, genbuf, len, DOECHO, YEA);
	if (genbuf[0] != '\0')
		strlcpy(buf, genbuf, len);
	clear_line(line);
	prints("  %s: %s\n", desc, buf);
}

int
x_fillform(void)
{
	char rname[NAMELEN], addr[STRLEN];
	char phone[STRLEN], dept[STRLEN], assoc[STRLEN];
	char ans[5], *mesg;
	FILE *fn;
	struct new_reg_rec regrec;

	if (guestuser)
		return -1;

	modify_user_mode(NEW);
	clear();
	move(2, 0);
	clrtobot();

	if (currentuser.userlevel & PERM_LOGINOK) {
		prints("您已经完成本站的使用者注册手续, 欢迎加入本站的行列.");
		pressreturn();
		return 0;
	}

#ifdef PASSAFTERTHREEDAYS
	if (currentuser.lastlogin - currentuser.firstlogin < 3 * 86400) {
		prints("您首次登入本站未满三天(72个小时)...\n");
		prints("请先四处熟悉一下，在满三天以后再填写注册单。");
		pressreturn();
		return 0;
	}
#endif

/*
	if ((fn = fopen("new_register", "r")) != NULL) {
		while (fgets(genbuf, STRLEN, fn) != NULL) {
			if ((ptr = strchr(genbuf, '\n')) != NULL)
				*ptr = '\0';
			if (strncmp(genbuf, "userid: ", 8) == 0
			    && strcmp(genbuf + 8, currentuser.userid) == 0) {
				fclose(fn);
				prints("站长尚未处理您的注册申请单, 您先到处看看吧.");
				pressreturn();
				return 0;
			}
		}
		fclose(fn);
	}
*/

	if (search_record("new_register.rec", &regrec, sizeof (regrec),
			  cmpregrec, currentuser.userid)) {
		prints("站长尚未处理您的注册申请单, 您先到处看看吧.");
		pressreturn();
		return 0;
	}

	strlcpy(rname, currentuser.realname, sizeof(rname));
	strlcpy(addr, currentuser.address, sizeof(addr));
	dept[0] = phone[0] = assoc[0] = '\0';
	while (1) {
		move(3, 0);
		clrtoeol();
		prints("%s 您好, 请据实填写以下的资料:\n", currentuser.userid);
		getfield(6, "请用中文,不能输入的汉字请用拼音", "真实姓名",
			 rname, NAMELEN);
		getfield(8, "学校系级或单位全称", "学校系级", dept, STRLEN);
		getfield(10, "请具体到寝室或门牌号码", "目前住址", addr,
			 STRLEN);
		getfield(12, "包括可联络时间", "联络电话", phone, STRLEN);
		getfield(14, "校友会或毕业学校", "校 友 会", assoc, STRLEN);
		mesg = "以上资料是否正确, 按 Q 放弃注册 (Y/N/Quit)? [Y]: ";
		getdata(t_lines - 1, 0, mesg, ans, 3, DOECHO, YEA);
		if (ans[0] == 'Q' || ans[0] == 'q')
			return 0;
		if (ans[0] != 'N' && ans[0] != 'n')
			break;
	}
	strlcpy(currentuser.realname, rname, sizeof(currentuser.realname));
	strlcpy(currentuser.address, addr, sizeof(currentuser.address));
/*	if ((fn = fopen("new_register", "a")) != NULL) {
		now = time(NULL);
		getdatestring(now);
		fprintf(fn, "usernum: %d, %s\n", usernum, datestring);
		fprintf(fn, "userid: %s\n", currentuser.userid);
		fprintf(fn, "realname: %s\n", rname);
		fprintf(fn, "dept: %s\n", dept);
		fprintf(fn, "addr: %s\n", addr);
		fprintf(fn, "phone: %s\n", phone);
		fprintf(fn, "assoc: %s\n", assoc);
		fprintf(fn, "----\n");
		fclose(fn);
	}
*/
//Rewrite by cancel at 02.03.10 use new struct in register file
	regrec.regtime = time(NULL);
	regrec.usernum = usernum;
	strcpy(regrec.userid, currentuser.userid);
	strcpy(regrec.rname, rname);
	strcpy(regrec.dept, dept);
	strcpy(regrec.addr, addr);
	strcpy(regrec.phone, phone);
	strcpy(regrec.assoc, assoc);
	regrec.Sname = count_same_reg(currentuser.realname, '1', NA);
	regrec.Slog = count_same_reg(currentuser.lasthost, '3', NA);
	regrec.Sip = count_same_reg(currentuser.ident, '2', NA);
	regrec.mark = ' ';
	append_record("new_register.rec", &regrec, sizeof (regrec));
//Rewrite end

	setuserfile(genbuf, "mailcheck");
	if ((fn = fopen(genbuf, "w")) != NULL) {
		fprintf(fn, "usernum: %d\n", usernum);
		fclose(fn);
	}
	return 0;
}
#endif

#ifdef AUTHHOST
/* <------- Added by betterman 06/07/27 -------> */
static int col[3] = { 2, 24, 48 };
static int ccol[3] = { 0, 22, 46 };
static int pagetotal;

void
dept_choose_redraw(char *title, slist *list, int current, int pagestart, int pageend)
{
	int i, j, len1, len2;

	len1 = 39 - strlen(BoardName) / 2;
	len2 = len1 - strlen(title);

	clear();
	move(0, 0);
	prints("\033[1;33;44m%s\033[1;37m%*s%s%*s\033[m\n", title, len2, " ", BoardName, len1, " ");
	prints("选择[\033[1;32mRtn\033[m] 取消[\033[1;32mq\033[m] \n");
	prints("\033[1;37;44m  名  单                名  单                  名  单                        \033[m\n");

	for (i = pagestart, j = 3; i < pageend && i < list->length; i++) {
		move(j, col[i % 3]);
		if(strlen(list->strs[i]) > 22)//处理专业名太长
			prints("%22s",list->strs[i]);
		else
			outs(list->strs[i]);
		if (i % 3 == 2) ++j;
	}

	update_endline();
}


int
dept_choose(char *buf, int buf_len, char *listfile, char *title/*, int (*callback)(int action, slist *list, char *uident)*/)
{
	slist *list;
	int current = 0, len, pagestart, pageend, oldx, oldy;

	if ((list = slist_init()) == NULL)
		return 0;
	slist_loadfromfile(list, listfile);

	oldy = 3;
	oldx = 0;
	pagetotal = (t_lines - 4) * 3;
	pagestart = 0;
	pageend = pagetotal - 1;
	dept_choose_redraw(title, list, current, pagestart, pageend);
	if (list->length > 0) {
		move(3, 0);
		outc('>');
	}

	while (1) {
		switch (egetch()) {
		case 'h':
			show_help("help/listedithelp");
			dept_choose_redraw(title, list, current, pagestart, pageend);
			break;
		case '\n':
			strlcpy(buf, list->strs[current], buf_len);
			goto out;
		case 'q':
			buf[0] = '\0';
			goto out;
		case KEY_HOME:
			if (list->length == 0 || current == 0)
				goto unchanged;
			current = 0;
			break;
		case KEY_END:
			if (list->length == 0 || current == list->length - 1)
				goto unchanged;
			current = list->length - 1;
			break;
		case KEY_LEFT:
			if (current == 0)
				goto unchanged;
			current--;
			break;
		case KEY_RIGHT:
			if (current == list->length - 1)
				goto unchanged;
			current++;
			break;
		case KEY_UP:
			if (current < 3)
				goto unchanged;
			current -= 3;
			break;
		case KEY_DOWN:
			if (current >= list->length - 3)
				goto unchanged;
			current += 3;
			break;
		case KEY_PGUP:
			if (current < pagetotal)
				goto unchanged;
			current -= pagetotal;
		case KEY_PGDN:
			if (current >= list->length - pagetotal)
				goto unchanged;
			current += pagetotal;
			break;
		default:
			goto unchanged;
		}

		if (current - current % pagetotal != pagestart || current - current % pagetotal + pagetotal != pageend) {
			pagestart = current - current % pagetotal;
			pageend = current - current % pagetotal + pagetotal;
			dept_choose_redraw(title, list, current, pagestart, pageend);
		} else {
			move(oldy, oldx);
			outc(' ');
		}

		if (list->length > 0) {
			oldy = 3 + (current - pagestart) / 3;
			oldx = ccol[current % 3];
			move(oldy, oldx);
			outc('>');
		}
unchanged: ;
	}

out:
	clear();
	len = list->length;
	slist_savetofile(list, listfile);
	slist_free(list);
	return len;
}
#endif

int
countauths(void *uentp_ptr, int unused)
{
	static int totalusers;
	struct userec *uentp = (struct userec *)uentp_ptr;

	if (uentp == NULL) {
		int c = totalusers;

		totalusers = 0;
		return c;
	}
	if (uentp->userlevel & (PERM_WELCOME | PERM_BASIC) && 
                    memcmp(uentp->reginfo,genbuf, MD5_PASSLEN) == 0) /* alarm: strncmp() , but not strcmp() */
		totalusers++;
	return 0;
}

int multi_auth_check(unsigned char auth[MD5_PASSLEN])
{
	strncpy(genbuf, (const char*) auth, MD5_PASSLEN);
	countmails(NULL, 0);
	if (apply_record(PASSFILE, countauths, sizeof (struct userec)) == -1) {
		return 0;
	}
	return countmails(NULL, 0);	
}

int check_auth_info(struct new_reg_rec *regrec/*, int graduate, char *realname, char *birthday, char *dept, char *account*/)
{
	FILE *fp;
	char name[STRLEN], dept[STRLEN], birth[STRLEN], account[STRLEN], birthday[11];
	char buf[256];
	char *ptr, *ptr2;
	int passover;

	clear();

	setauthfile(genbuf, regrec->graduate);

/* 06/10/10 betterman: 99届资料外泄.封禁99届资料 */
	if(regrec->graduate == 2003)
		return -1;

/* 09/05/21 rovingcloud: 应monson要求封禁自动注册033521*, 资料外泄 */
	if(strstr(regrec->account, "033521") == regrec->account) 
		return -1;

	fp = fopen(genbuf,"r");
	if(fp == NULL){
		presskeyfor("没有该毕业年份的资料, 激活失败");
		return -1;
	}

	sprintf(birthday, "%.4d-%.2d-%.2d", regrec->birthyear + 1900, regrec->birthmonth, regrec->birthday);
	while(fgets(buf,256,fp) != NULL)
	{
		passover = 0;
		ptr = ptr2 = buf;
		if(*ptr == '\0' || *ptr == '\n' || *ptr == '\r' || *ptr == '#')
			continue;

		if((ptr = strchr(ptr,';')) == NULL)	
			continue;
		if (ptr - ptr2 - 1 > STRLEN)
			continue;
		strlcpy(name, ptr2, ptr - ptr2 +1);		
		if(strcmp(name, regrec->rname) != 0)
			continue;

		if((ptr2 = strchr(ptr+1,';')) == NULL)	
			continue;		
		if (ptr2 - ptr - 1 > STRLEN)
			continue;
		strlcpy(dept, ptr+1, ptr2 - ptr );
		if(strlen(dept) == 0 ){
			passover++;
		}else if(strcmp(dept, regrec->dept) != 0)
			continue;

		if((ptr = strchr(ptr2+1,';')) == NULL)	
			continue;		
		if (ptr - ptr2 - 1 > STRLEN)
			continue;
		strlcpy(account, ptr2+1, ptr - ptr2 );
		if(strlen(account) == 0 ){
			passover++;
		}else if(strcmp(account,regrec->account) != 0 &&
		            (strncmp(account,"0",1) !=0 || strcmp(account+1,regrec->account) != 0 ) ) /* 模糊首位的0 */
			continue;

		if((ptr2 = strchr(ptr+1,';')) == NULL)	
			continue;		
		if (ptr2 - ptr - 1 > STRLEN)
			continue;
		strlcpy(birth, ptr+1, ptr2 - ptr );
		if (strlen(birth) == 7){ //生日字段形如: 1985-11
			if(strncmp(birth,birthday,7) != 0 ) 
				continue;
		}else if(strlen(birth) == 10){  //生日字段形如: 1985-11-16
			if(strcmp(birth,birthday) != 0 && 
			   (strncmp(birth, birthday, 7)!=0 || strncmp(birth+8,"01",2)!=0 ) &&  /* 若记录里生日日期是01号而用户输入对年月则通过 */
			   (strncmp(birth, birthday, 4)!=0 || strncmp(birth+5,"01-01",5)!=0  ) ) /* 若记录里生日日期是01-01而用户输入对年则通过 */
				continue;
		}
		else /* 缺生日字段或者字段长度不符合规范 */
			passover++;

		if(passover >= 2) /* 缺两个或两个以上字段则不通过 */
			continue;
		
		igenpass(buf, regrec->rname, regrec->auth);
		return YEA;

	}
	fclose(fp);
	return NA;
}

#ifdef AUTHHOST
int
auth_fillform(struct userec *u, int unum)
{
	char buf[STRLEN];
	char secu[STRLEN];
	struct new_reg_rec regrec;
	struct userec newinfo;
	FILE *authfile;
	static int fail_count = 0;
	struct tm time_s;
	time_t time_t_i = time(NULL);
	int fore_user = (u->userlevel != PERM_BASIC); /* 区分新旧用户 */

	if (guestuser)
		return -1;
	
	if(strcmp(u->userid,"SYSOP") == 0)
		return YEA;

	modify_user_mode(NEW);
	clear();
	move(0, 0);
	//clrtobot();

	if(fail_count >= 3){
		presskeyfor("验证错误过多，请勿再验证\n");
		return NA;
	}

#ifdef PASSAFTERTHREEDAYS
	if (u->lastlogin - u->firstlogin < 3 * 86400) {
		prints("您首次登入本站未满三天(72个小时)...\n");
		prints("请先四处熟悉一下，在满三天以后再激活帐号。");
		pressreturn();
		return 0;
	}
#endif
	regrec.regtime = time(NULL);

	outs("在完成激活操作前, 系统需要对您的身份进行确认: ");
	getdata(1, 0, "请输入您的密码: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0' || !checkpasswd2(buf, u))
		goto auth_fail;

	do {
		getdata(2, 0, "毕业年份(4位) : ", buf, 5, DOECHO, YEA);
	} while (buf[0] && (atoi(buf) <= 1920 || atoi(buf) >= 2020));
	if (!buf[0]) 
		goto auth_fail;
	gmtime_r(&time_t_i, &time_s);
	if(time_s.tm_mon >= 6){ /* second half year */
	  if(time_s.tm_year + 1900 < atoi(buf)){
	    presskeyfor("非毕业生请使用校内邮箱激活\n");
	    goto auth_fail;
	  }	    
	}else{ /* first half year */
	  if(time_s.tm_year + 1900 <= atoi(buf)){
	    presskeyfor("非毕业生请使用校内邮箱激活\n");
	    goto auth_fail;
	  }
	}
	regrec.graduate = atoi(buf);

	snprintf(genbuf, sizeof(genbuf), "真实姓名 [%s]: ", u->realname);
	getdata(3, 0, genbuf, regrec.rname, NAMELEN, DOECHO, YEA);
	if (!regrec.rname[0]) 
		strlcpy(regrec.rname, u->realname, sizeof(u->realname));

	snprintf(genbuf, sizeof(genbuf), "出生年 [%d]: ", u->birthyear + 1900);
	do {
		getdata(4, 0, genbuf, buf, 5, DOECHO, YEA);
	} while (buf[0] && (atoi(buf) <= 1920 || atoi(buf) >= 1998));
	if (buf[0]) {
		regrec.birthyear = atoi(buf) - 1900;
	}else{
		regrec.birthyear = u->birthyear;
	}

	snprintf(genbuf, sizeof(genbuf), "出生月 [%d]: ", u->birthmonth);
	do {
		getdata(5, 0, genbuf, buf, 3, DOECHO, YEA);
	} while (buf[0] && (atoi(buf) < 1 || atoi(buf) > 12));
	if (buf[0]) {
		regrec.birthmonth = atoi(buf);
	}else{
		regrec.birthmonth = u->birthmonth;
	}

	snprintf(genbuf, sizeof(genbuf), "出生日 [%d]: ", u->birthday);
	do {
		getdata(6, 0, genbuf, buf, 3, DOECHO, YEA);
	} while (buf[0] && (atoi(buf) < 1 || atoi(buf) > 31));
	if (buf[0]) {
		regrec.birthday = atoi(buf);
	}else{
		regrec.birthday = u->birthday;
	}
	if(valid_day(regrec.birthyear, regrec.birthmonth, regrec.birthday) == NA) goto auth_fail;

	getdata(7, 0, "请问您的学号 : ",
		regrec.account, STRLEN, DOECHO, YEA);
	if (!regrec.account[0])
		goto auth_fail;

	do {
		getdata(8, 0, "目前住址,请具体到寝室或门牌号码: ",
			regrec.addr, STRLEN, DOECHO, YEA);
	} while (!regrec.addr[0] || strlen(regrec.addr) < 4 );
	
	do {
		getdata(9, 0, "联络电话, 包括可联络时间: ",  
			regrec.phone, STRLEN, DOECHO, YEA);
	} while (!regrec.phone[0] || strlen(regrec.phone) < 8 );

	setdeptfile(genbuf, regrec.graduate);
	if (!dashf(genbuf)) {
		clear();
		prints("错误的毕业年份或者系统尚没提供该毕业年份的资料,请使用邮箱激活\n");
	}
	dept_choose(regrec.dept, sizeof(regrec.dept), genbuf, "专业选择");
	if (!regrec.dept[0])
		goto auth_fail;	

	if(check_auth_info(&regrec) == YEA){

		if(multi_auth_check(regrec.auth) >= MULTIAUTH){
			presskeyfor("\n\033[1;32m 警告: 当前资料已经激活过多! 请勿再试!\033[m\n");
			return NA;
		}				

		memcpy(&newinfo, u, sizeof (newinfo));	
		set_safe_record();
		strcpy(newinfo.realname, regrec.rname);
		newinfo.birthyear = regrec.birthyear;		
		newinfo.birthmonth = regrec.birthmonth;
		newinfo.birthday = regrec.birthday;
      newinfo.lastjustify = time(NULL);
		strcpy(newinfo.address, regrec.addr);
		memcpy(newinfo.reginfo, regrec.auth, MD5_PASSLEN);
		newinfo.reginfo[MD5_PASSLEN] = 0;
		memcpy(u, &newinfo, sizeof(newinfo));
      newinfo.userlevel |= ( PERM_WELCOME | PERM_DEFAULT );
      if (deny_me_fullsite()) newinfo.userlevel &= ~PERM_POST;

      if (substitute_record(PASSFILE, &newinfo, sizeof (newinfo), unum) == -1) {
      	presskeyfor("系统错误，请联系系统维护员\n");
			return NA;
      }else{
      	outs("恭贺您!! 您的帐号已顺利激活.\n");
			pressanykey();
			if(fore_user){ /* 旧用户 */
				mail_sysfile("etc/Activa_fore_users", u->userid, "恭喜，今后您可在各地畅游Argo");
			}else{ /* 新用户 */
				mail_sysfile("etc/smail", u->userid, "欢迎加入本站行列");
			}
			snprintf(secu, sizeof(secu), "激活 %s 的帐号", newinfo.userid);
			//todo : 改报告形式
			securityreport2(secu, YEA, NULL);
        	}
		
		setuserfile(buf, ".regpass"); 
		unlink(buf);
		setuserfile(buf, "auth");
		if (dashf(buf)) {
			setuserfile(genbuf, "auth.old");
			rename(buf, genbuf);
		}
		if ((authfile = fopen(buf, "w")) != NULL) {
			fprintf(authfile, "unum: %d, %s", unum,
				ctime(&(regrec.regtime)));
			fprintf(authfile, "userid: %s\n", u->userid);
			fprintf(authfile, "realname: %s\n", regrec.rname);
			fprintf(authfile, "dept: %s\n", regrec.dept);
			fprintf(authfile, "addr: %s\n", regrec.addr);
			fprintf(authfile, "phone: %s\n", regrec.phone);
			fprintf(authfile, "account: %s\n", regrec.account);
			//fprintf(authfile, "assoc: %s\n", regrec.assoc);			
			fprintf(authfile, "birthday: %d\n",regrec.birthyear + 1900);			
			fprintf(authfile, "birthday: %d\n",regrec.birthmonth);
			fprintf(authfile, "birthday: %d\n",regrec.birthday);
			fprintf(authfile, "graduate: %d\n",regrec.graduate);
			fprintf(authfile, "auth: %s\n",regrec.auth + 1);
			time_t now = time(NULL);
			fprintf(authfile, "Date: %s", ctime(&now));
			fprintf(authfile, "Approved: %s", u->userid);
			fclose(authfile);
		}
		return YEA;
	}else{	
		fail_count++;		
		prints("第 %d 次验证错误 ! \n", fail_count);
		if(fail_count >= 3){ 
			regrec.regtime = time(NULL);
			regrec.usernum = usernum;
			strcpy(regrec.userid, u->userid);
			regrec.Sname = count_same_reg(regrec.rname, '1', NA);
			regrec.Slog = count_same_reg(u->lasthost, '3', NA);
			regrec.Sip = count_same_reg(u->ident, '2', NA);
			regrec.mark = ' ';
			if(append_record("new_register.rec", &regrec, sizeof (regrec)) == -1)
				outs("系统错误，请联系系统维护员\n");
			else
				outs("您的资料已经提交至帐号管理员处，请耐心等候管理员的批复。\n在此期间，您将不能自行激活帐号\n");
		}else{
			outs("您提交的资料和数据库中的不符，请回到个人工具箱重新填写\n");
		}
		pressreturn();
		return NA;
	}

auth_fail:
	clear();
	presskeyfor("\n\n很抱歉, 您提供的资料不正确, 不能完成该操作.");
	return NA;


}
#endif


// deardragon 2000.09.26 over

char *
getuserlevelstr(unsigned int level)
{
	if (level & PERM_ACCOUNTS)
		return "[\033[1;33m帐号管理员\033[m]";
	if (level & PERM_OBOARDS)
		return "[\033[1;33m讨论区主管\033[m]";
	if (level & PERM_ACBOARD)
		return "[\033[1;33m美工\033[m]";
	if (level & PERM_ACHATROOM)
		return "[\033[1;33m聊天室管理员\033[m]";
	if (level & PERM_JUDGE)
		return "[\033[1;33m仲裁委员会\033[m]";
/*
	if (level & PERM_INTERNSHIP)
		return "[\033[1;33m实习站务\033[m]";
*/

	return (level & PERM_SUICIDE) ? "[\033[1;31m自杀中\033[m]" : "\0";
}

void
getusertitlestr(unsigned char title, char *name)
{
	name[0] = '\0';

	if (title & 0x08)
		strcat(name, "常务管理员 ");
	if (title & 0x10)
		strcat(name, "版主管理员 ");
	if (title & 0x20)
		strcat(name, "系统维护员 ");
	if (title & 0x40)
		strcat(name, "帐号管理员 ");
	if (title & 0x80)
		strcat(name, "版务管理员 ");
	if (title & 0x01)
		strcat(name, "系统美工 ");
	if (title & 0x02)
		strcat(name, "最佳版主 ");
	if (title & 0x04)
		strcat(name, "讨论区主管 ");

	if (name[0] != '\0')
		name[strlen(name) - 1] = '\0';
}

/* monster: 确认用户身份 (在自杀, 修改密码保护资料前需要完成的步骤) */
int
confirm_userident(char *operation)
{
	char buf[STRLEN];
	int day, month, year;

	clear();
	set_safe_record();
	prints("在完成“%s”操作前, 系统需要对您的身份进行确认: ", operation);

	getdata(3, 0, "请输入您的密码: ", buf, PASSLEN, NOECHO, YEA);
	if (*buf == '\0' || !checkpasswd2(buf, &currentuser))
		goto auth_fail;

	getdata(5, 0, "请问您叫什么名字? ", buf, NAMELEN, DOECHO, YEA);
	if (*buf == '\0' || strcmp(buf, currentuser.realname))
		goto auth_fail;

	getdata(7, 0, "请问您的生日 (按\033[1;32m月/日/年\033[m的格式输入, 如\033[1;32m11/7/1981\033[m): ",
		buf, 12, DOECHO, YEA);
	if (*buf == '\0' || sscanf(buf, "%d/%d/%d", &month, &day, &year) != 3)
		goto auth_fail;
	if (currentuser.birthyear + 1900 != year || currentuser.birthmonth != month || currentuser.birthday != day)
		goto auth_fail;

	clear();
	return YEA;

auth_fail:
	presskeyfor("\n\n很抱歉, 您提供的资料不正确, 不能完成该操作.");
	clear();
	return NA;
}
