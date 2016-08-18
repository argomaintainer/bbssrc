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

#define refreshtime     (30)
int (*func_list_show) ();
time_t update_time = 0;
int freshmode = 0;
int toggle1 = 0, toggle2 = 0;
int mailmode, numf;
int friendmode = NA;
int usercounter, real_user_names = 0;
int range, page, readplan, num;
struct user_info *user_record[MAXACTIVE];
struct userec *userdata;

int choose(int update, int defaultn, void (*title_show)(void), int (*key_deal)(int, int, int), int (*list_show)(void), int (*read)(int, int));

// add by Flier - 2000.5.12
enum sort_type { stUserID, stUserName, stIP, stState } st = stUserID;

#define SHOW_IP(uentp) \
	((uentp->hideip == 'N') || \
	(hisfriend(uentp) && (uentp->hideip == 'F')) || \
	 HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEEIP)) \

int
friend_search(unsigned short uid, struct user_info *uentp, int tblsize)
{
	int hi, low, mid;

	if (uid == 0)
		return NA;

	hi = tblsize - 1;
	low = 0;
	while (low <= hi) {
		mid = (low + hi) / 2;

		if (uentp->friends[mid] == uid) {
			return YEA;
		} else if (uentp->friends[mid] > uid) {
			hi = mid - 1;
		} else {
			low = mid + 1;
		}
	}
	return NA;
}

int
myfriend(unsigned short uid)
{
	return friend_search(uid, &uinfo, uinfo.fnum);
}

int
hisfriend(struct user_info *uentp)
{
	return friend_search(uinfo.uid, uentp, uentp->fnum);
}

int
isreject(struct user_info *uentp)
{
	int i;

	if (HAS_PERM(PERM_SYSOP))
		return NA;
	if (uentp->uid != uinfo.uid) {
		for (i = 0; i < MAXREJECTS && (uentp->reject[i] || uinfo.reject[i]); i++) {
			if (uentp->reject[i] == uinfo.uid ||
			    uentp->uid == uinfo.reject[i])
				return YEA;     /* 被设为黑名单 */
		}
	}
	return NA;
}

void
print_title()
{
	if (friendmode)
		modify_user_mode(FRIEND);
	else
		modify_user_mode(LUSERS);

	docmdtitle((friendmode) ? "[好朋友列表]" : "[使用者列表]",
		   " 聊天[\033[1;32mt\033[m] 寄信[\033[1;32mm\033[m] 送讯息[\033[1;32ms\033[m] 加,减朋友[\033[1;32mo\033[m,\033[1;32md\033[m] 看说明档[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 切换模式 [\033[1;32mf\033[m] 求救[\033[1;32mh\033[m]");
	update_endline();
}

void
print_title2()
{
	if (friendmode)
		modify_user_mode(FRIEND);
	else
		modify_user_mode(LUSERS);

	docmdtitle((friendmode) ? "[好朋友列表]" : "[使用者列表]",
		   " 寄信[\033[1;32mm\033[m] 加,减朋友[\033[1;32mo\033[m,\033[1;32md\033[m] 看说明档[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 求救[\033[1;32mh\033[m]");
	update_endline();
}

void
update_data()
{
	if (readplan == YEA)
		return;
	if (time(NULL) >= update_time + refreshtime - 1) {
		freshmode = 1;
	}
	signal(SIGALRM, update_data);
	alarm(refreshtime);
	return;
}

int
print_user_info_title(void)
{
	move(2, 0);
	clrtoeol();
	prints("\033[1;44m编号 %s使用者代号%s %s%s%s%8.8s %s上站的位置%s      P M %c%s目前动态%s  %5s\033[m\n",
		(st == stUserID) ? "\033[32m{" : " ",
		(st == stUserID) ? "}\033[37m" : " ",
		(st == stUserName) ? "\033[32m{" : " ", (real_user_names) ? "真实姓名  " : "使用者昵称",
		(st == stUserName) ? "}\033[37m" : " ", " ",
		(st == stIP) ? "\033[32m{" : " ", (st == stIP) ? "}\033[37m" : " ",
		((HAS_PERM(PERM_SYSOP | PERM_SEECLOAK | PERM_CLOAK)) ? 'C' :
		 ' '), (st == stState) ? "\033[32m{" : " ",
		(st == stState) ? "}\033[37m" : " ",
#ifdef SHOW_IDLE_TIME
		"时:分");
#else
		"");
#endif
	return 0;
}

// Add by Flier - 2000.5.12
int
compare_user_record(const void *leftptr, const void *rightptr)
{
	struct user_info *left = (struct user_info *)(*(struct user_info **)leftptr);
	struct user_info *right = (struct user_info *)(*(struct user_info **)rightptr);

	switch (st) {
	case stUserID:
		return strcasecmp(left->userid, right->userid);
	case stUserName:
		return strcasecmp(left->username, right->username);
	case stIP:
		if (!SHOW_IP(left))
			return 1;
		if (!SHOW_IP(right))
			return -1;
		return strncmp(left->from, right->from, 20);
	case stState:
		return ((left->mode & ~WWW) - (right->mode & ~WWW));
	}

	// unreachable
	return 0;
}

int
fill_userlist(void)
{
	int i, n, totalusernum;
	int friendno[MAXACTIVE];

	resolve_utmp();
	totalusernum = 0;
	numf = 0;
	for (i = 0; i < USHM_SIZE; i++) {
		if (!utmpshm->uinfo[i].active
		    || !utmpshm->uinfo[i].pid || isreject(&utmpshm->uinfo[i]))
			continue;
		if ((utmpshm->uinfo[i].invisible)
		    && (usernum != utmpshm->uinfo[i].uid)
		    && (!HAS_PERM(PERM_SYSOP | PERM_SEECLOAK)))
			continue;
		if (myfriend(utmpshm->uinfo[i].uid)) {
			friendno[numf++] = totalusernum;
		} else if (friendmode)
			continue;
		user_record[totalusernum++] = &utmpshm->uinfo[i];
	}
	if (!friendmode) {
		for (i = 0, n = 0; i < totalusernum && n < numf; i++) {
			if (friendno[n] == i) {
				if (i != n) {
					struct user_info *tmp;

					tmp = user_record[n];
					user_record[n] = user_record[i];
					user_record[i] = tmp;
				}
				n++;
			}
		}
		if (numf > 1)
			qsort(&(user_record[0]), numf, sizeof(struct user_info *), compare_user_record);
		if (totalusernum - numf > 1)
			qsort(&(user_record[numf]), totalusernum - numf, sizeof(struct user_info *), compare_user_record);
	} else {
		if (totalusernum > 1)
			qsort(&(user_record[0]), totalusernum, sizeof(struct user_info *), compare_user_record);
	}
	range = totalusernum;
	return totalusernum == 0 ? -1 : 1;
}

int
show_userlist()
{
	if (update_time + refreshtime < time(NULL)) {
		fill_userlist();
		update_time = time(NULL);
	}
	if (range == 0 /* ||fill_userlist() == 0 */ ) {
		move(2, 0);
		if (friendmode) {
			outs("没有好友在列表中...\n");
			clrtobot();
			move(BBS_PAGESIZE + 3, 0);
			if (askyn("是否转换成使用者模式", YEA, NA) == YEA) {
				range = num_visible_users();
				page = -1;
				friendmode = NA;
				do_userlist();
				clrtobot();
				return 1;
			}
		} else {
			outs("没有使用者在列表中...\n");
			clrtobot();
			pressanykey();
		}
		return -1;
	}
	do_userlist();
	clrtobot();
	return 1;
}

int
deal_key(int ch, int allnum, int pagenum)       //环顾四方处理按键
{
	char buf[STRLEN], desc[5];
	int kickid;
	static int msgflag;
	extern int friendflag;

	if (msgflag == YEA) {
		show_message(NULL);
		msgflag = NA;
	}
	switch (ch) {
	case 'Y':
		if (HAS_PERM(PERM_CLOAK))
			x_cloak();
		break;
	case 'P':
		t_pager();
		break;
	case 'C':
	case 'c':
		if (guestuser)
			break;

		buf[0] = '\0';
		getdata(BBS_PAGESIZE + 3, 0, (ch == 'C') ? "变换昵称(不是临时变换)为: " :
			"暂时变换昵称: ", buf, NICKNAMELEN, DOECHO, NA);
		if (buf[0] != '\0') {
			strlcpy(uinfo.username, buf, sizeof(uinfo.username));
			if (ch == 'C') {
				set_safe_record();
				strlcpy(currentuser.username, buf, sizeof(currentuser.username));
				substitute_record(PASSFILE, &currentuser, sizeof(currentuser), usernum);
			}
		}
		update_ulist(&uinfo, utmpent);
		update_time = 0;
		break;
	case 'k':
	case 'K':
		if (!HAS_PERM(PERM_SYSOP) &&
		    (usernum != user_record[allnum]->uid))
			return 1;
		kickid = user_record[allnum]->uid;      //add by cancel
		if (guestuser)
			return 1;

		if (user_record[allnum]->pid == uinfo.pid) {
			strlcpy(buf, "你自己要把【自己】踢出去吗", sizeof(buf));
		} else {
			snprintf(buf, sizeof(buf), "你要把 %s 踢出站外吗", user_record[allnum]->userid);
		}

		move(BBS_PAGESIZE + 2, 0);
		if (askyn(buf, NA, NA) == NA)
			break;
		if (kickid != user_record[allnum]->uid)
			return 1;       //rewrite by cancel
		if (kick_user(user_record[allnum]) == 1) {
			snprintf(buf, sizeof(buf), "%s 已被踢出站外", user_record[allnum]->userid);
		} else {
			snprintf(buf, sizeof(buf), "%s 无法踢出站外", user_record[allnum]->userid);
		}
		user_record[allnum]->active = NA;
		user_record[allnum]->deactive_time = time(NULL);
		update_time = 0;
		msgflag = YEA;
		break;
	case 'h':
	case 'H':
		show_help("help/userlisthelp");
		break;
	case 't':
	case 'T':
		if (!HAS_PERM(PERM_PAGE) || user_record[allnum]->uid == usernum)
			return 1;
		talk(user_record[allnum]);
		break;
	case 'v':
	case 'V':
		if (!HAS_PERM(PERM_SYSOP))
			return 1;
		real_user_names = !real_user_names;
		break;
	case 'm':
	case 'M':
		if (!HAS_PERM(PERM_SENDMAIL))
			return 1;
		m_send(user_record[allnum]->userid);
		break;
	case 'f':
	case 'F':
		friendmode = !friendmode;
		update_time = 0;
		break;
	case 'l':
/*
 *              monster: 'l' for mailbox, temporarily disabled
 *
 *              savemode = uinfo.mode;
 *              m_read();
 *              modify_user_mode(savemode);
 *              break;
 */
	case 'L':
		show_allmsgs();
		page = -1;
		break;
		/* Add end */
	case 'x':
	case 'X':
		show_personal_announce(user_record[allnum]->userid);
		break;
	case 's':
	case 'S':
		if (guestuser)
			return 0;
		if (!HAS_PERM(PERM_MESSAGE))
			return 1;
		if (!canmsg(user_record[allnum])) {
			snprintf(buf, sizeof(buf), "%s 已经关闭讯息呼叫器", user_record[allnum]->userid);
			msgflag = YEA;
			break;
		}
		do_sendmsg(user_record[allnum], NULL, 0, user_record[allnum]->pid);
		break;
	case 'o':
	case 'O':
	case 'R':
		if (guestuser)
			return 0;
		if (ch == 'o' || ch == 'O') {
			friendflag = YEA;
			strcpy(desc, "好友");
		} else {
			friendflag = NA;
			strcpy(desc, "坏人");
		}
		snprintf(buf, sizeof(buf), "确定要把 %s 加入%s名单吗", user_record[allnum]->userid, desc);
		move(BBS_PAGESIZE + 2, 0);
		if (askyn(buf, NA, NA) == NA)
			break;
		if (addtooverride(user_record[allnum]->userid) == -1) {
			snprintf(buf, sizeof(buf), "%s 已在%s名单", user_record[allnum]->userid, desc);
		} else {
			snprintf(buf, sizeof(buf), "%s 列入%s名单", user_record[allnum]->userid, desc);
		}
		msgflag = YEA;
		update_time = 0;
		break;
	case 'd':
	case 'D':
		if (guestuser)
			return 0;
		snprintf(buf, sizeof(buf), "确定要把 %s 从好友名单删除吗", user_record[allnum]->userid);
		move(BBS_PAGESIZE + 2, 0);
		if (askyn(buf, NA, NA) == NA)
			break;
		if (deleteoverride(user_record[allnum]->userid, "friends") == -1) {
			snprintf(buf, sizeof(buf), "%s 本来就不在好友名单中", user_record[allnum]->userid);
		} else {
			snprintf(buf, sizeof(buf), "%s 已从好友名单移除", user_record[allnum]->userid);
		}
		msgflag = YEA;
		update_time = 0;
		break;
	case '/':       // monster: search online user, inspired by redhatpku
		num = onlinesearch(num, YEA);
		update_endline();
		break;
	case '?':
		num = onlinesearch(num, NA);
		update_endline();
		break;
	case KEY_TAB:/*add by betterman*/
		if (st != stState)
			st++;
		else
			st = stUserID;
		fill_userlist();
		freshmode = 1;
		break;
	default:
		return 0;
	}

	if (friendmode)
		modify_user_mode(FRIEND);
	else
		modify_user_mode(LUSERS);
	if (readplan == NA) {
		print_title();
		clrtobot();
		if (show_userlist() == -1)
			return -1;
		print_title();
		if (msgflag)
			show_message(buf);
		update_endline();
	}
	return 1;
}

int
deal_key2(int ch, int allnum, int pagenum)      //探视网友处理按键
{
	char buf[STRLEN];
	static int msgflag;

	if (msgflag == YEA) {
		show_message(NULL);
		msgflag = NA;
	}
	switch (ch) {
	case 'h':
	case 'H':
		show_help("help/usershelp");
		break;
	case 'm':
	case 'M':
		if (!HAS_PERM(PERM_SENDMAIL))
			return 1;
		m_send(userdata[allnum - pagenum].userid);
		break;
	case 'o':
	case 'O':
		if (guestuser)
			return 0;
		snprintf(buf, sizeof(buf), "确定要把 %s 加入好友名单吗", userdata[allnum - pagenum].userid);
		move(BBS_PAGESIZE + 2, 0);
		if (askyn(buf, NA, NA) == NA)
			break;
		if (addtooverride(userdata[allnum - pagenum].userid) == -1) {
			snprintf(buf, sizeof(buf), "%s 已在朋友名单", userdata[allnum - pagenum].userid);
		} else {
			snprintf(buf, sizeof(buf), "%s 列入朋友名单", userdata[allnum - pagenum].userid);
		}
		show_message(buf);
		msgflag = YEA;
		update_time = 0;
		if (!friendmode)
			return 1;
		break;
	case 'f':
	case 'F':
		toggle1++;
		if (toggle1 >= 3)
			toggle1 = 0;
		break;
	case 't':
	case 'T':
		toggle2++;
		if (toggle2 >= 2)
			toggle2 = 0;
		break;
	case 'd':
	case 'D':
		if (guestuser)
			return 0;
		snprintf(buf, sizeof(buf), "确定要把 %s 从好友名单删除吗", userdata[allnum - pagenum].userid);
		move(BBS_PAGESIZE + 2, 0);
		if (askyn(buf, NA, NA) == NA)
			break;
		if (deleteoverride(userdata[allnum - pagenum].userid, "friends") == -1) {
			snprintf(buf, sizeof(buf), "%s 本来就不在好友名单中", userdata[allnum - pagenum].userid);
		} else {
			snprintf(buf, sizeof(buf), "%s 已从好友名单移除", userdata[allnum - pagenum].userid);
		}
		show_message(buf);
		msgflag = YEA;
		update_time = 0;
		if (!friendmode)
			return 1;
		break;
	default:
		return 0;
	}
	modify_user_mode(LAUSERS);
	if (readplan == NA) {
		print_title2();
		move(3, 0);
		clrtobot();
		if (show_users() == -1)
			return -1;
		update_endline();
	}
	redoscr();
	return 1;
}

int
uleveltochar(char *buf, unsigned int lvl)
{
	if (!(lvl & PERM_BASIC)) {
		strcpy(buf, "--------- ");
		return 0;
	}
	if (lvl < PERM_DEFAULT) {
		strcpy(buf, "- ------- ");
		return 1;
	}
	buf[10] = '\0';
	buf[9] = (lvl & (PERM_BOARDS)) ? 'B' : ' ';
	buf[8] = (lvl & (PERM_CLOAK)) ? 'C' : ' ';
	buf[7] = (lvl & (PERM_SEECLOAK)) ? '#' : ' ';
	buf[6] = (lvl & (PERM_XEMPT)) ? 'X' : ' ';
	buf[5] = (lvl & (PERM_CHATCLOAK)) ? 'M' : ' ';
	buf[4] = (lvl & (PERM_ACCOUNTS)) ? 'A' : ' ';
	buf[3] = (lvl & (PERM_ANNOUNCE)) ? 'N' : ' ';
	buf[2] = (lvl & (PERM_OBOARDS)) ? 'O' : ' ';
	buf[1] = (lvl & (PERM_DENYPOST)) ? 'D' : ' ';
	buf[0] = (lvl & (PERM_SYSOP)) ? 'S' : ' ';
	return 1;
}

void
printutitle(void)
{
	move(2, 0);
	prints
	    ("\033[1;44m 编 号  使用者代号   %-19s #%-4s #%-4s %8s    %-12s  \033[m\n",
#if defined(ACTS_REALNAMES)
	     HAS_PERM(PERM_SYSOP) ? "真实姓名" : "使用者昵称",
#else
	     "使用者昵称",
#endif
	     (toggle2 == 0) ? "上站" : "文章",
	     (toggle2 == 0) ? "时数" : "信件",
	     HAS_PERM(PERM_SEEULEVELS) ? " 等  级 " : "",
	     (toggle1 == 0) ? "最近光临日期" :
	     (toggle1 == 1) ? "最近光临地点" : "帐号建立日期");
}

/* count登录次数>0 && 有 PERM_BASIC的用户 */
int
countusers(void *uentp_ptr, int unused)
{
	static int totalusers;
	char permstr[11];
	struct userec *uentp = (struct userec *)uentp_ptr;

	if (uentp == NULL) {
		int c = totalusers;

		totalusers = 0;
		return c;
	}
	if (uentp->numlogins != 0 && uleveltochar(permstr, uentp->userlevel) != 0)
		totalusers++;
	return 0;
}

int
printuent(void *uentp_ptr, int unused)
{
	static int i;
	char permstr[11];
	char msgstr[18];
	int override;
	struct userec *uentp = (struct userec *)uentp_ptr;

	if (uentp == NULL) {
		printutitle();
		i = 0;
		return 0;
	}
	if (uentp->numlogins == 0 || uleveltochar(permstr, uentp->userlevel) == 0)
		return 0;
	if (i < page || i >= page + BBS_PAGESIZE || i >= range) {
		i++;
		if (i >= page + BBS_PAGESIZE || i >= range)
			return QUIT;
		else
			return 0;
	}
	uleveltochar(permstr, uentp->userlevel);
	switch (toggle1) {
	case 0:
		getdatestring(uentp->lastlogin);
		snprintf(msgstr, sizeof(msgstr), "%-.16s", datestring + 6);
		break;
	case 1:
		snprintf(msgstr, sizeof(msgstr), "%-.16s", uentp->lasthost);
		break;
	case 2:
	default:
		getdatestring(uentp->firstlogin);
		snprintf(msgstr, sizeof(msgstr), "%-.14s", datestring);
		break;
	}
	userdata[i - page] = *uentp;
	override = myfriend(searchuser(uentp->userid));

        prints(" %5d%2s%s%-12s%s %-17s %6d %4d %10s %-16s\n", i + 1,
               (override) ? "√" : "",
               (override) ? "\033[1;32m" : "", uentp->userid,
               (override) ? "\033[m" : "",

#if defined(ACTS_REALNAMES)
               HAS_PERM(PERM_SYSOP) ? uentp->realname : uentp->username,
#else
               uentp->username,
#endif
               (toggle2 == 0) ? (uentp->numlogins) : (uentp->numposts),
               (toggle2 == 0) ? uentp->stay / 3600 : uentp->nummails,
               HAS_PERM(PERM_SEEULEVELS) ? permstr : "", msgstr);

	i++;
	usercounter++;
	return 0;
}

int
allusers(void)
{
	countusers(NULL, 0);
	if (apply_record(PASSFILE, countusers, sizeof (struct userec)) == -1) {
		return 0;
	}
	return countusers(NULL, 0);
}

int
show_users(void)
{
	usercounter = 0;
	modify_user_mode(LAUSERS);
	printuent(NULL, 0);
	if (apply_record(PASSFILE, printuent, sizeof (struct userec)) == -1) {
		outs("错误：指定的用户不存在");
		pressreturn();
	} else {
		clrtobot();
	}
	return 0;
}

int
do_query(int star, int curr)
{
	if (user_record[curr] != NULL && user_record[curr]->userid != NULL) {
		clear();
		t_query(user_record[curr]->userid);
		move(t_lines - 1, 0);
		outs("\033[0;1;37;44m聊天[\033[1;32mt\033[37m] 寄信[\033[1;32mm\033[37m] 送讯息[\033[1;32ms\033[37m] 加,减朋友[\033[1;32mo\033[37m,\033[1;32md\033[37m] 选择使用者[\033[1;32m↑\033[37m,\033[1;32m↓\033[37m] 切换模式 [\033[1;32mf\033[37m] 求救[\033[1;32mh\033[37m]\033[m");
		return 0;
	}
	return -1;
}

int
do_query2(int star, int curr)
{
	/*betterman: 使用者列表里使用的是userdata中的数据.*/
	/*if (user_record[curr - star] != NULL && user_record[curr - star]->userid != NULL) {*/
	if (userdata[curr - star].userid != NULL) {
		t_query(userdata[curr - star].userid);
		move(t_lines - 1, 0);
		outs("\033[0;1;37;44m          寄信[\033[1;32mm\033[37m] 加,减朋友[\033[1;32mo\033[37m,\033[1;32md\033[37m] 看说明档[\033[1;32m→\033[37m,\033[1;32mRtn\033[37m] 选择[\033[1;32m↑\033[37m,\033[1;32m↓\033[37m] 求救[\033[1;32mh\033[37m]          \033[m");
		return 0;
	}
	return -1;
}

int
onlinesearch_id(char *query, int currnum, int forward)
{
	int i;

	if (query[0] == 0)
		return currnum;

	if (forward == YEA) {
		for (i = currnum + 1; i < range; i++)
			if (!strcasecmp(query, user_record[i]->userid))
				return i;
	} else {
		for (i = currnum - 1; i > 0; i--)
			if (!strcasecmp(query, user_record[i]->userid))
				return i;
	}

	return currnum;
}

int
onlinesearch_nickname(char *query, int currnum, int forward)
{
	int i;

	if (query[0] == 0)
		return currnum;

	if (forward == YEA) {
		for (i = currnum + 1; i < range; i++)
			if (!strcasecmp(query, user_record[i]->username))
				return i;
	} else {
		for (i = currnum - 1; i > 0; i--)
			if (!strcasecmp(query, user_record[i]->username))
				return i;
	}

	return currnum;
}

int
onlinesearch_ip(char *query, int currnum, int forward)
{
	int i;

	if (query[0] == 0)
		return currnum;

	if (forward == YEA) {
		for (i = currnum + 1; i < range; i++)
			if (!strcasecmp(query, user_record[i]->from))
				return i;
	} else {
		for (i = currnum - 1; i > 0; i--)
			if (!strcasecmp(query, user_record[i]->from))
				return i;
	}

	return currnum;
}

int
onlinesearch(int currnum, int forward)
{
	char buf[STRLEN];
	char ans[2];
	static int lastans;

	if (HAS_PERM(PERM_SYSOP | PERM_SEEIP)) {
		snprintf(buf, sizeof(buf), "向%s搜寻: 1) 使用者代号  2) 使用者昵称  3) 上站位置  0) 取消 [%d]: ",
			(forward == NA) ? "前" : "后", lastans);
	} else {
		snprintf(buf, sizeof(buf), "向%s搜寻: 1) 使用者代号  2) 使用者昵称  0) 取消 [%d]: ",
			(forward == NA) ? "前" : "后", lastans);
	}
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, buf, ans, 2, DOECHO, YEA);

	if (ans[0] == '3' && !HAS_PERM(PERM_SYSOP | PERM_SEEIP))
		return currnum;

	if (ans[0] != 0) {
		if (ans[0] <= '0' || ans[0] > '3')
			return currnum;
		lastans = ans[0] - '0';
	}

	clear_line(t_lines - 1);
	switch (lastans) {
		case 1:
			snprintf(buf, sizeof(buf), "向%s搜寻使用者代号: ", (forward == NA) ? "前" : "后");
			getdata(t_lines - 1, 0, buf, buf, IDLEN + 1, DOECHO, YEA);
			currnum = onlinesearch_id(buf, currnum, forward);
			break;
		case 2:
			snprintf(buf, sizeof(buf), "向%s搜寻使用者昵称: ", (forward == NA) ? "前" : "后");
			getdata(t_lines - 1, 0, buf, buf, NAMELEN + 1, DOECHO, YEA);
			currnum = onlinesearch_nickname(buf, currnum, forward);
			break;
		case 3:
			snprintf(buf, sizeof(buf), "向%s搜寻上站位置: ", (forward == NA) ? "前" : "后");
			getdata(t_lines - 1, 0, buf, buf, 17, DOECHO, YEA);
			currnum = onlinesearch_ip(buf, currnum, forward);
			break;
	}

	return currnum;
}

int
Users(void)
{
	range = allusers();
	modify_user_mode(LAUSERS);
	clear();
	userdata = (struct userec *) calloc(sizeof (struct userec), BBS_PAGESIZE);
	choose(NA, 0, print_title2, deal_key2, show_users, do_query2);
	clear();
	free(userdata);
	return 0;
}

int
t_friends(void)
{
	char buf[STRLEN];

	modify_user_mode(FRIEND);
	real_user_names = 0;
	friendmode = YEA;
	setuserfile(buf, "friends");
	if (!dashf(buf)) {
		move(1, 0);
		clrtobot();
		outs("你尚未利用 Info -> Override 设定好友名单，所以...\n");
		range = 0;
	} else {
		num_alcounter();
		range = count_friends;
	}
	if (range == 0) {
		move(2, 0);
		clrtobot();
		outs("目前无好友上线\n");
		move(BBS_PAGESIZE + 3, 0);
		if (askyn("是否转换成使用者模式", YEA, NA) == YEA) {
			range = num_visible_users();
			freshmode = 1;
			page = -1;
			friendmode = NA;
			update_time = 0;
			choose(YEA, 0, print_title, deal_key, show_userlist, do_query);
			return MODECHANGED;
		}
	} else {
		update_time = 0;
		choose(YEA, 0, print_title, deal_key, show_userlist, do_query);
	}
	friendmode = NA;
	return MODECHANGED;
}

int
t_users(void)
{
	friendmode = NA;
	modify_user_mode(LUSERS);
	real_user_names = 0;
	range = num_visible_users();
	if (range == 0) {
		move(3, 0);
		clrtobot();
		outs("目前无使用者上线\n");
	}
	update_time = 0;
	choose(YEA, 0, print_title, deal_key, show_userlist, do_query);
	return MODECHANGED;
}

int
choose(int update, int defaultn, void (*title_show)(void), int (*key_deal)(int, int, int), int (*list_show)(void), int (*read)(int, int))
{
	int ch, number, deal;

	readplan = NA;
	(*title_show) ();
	func_list_show = list_show;
	signal(SIGALRM, SIG_IGN);
	if (update == 1)
		update_data();
	page = -1;
	number = 0;
	num = defaultn;
	while (1) {
		if (num <= 0)
			num = 0;
		if (num >= range)
			num = range - 1;
		if (page < 0 || freshmode == 1) {
			freshmode = 0;
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			move(3, 0);
			clrtobot();
			if ((*list_show) () == -1)
				return -1;
			update_endline();
		}
		if (num < page || num >= page + BBS_PAGESIZE) {
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			if ((*list_show) () == -1)
				return -1;
			update_endline();
			continue;
		}
		if (readplan == YEA) {
			if ((*read) (page, num) == -1)
				return num;
		} else {
			move(3 + num - page, 0);
			prints(">", number);
		}
		ch = egetch();
		if (readplan == NA)
			move(3 + num - page, 0);
		outc(' ');
		if (ch == 'q' || ch == 'e' || ch == KEY_LEFT || ch == EOF) {
			if (readplan == YEA) {
				readplan = NA;
				move(1, 0);
				clrtobot();
				if ((*list_show) () == -1)
					return -1;
				(*title_show) ();
				continue;
			}
			break;
		}
		deal = (*key_deal) (ch, num, page);
		if (range == 0)
			break;
		if (deal == 1)
			continue;
		else if (deal == -1)
			break;
		switch (ch) {
		case 'b':
		case Ctrl('B'):
		case KEY_PGUP:
			if (num == 0)
				num = range - 1;
			else
				num -= BBS_PAGESIZE;
			break;
		case ' ':
			if (readplan == YEA) {
				if (++num >= range)
					num = 0;
				break;
			}
		case 'N':
		case Ctrl('F'):
		case KEY_PGDN:
			if (num == range - 1)
				num = 0;
			else
				num += BBS_PAGESIZE;
			break;
		case 'p':
		case 'l':
		case KEY_UP:
			if (num-- <= 0)
				num = range - 1;
			break;
		case 'n':
		case 'j':
		case KEY_DOWN:
			if (++num >= range)
				num = 0;
			break;
/*KEY_TAB键的处理移至 deal_key(int ch, int allnum, int pagenum) 

		case KEY_TAB:
			if (st != stState)
				st++;
			else
				st = stUserID;
			fill_userlist();
			freshmode = 1;
			break;
*/
		case '$':
		case KEY_END:
			num = range - 1;
			break;
		case KEY_HOME:
			num = 0;
			break;
		case '\n':
		case '\r':
			if (number > 0) {
				num = number - 1;
				break;
			}
			/* fall through */
		case KEY_RIGHT:
			{
				if (readplan == YEA) {
					if (++num >= range)
						num = 0;
				} else
					readplan = YEA;
				break;
			}
		case Ctrl('V'):
			x_lockscreen_silent();
			break;
		default:
			;
		}
		if (ch >= '0' && ch <= '9') {
			number = number * 10 + (ch - '0');
			ch = '\0';
		} else {
			number = 0;
		}
	}
	signal(SIGALRM, SIG_IGN);
	return -1;
}


#ifdef SHOW_IDLE_TIME

inline const char *
idle_str(struct user_info *uentp)
{
	static char hh_mm_ss[32];
	time_t now, diff;
	int limit, hh, mm;

	if (uentp->mode & WWW)
		return "     ";

	now = time(NULL);

	if (uentp->mode == TALK) {
		diff = talkidletime;    /* 聊天有另一套 idle kick 机制 */
	} else if (uentp->mode == BBSNET || INBBSGAME(uentp->mode)) {
		diff = 0;
	} else {
		diff = now - uentp->idle_time;
	}

#ifdef DOTIMEOUT
	/*
	 * the 60 * 60 * 24 * 5 is to prevent fault /dev mount from kicking
	 * out all users
	 */

	if (uentp->ext_idle) {
		limit = IDLE_TIMEOUT * 3;
	} else {
		limit = IDLE_TIMEOUT;
	}

	if ((diff > limit) && (diff < 86400 * 5) && uentp->pid)
		safe_kill(uentp->pid);
#endif

	hh = diff / 3600;
	mm = (diff / 60) % 60;

	if (hh > 0) {
		snprintf(hh_mm_ss, sizeof(hh_mm_ss), "%02d:%02d", hh, mm);
	} else if (mm > 0) {
		snprintf(hh_mm_ss, sizeof(hh_mm_ss), "%d", mm);
	} else {
		return "   ";
	}

	return hh_mm_ss;
}

#else

#define idle_str(uentp) ""

#endif

static const char *
color_str(struct user_info *uentp)
{
	if (uentp->invisible == YEA)
		return "\033[36m";

	if (uentp->mode & WWW)
		return "\033[1;35m";

	if (uentp->mode == POSTING)
		return "\033[1;32m";

	if (uentp->mode == BBSNET || INBBSGAME(uentp->mode))
		return "\033[1;33m";

	return "";
}

int
do_userlist(void)
{
	int i, j, override;
	struct user_info *uentp;

	TRY
		print_user_info_title();
		for (i = 0, j = 0; j < BBS_PAGESIZE && i + page < range; i++) {
			uentp = user_record[i + page];
			override = (i + page < numf) || friendmode;

			if (readplan == YEA)
				return 0;

			if (uentp == NULL || !uentp->active || !uentp->pid)
				continue;       /* 某人正巧离开, by sunner */

			/* by wujian 增加是否为对方好友的显示 */
			/* monster: 增加昵称彩色与改变IP隐藏方式 */
			clrtoeol();
			prints(" \033[m%3d%s%-12.22s\033[37m \033[1;%dm%-20.20s\033[m %-16.16s %c %c %c %s%-10.10s\033[37m %5.5s\033[m\n",
				i + 1 + page,
#ifdef SHOWMETOFRIEND
				(override) ? ((hisfriend(uentp)) ? "  \033[1;32m" :
					      "□\033[1;32m") : ((hisfriend(uentp)) ? "  \033[1;33m" : "  "),
#else
				(override) ? "□\033[1;32m" : "  ",
#endif
				uentp->userid,
#ifdef NICKCOLOR
				uentp->nickcolor,
#else
				0,
#endif
				(real_user_names) ? uentp->realname : uentp->
				username,
				SHOW_IP(uentp) ? uentp->from : "     *     ",
				(uentp->mode == BBSNET ||
				 INBBSGAME(uentp->
					   mode)) ? '@' :
				pagerchar(hisfriend(uentp), uentp->pager),
				msgchar(uentp),
				(uentp->invisible == YEA) ? '@' : ' ',
				color_str(uentp),
				modetype(uentp->mode),
				idle_str(uentp));
			++j;
		}
	END
	return 0;
}

