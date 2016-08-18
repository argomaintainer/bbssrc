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

#define M_INT 8			/* monitor mode update interval */
#define P_INT 20		/* interval to check for page req. in
				 * talk/chat */
extern int numf, friendmode;
int talkrequest = NA;
int talkidletime = 0;
int ulistpage;
int friendflag = 1;

/* Added End. */
#ifdef TALK_LOG
void do_log(char *msg, int who);
int talkrec = -1;
char partner[IDLEN + 1];
#endif

struct one_key friend_list[] = {
	{ 'r', 	friend_query},
	{ 'm', 	friend_mail },
	{ 'a', 	friend_add  },
	{ 'A', 	friend_add  },
	{ 'd', 	friend_dele },
	{ 'D', 	friend_dele },
	{ 'E', 	friend_edit },
	{ 'h', 	friend_help },
	{ 'H', 	friend_help },
	{ '\0', NULL 	    }
};

struct one_key reject_list[] = {
	{ 'r', 	reject_query },
	{ 'a', 	reject_add   },
	{ 'A', 	reject_add   },
	{ 'd', 	reject_dele  },
	{ 'D', 	reject_dele  },
	{ 'E', 	reject_edit  },
	{ 'h', 	reject_help  },
	{ 'H', 	reject_help  },
	{ '\0', NULL         }
};

struct one_key maildeny_list[] = {
	{ 'r', 	maildeny_query},
	{ 'a', 	maildeny_add  },
	{ 'A', 	maildeny_add  },
	{ 'd', 	maildeny_dele },
	{ 'D', 	maildeny_dele },
	{ 'E', 	maildeny_edit },
	{ 'h', 	maildeny_help },
	{ 'H', 	maildeny_help },
	{ '\0', NULL          }
};

struct talk_win {
	int curcol, curln;
	int sline, eline;
};

int nowmovie;
void moveto(int mode, struct talk_win *twin);

static char *refuse[] = {
	"抱歉，我现在想专心看 Board。    ", "请不要吵我，好吗？..... :)      ",
	"我现在有事，等一下再 Call 你。  ", "我马上要离开了，下次再聊吧。    ",
	"请你不要再 Page，我不想跟你聊。 ", "请先写一封自我介绍给我，好吗？  ",
	"对不起，我现在在等人。          ", "我今天很累，不想跟别人聊天。    ",
	NULL
};

char save_page_requestor[40];

int
ishidden(char *user)
{
	int tuid;
	struct user_info uin;

	if (!(tuid = getuser(user, NULL)))
		return 0;
	if (!search_ulist(&uin, t_cmpuids, tuid))
		return 0;
	return (uin.invisible);
}

char
pagerchar(int friend, int pager)
{
	if (pager & ALL_PAGER)
		return ' ';
	if ((friend)) {
		if (pager & FRIEND_PAGER)
			return 'O';
		else
			return '#';
	}
	return '*';
}

int
canpage(int friend, int pager)
{
	if ((pager & ALL_PAGER) || HAS_PERM(PERM_SYSOP | PERM_FORCEPAGE))
		return YEA;
	if ((pager & FRIEND_PAGER)) {
		if (friend)
			return YEA;
	}
	return NA;
}

int
listcuent(struct user_info *uentp)
{
	if (uentp == NULL) {
		CreateNameList();
		return 0;
	}
	if (uentp->uid == usernum)
		return 0;
	if (!uentp->active || !uentp->pid || isreject(uentp))
		return 0;
	if (uentp->invisible && !(HAS_PERM(PERM_SYSOP | PERM_SEECLOAK)))
		return 0;
	AddToNameList(uentp->userid);
	return 0;
}

void
creat_list(void)
{
	listcuent(NULL);
	apply_ulist(listcuent);
}

int
t_pager(void)
{

	if (uinfo.pager & ALL_PAGER) {
		uinfo.pager &= ~ALL_PAGER;
		if (DEFINE(DEF_FRIENDCALL))
			uinfo.pager |= FRIEND_PAGER;
		else
			uinfo.pager &= ~FRIEND_PAGER;
	} else {
		uinfo.pager |= ALL_PAGER;
		uinfo.pager |= FRIEND_PAGER;
	}

	if (!uinfo.in_chat && uinfo.mode != TALK) {
		move(1, 0);
		clrtoeol();
		prints("您的呼叫器 (pager) 已经\033[1m%s\033[m了!",
		       (uinfo.pager & ALL_PAGER) ? "打开" : "关闭");
		pressreturn();
	}
	update_utmp();
	return 0;
}

/*Add by SmallPig*/
/*此函数只负责列印说明档，并不管清除或定位的问题。*/

int
show_user_plan(char *userid)
{
	char pfile[STRLEN];

	sethomefile(pfile, userid, "plans");
	if (show_one_file(pfile) == NA) {
		prints("\033[1;36m没有个人说明档\033[m\n");
		return NA;
	}
	return YEA;
}

int
show_one_file(char *filename)
{
	int i, j, ci;
	char pbuf[256];
	FILE *pf;

	if ((pf = fopen(filename, "r")) == NULL) {
		return NA;
	} else {
		/* monster: change limit from MAXQUERYLINES to t_lines - 9 */
		for (i = HAS_PERM(PERM_SYSOP) ? 8 : 7; i < t_lines - 1; i++) {
			move(i, 0);
			if (fgets(pbuf, sizeof (pbuf), pf)) {
				pbuf[strlen(pbuf) - 1] = '\0';
				for (j = 0; pbuf[j]; j++)
					if (pbuf[j] != '\033')
						outc(pbuf[j]);
					else {
						ci = strspn(&pbuf[j], "\033[0123456789;");
						if (pbuf[ci + j] != 'm')
							j += ci;
						else
							outc(pbuf[j]);
					}
			} else {
				break;
			}
		}
		fclose(pf);
		return YEA;
	}
}

/* Modified By Excellent*/
int
t_query(char *q_id)
{
	char uident[IDLEN + 2];
	int tuid = 0, hideip = 0, oldmode;
	int num;		/* Add by SmallPig */
	struct user_info uin;
	char qry_mail_dir[STRLEN];
	char planid[IDLEN + 2], name[STRLEN];
	struct userec lookupuser;
	time_t now;

	if (digestmode == 10 || digestmode == 11)
		return DONOTHING;

	/* monster: 修正Ctrl + A引起的一个小问题 */
	oldmode = uinfo.mode;
	if ((uinfo.mode == READING || uinfo.mode == RMAIL) && q_id[0] != '\0') {
		modify_user_mode(QUERY);
		goto init_query;
	}

	if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS &&
	    uinfo.mode != FRIEND && uinfo.mode != READING &&
	    uinfo.mode != RMAIL && uinfo.mode != GMENU) {
		modify_user_mode(QUERY);
		refresh();
		move(1, 0);
		clrtobot();
		prints("查询谁:\n<输入使用者代号, 按空白键可列出符合字串>\n");
		move(1, 8);
		usercomplete(NULL, uident);
		if (uident[0] == '\0')
			return 0;
	} else {
		// if (*q_id == '\0') return 0;
		if (q_id == NULL || q_id[0] == '\0')
			return 0;	/* monster: a possible fix for high cpu usage */

	      init_query:
		if (strchr(q_id, ' '))
			strtok(q_id, " ");
		strlcpy(uident, q_id, sizeof(uident));
	}

	if (!(tuid = getuser(uident, &lookupuser))) {
		if (oldmode == READING || oldmode == RMAIL) {
			clear();
			move(2, 0);
		} else {
			move(2, 0);
			clrtoeol();
		}
		prints("\033[1m不正确的使用者代号\033[m\n");
		pressanykey();
		return -1;
	}
	uinfo.destuid = tuid;
	update_utmp();
	
	num = t_search_ulist(&uin, t_cmpuids, tuid, NA, NA);
	search_ulist(&uin, t_cmpuids, tuid); /* tuid should equal to getuser(lookupuser.userid, NULL); */

	clear();
	prints("\033[1;37m%s \033[m(\033[1;33m%s\033[m) 共上站 \033[1;32m%d\033[m 次，发表过 \033[1;32m%d\033[m 篇文章\n",
		lookupuser.userid, lookupuser.username, lookupuser.numlogins,
		lookupuser.numposts);

	getdatestring(lookupuser.lastlogin);
	if (!HAS_PERM(PERM_SEEIP) && !HAS_PERM(PERM_SYSOP)) {
		if (num) {
			hideip = !((lookupuser.userdefine & DEF_NOTHIDEIP) ||
				   (hisfriend(&uin) && (lookupuser.userdefine & DEF_FRIENDSHOWIP)));
		} else {
			hideip = !(lookupuser.userdefine & DEF_NOTHIDEIP);
		}
	}
	prints("上 次 在: [\033[1;32m%s\033[m] 从 [\033[1;32m%s\033[m] 到本站一游。\n",
	       datestring, (hideip || lookupuser.lasthost[0] == '\0') ? "(不详)" : lookupuser.lasthost);

	if (num) {
		prints("目前在线：[\033[1;32m讯息器：(\033[36m%s\033[32m) 呼叫器：(\033[36m%s\033[32m)\033[m] ",
			canmsg(&uin) ? "打开" : "关闭", canpage(hisfriend(&uin), uin.pager) ? "打开" : "关闭");
	} else {
		if (lookupuser.lastlogout < lookupuser.lastlogin) {
			now = ((time(NULL) - lookupuser.lastlogin) / 120) % 47 + 1 +
			      lookupuser.lastlogin;
		} else {
			now = lookupuser.lastlogout;
		}
		getdatestring(now);
		prints("离站时间: [\033[1;32m%s\033[m] ", datestring);
	}

	snprintf(qry_mail_dir, sizeof(qry_mail_dir), "mail/%c/%s/.DIR", 
		mytoupper(lookupuser.userid[0]), lookupuser.userid);
	prints("\n信箱：[\033[1;5;32m%2s\033[m] 生命力：[\033[1;32m%d\033[m]",
	       (check_query_mail(qry_mail_dir) == 1) ? "信" : "  ", compute_user_value(&lookupuser));

	if (lookupuser.usertitle == 0) {
		outc('\n');
	} else {
		getusertitlestr(lookupuser.usertitle, name);
		prints("\n称号：[\033[1;33m%s\033[m]\n", name);
	}

	t_search_ulist(&uin, t_cmpuids, tuid, YEA, NA);
#if defined(QUERY_REALNAMES)
	if (HAS_PERM(PERM_SYSOP))
		prints("真实姓名: %s \n", lookupuser.realname);
#endif

	/* 显示说明档 */
	strlcpy(planid, lookupuser.userid, sizeof(planid));
	show_user_plan(planid);

	if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS && uinfo.mode != FRIEND && uinfo.mode != GMENU) {
		move(t_lines - 1, 0);
		outs("\033[m\033[1;44m聊天[\033[32mt\033[37m] 寄信[\033[32mm\033[37m] 送讯息[\033[32ms\033[37m] 加,减朋友[\033[32mo,d\033[37m] 设为坏人[\033[32mR\033[37m] 个人文集[\033[32mx\033[37m]               \033[m");

		if (!guestuser) {
			search_ulist(&uin, t_cmpuids, tuid); 
			switch (egetch()) {
			case 'T':
			case 't':
				if (strcmp(currentuser.userid, lookupuser.userid) && num) {
					clear();
					talk(&uin);
				}
				break;
			case 'M':
			case 'm':
				if (HAS_PERM(PERM_SENDMAIL))
					m_send(lookupuser.userid);
				break;
			case 'S':
			case 's':
				if (HAS_PERM(PERM_MESSAGE) && canmsg(&uin)) {
					clear();
					do_sendmsg(&uin, NULL, 0, uin.pid);
				}
				break;
			case 'O':
			case 'o':
				friendflag = YEA;
				if (addtooverride(lookupuser.userid) == -1) {
					snprintf(genbuf, sizeof(genbuf), "%s 已在好友名单", lookupuser.userid);
				} else {
					snprintf(genbuf, sizeof(genbuf), "%s 列入好友名单", lookupuser.userid);
				}
				presskeyfor(genbuf);
				break;
			case 'R':
				friendflag = NA;
				if (addtooverride(lookupuser.userid) == -1) {
					snprintf(genbuf, sizeof(genbuf), "%s 已在坏人名单", lookupuser.userid);
				} else {
					snprintf(genbuf, sizeof(genbuf), "%s 列入坏人名单", lookupuser.userid);
				}
				presskeyfor(genbuf);
				break;
			case 'D':
			case 'd':
				if (deleteoverride(lookupuser.userid, "friends") == -1) {
					snprintf(genbuf, sizeof(genbuf), "%s 本来就不在好友名单中", lookupuser.userid);
				} else {
					snprintf(genbuf, sizeof(genbuf), "%s 已从好友名单移除", lookupuser.userid);
				}
				presskeyfor(genbuf);
				break;
			case 'X':
			case 'x':
				show_personal_announce(lookupuser.userid);
				break;
			}
		} else {
			pressanykey();
		}	
	}

	uinfo.destuid = 0;
	return 0;
}

int
cmpfnames(void *userid_ptr, void *uv_ptr)
{
	char *userid = (char *)userid_ptr;
	struct override *uv = (struct override *)uv_ptr;

	return !strcmp(userid, uv->id);
}

int
t_cmpuids(int uid, struct user_info *up)
{
	return (up->active && uid == up->uid);
}

int
t_talk(void)
{
	int result;

#ifdef DOTIMEOUT
	init_alarm();
#else
	signal(SIGALRM, SIG_IGN);
#endif
	refresh();
	result = talk(NULL);
	clear();
	return result;
}

int
talk(struct user_info *userinfo)
{
	char uident[STRLEN];
	char reason[STRLEN];
	int tuid, ucount, unum, tmp;

#ifdef FIVEGAME
/*added by djq,99.07.19,for FIVE */
	int five = 0;
#endif
	struct user_info uin;

	move(1, 0);
	clrtobot();
	if (uinfo.invisible) {
		move(2, 0);
		prints("抱歉, 此功能在隐身状态下不能执行...\n");
		pressreturn();
		return 0;
	}
	if (userinfo == NULL || uinfo.mode == GMENU) {
		move(2, 0);
		prints("<输入使用者代号>\n");
		move(1, 0);
		clrtoeol();
		prints("跟谁聊天: ");
		creat_list();
		namecomplete(NULL, uident);

		if (uident[0] == '\0') {
			clear();
			return 0;
		}

		if (!strcmp(uident, "guest") && !HAS_PERM(PERM_FORCEPAGE)) {
			move(2, 0);
			prints("无法呼叫该用户\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}

		if (!(tuid = searchuser(uident)) || tuid == usernum) {
		      wrongid:
			move(2, 0);
			prints("错误代号\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
		ucount = t_search_ulist(&uin, t_cmpuids, tuid, NA, YEA);
		if (ucount > 1) {
		      list:move(3, 0);
			ucount = t_search_ulist(&uin, t_cmpuids, tuid, YEA, YEA);
			clrtobot();
			tmp = ucount + 5;
			getdata(tmp, 0,
				"请选一个你看的比较顺眼的 [0 -- 不聊了]: ",
				genbuf, 4, DOECHO, YEA);
			unum = atoi(genbuf);
			if (unum == 0) {
				clear();
				return 0;
			}
			if (unum > ucount || unum < 0) {
				move(tmp, 0);
				prints("笨笨！你选错了啦！\n");
				clrtobot();
				pressreturn();
				goto list;
			}
			if (!search_ulistn(&uin, t_cmpuids, tuid, unum))
				goto wrongid;
		} else if (!search_ulist(&uin, t_cmpuids, tuid))
			goto wrongid;
	} else {
/*     		memcpy(&uin,userinfo,sizeof(uin));     */
		uin = *userinfo;
		tuid = uin.uid;
		strcpy(uident, uin.userid);
		move(1, 0);
		clrtoeol();
		prints("跟谁聊天: %s", uin.userid);
	}

	/* check if pager on/off       --gtv */
	if (!canpage(hisfriend(&uin), uin.pager)) {
		move(2, 0);
		prints("对方呼叫器已关闭.\n");
		pressreturn();
		move(2, 0);
		clrtoeol();
		return -1;
	}
	if (uin.mode == SYSINFO || uin.mode == BBSNET
	    || uin.mode == DICT
	    || uin.mode == ADMIN
	    || uin.mode == LOCKSCREEN || uin.mode == PAGE || INBBSGAME(uin.mode)
	    || uin.mode == LOGIN) {
		move(2, 0);
		prints("目前无法呼叫.\n");
		clrtobot();
		pressreturn();
		return -1;
	}
	if (!uin.active || (kill(uin.pid, 0) == -1)) {
		move(2, 0);
		prints("对方已离开\n");
		pressreturn();
		move(2, 0);
		clrtoeol();
		return -1;
	} else {
		int sock, msgsock;
		char c;
		struct sockaddr_in server;
		socklen_t namelen;

#ifdef FIVEGAME
		char answer[2] = "";
#endif
		move(3, 0);
		clrtobot();
		show_user_plan(uident);
#ifndef FIVEGAME
		move(2, 0);
		if (askyn("确定要和他/她谈天吗", NA, NA) == NA) {
			clear();
			return 0;
		}
		report("Talk to '%s'", uident);
#else
		getdata(2, 0,
			"想找对方谈天请按\'y\',下『五子棋』请按\'w\'(Y/W/N)[N]:",
			answer, 4, DOECHO, YEA);
		if (*answer != 'y' && *answer != 'w') {
			clear();
			return 0;
		}

		if (*answer == 'w')
			five = 1;

		if (five == 1) {
			report("FIVE to %s", uident);
		} else {
			report("TALK to %s", uident);
		}
#endif
		if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
			return -1;
		server.sin_family = AF_INET;
		server.sin_addr.s_addr = INADDR_ANY;
		server.sin_port = 0;
		if (bind(sock, (struct sockaddr *) &server, sizeof (server)) < 0)
			return -1;
		if (getsockname(sock, (struct sockaddr *) &server, &namelen) < 0)
			return -1;
		uinfo.sockactive = YEA;
		uinfo.sockaddr = server.sin_port;
		uinfo.destuid = tuid;
#ifndef FIVEGAME

		modify_user_mode(PAGE);
#else
		if (five == 1) {
			modify_user_mode(PAGE_FIVE);
		} else {
			modify_user_mode(PAGE);
		}
#endif
#ifdef TALK_LOG
		strcpy(partner, uin.userid);
#endif
		kill(uin.pid, SIGUSR1);
		clear();
		prints("呼叫 %s 中...\n输入 Ctrl-D 结束\n", uident);

		listen(sock, 1);
		add_io(sock, 20);
		while (YEA) {
			int ch;

			ch = igetkey();
			if (ch == I_TIMEOUT) {
				move(0, 0);
				prints("再次呼叫.\n");
				add_io(sock, 20);	/* 1999.12.20 */
				bell();
				if (kill(uin.pid, SIGUSR1) == -1) {
					move(0, 0);
					prints("对方已离线\n");
					pressreturn();
					/* Add by SmallPig 2 lines */
					uinfo.sockactive = NA;
					uinfo.destuid = 0;
					return -1;
				}
				continue;
			}
			if (ch == I_OTHERDATA)
				break;
			if (ch == '\004') {
				add_io(0, 0);
				close(sock);
				uinfo.sockactive = NA;
				uinfo.destuid = 0;
				clear();
				return 0;
			}
		}

		msgsock = accept(sock, (struct sockaddr *) 0, (socklen_t *) 0);
		add_io(0, 0);
		close(sock);
		uinfo.sockactive = NA;
//		uinfo.destuid = 0;
		read(msgsock, &c, sizeof (c));

		clear();

		switch (c) {
#ifdef FIVEGAME
		case 'y':
		case 'Y':
		case 'w':
		case 'W':	/*added for FIVE,by djq. */
			snprintf(save_page_requestor, sizeof(save_page_requestor), "%s (%s)", uin.userid, uin.username);
			if (five == 1) {
				five_pk(msgsock, 1);
			} else {
				do_talk(msgsock);
			}
			break;
#else

		case 'y':
		case 'Y':
			snprintf(save_page_requestor, sizeof(save_page_requestor), "%s (%s)", uin.userid, uin.username);
			do_talk(msgsock);
			break;
#endif
		case 'a':
		case 'A':
			prints("%s (%s)说：%s\n", uin.userid, uin.username, refuse[0]);
			pressreturn();
			break;
		case 'b':
		case 'B':
			prints("%s (%s)说：%s\n", uin.userid, uin.username, refuse[1]);
			pressreturn();
			break;
		case 'c':
		case 'C':
			prints("%s (%s)说：%s\n", uin.userid, uin.username, refuse[2]);
			pressreturn();
			break;
		case 'd':
		case 'D':
			prints("%s (%s)说：%s\n", uin.userid, uin.username, refuse[3]);
			pressreturn();
			break;
		case 'e':
		case 'E':
			prints("%s (%s)说：%s\n", uin.userid, uin.username, refuse[4]);
			pressreturn();
			break;
		case 'f':
		case 'F':
			prints("%s (%s)说：%s\n", uin.userid, uin.username, refuse[5]);
			pressreturn();
			break;
		case 'g':
		case 'G':
			prints("%s (%s)说：%s\n", uin.userid, uin.username, refuse[6]);
			pressreturn();
			break;
		case 'n':
		case 'N':
			prints("%s (%s)说：%s\n", uin.userid, uin.username, refuse[7]);
			pressreturn();
			break;
		case 'm':
		case 'M':
			read(msgsock, reason, sizeof (reason));
			prints("%s (%s)说：%s\n", uin.userid, uin.username, reason);
			pressreturn();
			break;
		default:
			snprintf(save_page_requestor, sizeof(save_page_requestor), "%s (%s)", uin.userid, uin.username);
#ifdef TALK_LOG
			strcpy(partner, uin.userid);
#endif
			do_talk(msgsock);
			break;
		}
		close(msgsock);
		clear();
		refresh();
		uinfo.destuid = 0;
	}
	return 0;
}

extern int talkrequest;
struct user_info ui;
char page_requestor[STRLEN];
char page_requestorid[STRLEN];

int
cmpunums(int unum, struct user_info *up)
{
	if (!up->active)
		return 0;
	return (unum == up->destuid);
}

int
cmpmsgnum(int unum, struct user_info *up)
{
	if (!up->active)
		return 0;
	return (unum == up->destuid && up->sockactive == 2);
}

int
setpagerequest(int mode)
{
	int tuid;

	if (mode == 0)
		tuid = search_ulist(&ui, cmpunums, usernum);
	else
		tuid = search_ulist(&ui, cmpmsgnum, usernum);
	if (tuid == 0)
		return 1;
	if (!ui.sockactive)
		return 1;
	uinfo.destuid = ui.uid;
	snprintf(page_requestor, sizeof(page_requestor), "%s (%s)", ui.userid, ui.username);
	strcpy(page_requestorid, ui.userid);
	return 0;
}

int
servicepage(int line, char *mesg)
{
	static time_t last_check;
	time_t now;
	char buf[STRLEN];
	int tuid = search_ulist(&ui, cmpunums, usernum);

	if (tuid == 0 || !ui.sockactive)
		talkrequest = NA;
	if (!talkrequest) {
		if (page_requestor[0]) {
			switch (uinfo.mode) {
			case TALK:
#ifdef FIVEGAME
			case FIVE:	//added by djq,for five
#endif
				move(line, 0);
				printdash(mesg);
				break;
			default:	/* a chat mode */
				snprintf(buf, sizeof(buf), "** %s 已停止呼叫.", page_requestor);
				printchatline(buf);
			}
			memset(page_requestor, 0, STRLEN);
			last_check = 0;
		}
		return NA;
	} else {
		now = time(NULL);
		if (now - last_check > P_INT) {
			last_check = now;
			if (!page_requestor[0] &&
			    setpagerequest(0 /* For Talk */ ))
				return NA;
			else
				switch (uinfo.mode) {
				case TALK:
					move(line, 0);
					snprintf(buf, sizeof(buf), "** %s 正在呼叫你", page_requestor);
					printdash(buf);
					break;
				default:	/* chat */
					snprintf(buf, sizeof(buf), "** %s 正在呼叫你", page_requestor);
					printchatline(buf);
				}
		}
	}
	return YEA;
}

int
talkreply(void)
{
	int fd;
	char buf[512];
	char reason[51];
	char inbuf[STRLEN * 2];

#ifdef FIVEGAME
/* added by djq 99.07.19,for FIVE */
	struct user_info uip;
	int five = 0;
	int tuid;
#endif
	talkrequest = NA;
	if (setpagerequest(0 /* For Talk */ ))
		return 0;
#ifdef DOTIMEOUT
	init_alarm();
#else
	signal(SIGALRM, SIG_IGN);
#endif
	clear();
#ifdef FIVEGAME
	/* modified by djq, 99.07.19, for FIVE */
	if (!(tuid = getuser(page_requestorid, NULL)))
		return 0;
	who_callme(&uip, t_cmpuids, tuid, uinfo.uid);
	uinfo.destuid = uip.uid;
	if (uip.mode == PAGE_FIVE)
		five = 1;
//	getuser(uip.userid);

#endif
	move(5, 0);
	clrtobot();
	show_user_plan(page_requestorid);
	move(1, 0);
	prints("(A)【%s】(B)【%s】\n", refuse[0], refuse[1]);
	prints("(C)【%s】(D)【%s】\n", refuse[2], refuse[3]);
	prints("(E)【%s】(F)【%s】\n", refuse[4], refuse[5]);
	prints("(G)【%s】(N)【%s】\n", refuse[6], refuse[7]);
	prints("(M)【留言给 %-13s            】\n", page_requestorid);
#ifndef FIVEGAME
	snprintf(inbuf, sizeof(inbuf), "你想跟 %s 聊聊天吗? (Y N A B C D E F G M)[Y]: ", page_requestor);
#else
	snprintf(inbuf, sizeof(inbuf), "你想跟 %s %s吗？请选择(Y/N/A/B/C/D)[Y] ", page_requestor, (five) ? "下五子棋" : "聊聊天");
#endif
	strcpy(save_page_requestor, page_requestor);
#ifdef TALK_LOG
	strcpy(partner, page_requestorid);
#endif
	memset(page_requestor, 0, sizeof (page_requestor));
	memset(page_requestorid, 0, sizeof (page_requestorid));
	getdata(0, 0, inbuf, buf, 2, DOECHO, YEA);

	if ((fd = async_connect(NULL, ntohs(ui.sockaddr), TALK_CONNECT_TIMEOUT)) < 0)
		return -1;

	if (buf[0] != 'A' && buf[0] != 'a' && buf[0] != 'B' && buf[0] != 'b'
	    && buf[0] != 'C' && buf[0] != 'c' && buf[0] != 'D' && buf[0] != 'd'
	    && buf[0] != 'e' && buf[0] != 'E' && buf[0] != 'f' && buf[0] != 'F'
	    && buf[0] != 'g' && buf[0] != 'G' && buf[0] != 'n' && buf[0] != 'N'
	    && buf[0] != 'm' && buf[0] != 'M')
		buf[0] = 'y';
	if (buf[0] == 'M' || buf[0] == 'm') {
		move(1, 0);
		clrtobot();
		getdata(1, 0, "留话：", reason, 50, DOECHO, YEA);
	}
	write(fd, buf, 1);
	if (buf[0] == 'M' || buf[0] == 'm')
		write(fd, reason, sizeof (reason));
	if (buf[0] != 'y') {
		close(fd);
		report("page refused");
		clear();
		refresh();
		return 0;
	}
	report("page accepted");
	clear();
#ifndef FIVEGAME
/* modified by djq 99.07.19 for FIVE */
	do_talk(a);
#else
	if (!five) {
		do_talk(fd);
	} else {
		five_pk(fd, 0);
	}
#endif
	close(fd);
	clear();
	refresh();
	return 0;
}

void
do_talk_nextline(struct talk_win *twin)
{

	twin->curln = twin->curln + 1;
	if (twin->curln > twin->eline)
		twin->curln = twin->sline;
	if (twin->curln != twin->eline) {
		move(twin->curln + 1, 0);
		clrtoeol();
	}
	move(twin->curln, 0);
	clrtoeol();
	twin->curcol = 0;
}

void
do_talk_char(struct talk_win *twin, int ch)
{
	if (isprint2(ch)) {
		if (twin->curcol < 79) {
			move(twin->curln, (twin->curcol)++);
			prints("%c", ch);
			return;
		}
		do_talk_nextline(twin);
		twin->curcol++;
		prints("%c", ch);
		return;
	}
	switch (ch) {
	case Ctrl('H'):
	case '\177':
		if (twin->curcol == 0) {
			return;
		}
		(twin->curcol)--;
		move(twin->curln, twin->curcol);
		prints(" ");
		move(twin->curln, twin->curcol);
		return;
	case Ctrl('M'):
	case Ctrl('J'):
		do_talk_nextline(twin);
		return;
	case Ctrl('G'):
		bell();
		return;
	default:
		break;
	}
	return;
}

void
do_talk_string(struct talk_win *twin, char *str)
{
	while (*str) {
		do_talk_char(twin, *str++);
	}
}

char talkobuf[80];
int talkobuflen;
int talkflushfd;

void
talkflush(void)
{
	if (talkobuflen)
		write(talkflushfd, talkobuf, talkobuflen);
	talkobuflen = 0;
}

void
moveto(int mode, struct talk_win *twin)
{
	if (mode == 1)
		twin->curln--;
	if (mode == 2)
		twin->curln++;
	if (mode == 3)
		twin->curcol++;
	if (mode == 4)
		twin->curcol--;
	if (twin->curcol < 0) {
		twin->curln--;
		twin->curcol = 0;
	} else if (twin->curcol > 79) {
		twin->curln++;
		twin->curcol = 0;
	}
	if (twin->curln < twin->sline) {
		twin->curln = twin->eline;
	}
	if (twin->curln > twin->eline) {
		twin->curln = twin->sline;
	}
	move(twin->curln, twin->curcol);
}

void
endmsg(int signo)
{
	int x, y;
	int tmpansi;

	tmpansi = showansi;
	showansi = 1;
	talkidletime += 60;
	if (talkidletime >= IDLE_TIMEOUT)
		safe_kill(getpid());
	if (uinfo.in_chat == YEA)
		return;
	getyx(&x, &y);
	update_endline();
	signal(SIGALRM, endmsg);
	move(x, y);
	refresh();
	alarm(60);
	showansi = tmpansi;
	return;
}

int
do_talk(int fd)
{
	struct talk_win mywin, itswin;
	char mid_line[256];
	int page_pending = NA;
	int i, i2;
	char ans[3];
	int previous_mode;

#ifdef TALK_LOG
	char tlogname[STRLEN];
	char mywords[80], itswords[80], talkbuf[80];
	int mlen = 0, ilen = 0;
	time_t now;

	mywords[0] = itswords[0] = '\0';
#endif

	signal(SIGALRM, SIG_IGN);
	endmsg(SIGALRM);
	refresh();
	previous_mode = uinfo.mode;
	modify_user_mode(TALK);
	snprintf(mid_line, sizeof(mid_line), " %s (%s) 和 %s 正在畅谈中", currentuser.userid, currentuser.username, save_page_requestor);

	memset(&mywin, 0, sizeof (mywin));
	memset(&itswin, 0, sizeof (itswin));
	i = (t_lines - 1) >> 1;
	mywin.eline = i - 1;
	itswin.curln = itswin.sline = i + 1;
	itswin.eline = t_lines - 2;
	move(i, 0);
	printdash(mid_line);
	move(0, 0);

	talkobuflen = 0;
	talkflushfd = fd;
	add_io(fd, 0);
	add_flush(talkflush);

	while (YEA) {
		int ch;

		if (talkrequest)
			page_pending = YEA;
		if (page_pending)
			page_pending = servicepage((t_lines - 1) >> 1, mid_line);
		ch = igetkey();
		talkidletime = 0;
		if (ch == '\033') {
			igetkey();
			igetkey();
			continue;
		}
		if (ch == I_OTHERDATA) {
			char data[80];
			int datac, i;

			datac = read(fd, data, 80);
			if (datac <= 0)
				break;
			for (i = 0; i < datac; i++) {
				if (data[i] >= 1 && data[i] <= 4) {
					moveto(data[i] - '\0', &itswin);
					continue;
				}
#ifdef TALK_LOG
				/*
				 * Sonny.990514 add an robust and fix some
				 * logic problem
				 */
				/*
				 * Sonny.990606 change to different algorithm
				 * and fix the
				 */
				/* existing do_log() overflow problem       */
				else if (isprint2((unsigned int)data[i])) {
					if (ilen >= 80) {
						itswords[79] = '\0';
						(void) do_log(itswords, 2);
						ilen = 0;
					} else {
						itswords[ilen] = data[i];
						ilen++;
					}
				} else
				    if ((data[i] == Ctrl('H') ||
					 data[i] == '\177') && !ilen) {
					itswords[ilen--] = '\0';
				} else if (data[i] == Ctrl('M') ||
					   data[i] == '\r' || data[i] == '\n') {
					itswords[ilen] = '\0';
					(void) do_log(itswords, 2);
					ilen = 0;
				}
#endif
				do_talk_char(&itswin, data[i]);
			}
		} else {
			if (ch == Ctrl('D') || ch == Ctrl('C'))
				break;
			if (isprint2(ch) || ch == Ctrl('H') || ch == '\177'
			    || ch == Ctrl('G')) {
				talkobuf[talkobuflen++] = ch;
				if (talkobuflen == 80)
					talkflush();
#ifdef TALK_LOG
				if (mlen < 80) {
					if ((ch == Ctrl('H') || ch == '\177') &&
					    mlen != 0) {
						mywords[mlen--] = '\0';
					} else {
						mywords[mlen] = ch;
						mlen++;
					}
				} else if (mlen >= 80) {
					mywords[79] = '\0';
					(void) do_log(mywords, 1);
					mlen = 0;
				}
#endif
				do_talk_char(&mywin, ch);
			} else if (ch == '\n' || ch == Ctrl('M') || ch == '\r') {
#ifdef TALK_LOG
				if (mywords[0] != '\0') {
					mywords[mlen++] = '\0';
					(void) do_log(mywords, 1);
					mlen = 0;
				}
#endif
				talkobuf[talkobuflen++] = '\r';
				talkflush();
				do_talk_char(&mywin, '\r');
			} else if (ch >= KEY_UP && ch <= KEY_LEFT) {
				moveto(ch - KEY_UP + 1, &mywin);
				talkobuf[talkobuflen++] = ch - KEY_UP + 1;
				if (talkobuflen == 80)
					talkflush();
			} else if (ch == Ctrl('E')) {
				for (i2 = 0; i2 <= 10; i2++) {
					talkobuf[talkobuflen++] = '\r';
					talkflush();
					do_talk_char(&mywin, '\r');
				}
			} else if (ch == Ctrl('P') && HAS_PERM(PERM_BASIC)) {
				t_pager();
				update_utmp();
				update_endline();
			}
		}
	}
	add_io(0, 0);
	talkflush();
	signal(SIGALRM, SIG_IGN);
	add_flush(NULL);
	modify_user_mode(previous_mode);

#ifdef TALK_LOG
	/* edwardc.990106 聊天纪录 */
	mywords[mlen] = '\0';
	itswords[ilen] = '\0';
	if (mywords[0] != '\0')
		do_log(mywords, 1);
	if (itswords[0] != '\0')
		do_log(itswords, 2);

	now = time(NULL);
	snprintf(talkbuf, sizeof(talkbuf), "\n\033[1;34m通话结束, 时间: %s \033[m\n", Cdate(&now));
	write(talkrec, talkbuf, strlen(talkbuf));
	close(talkrec);
	talkrec = -1;

	sethomefilewithpid(tlogname, currentuser.userid, "talklog");
	if (!dashf(tlogname))
		return 0;

	getdata(t_lines - 1, 0, "是否寄回聊天纪录 [Y/n]: ", ans, 2, DOECHO,
		YEA);

	switch (ans[0]) {
	case 'n':
	case 'N':
		break;
	default:
		snprintf(mywords, sizeof(mywords), "跟 %s 的聊天记录 [%s]", partner, Cdate(&now) + 4);
		/* borrow the talkbuf to save the title, instead of
		 * using genbuf which is used by mail_file */ 
		strlcpy(talkbuf, save_title, sizeof(save_title));
		mail_sysfile(tlogname, currentuser.userid, mywords);
		strlcpy(save_title, talkbuf, sizeof(save_title));
	}
	unlink(tlogname);
#endif
	return 0;
}

int
shortulist()
{
	int i;
	int pageusers = 60;
	extern struct user_info *user_record[];
	extern int range;

	fill_userlist();
	if (ulistpage > ((range - 1) / pageusers))
		ulistpage = 0;
	if (ulistpage < 0)
		ulistpage = (range - 1) / pageusers;
	move(1, 0);
	clrtoeol();
	prints
	    ("每隔 \033[1;32m%d\033[m 秒更新一次，\033[1;32mCtrl-C\033[m 或 \033[1;32mCtrl-D\033[m 离开，\033[1;32mF\033[m 更换模式 \033[1;32m↑↓\033[m 上、下一页 第\033[1;32m %1d\033[m 页",
	     M_INT, ulistpage + 1);
	clrtoeol();
	move(3, 0);
	clrtobot();
	for (i = ulistpage * pageusers;
	     i < (ulistpage + 1) * pageusers && i < range; i++) {
		char ubuf[STRLEN];
		int ovv;

		ovv = (i < numf || friendmode) ? YEA : NA;
		snprintf(ubuf, sizeof(ubuf), "%s%-12.12s %s%-10.10s\033[m",
			(ovv) ? "□\033[1;32m" : "  ", user_record[i]->userid,
			(user_record[i]->invisible == YEA) ? "\033[1;34m" : "",
			modetype(user_record[i]->mode));
		//modestring(user_record[i]->mode, user_record[i]->destuid, 0, NULL));
		outs(ubuf);
		if ((i + 1) % 3 == 0)
			outc('\n');
		else
			outs(" |");
	}
	return range;
}

int
do_list(char *modestr)
{
	char buf[STRLEN];
	int count;
	extern int RMSG;

	if (RMSG != YEA) {	/* 如果收到 Msg 第一行不显示。 */
		showtitle(modestr, chkmail(NA) ? "[您有信件]" : BoardName);
	}
	clear_line(2);
	snprintf(buf, sizeof(buf), "  %-12s %-10s", "使用者代号", "目前动态");
	prints("\033[1;33;44m%s |%s |%s\033[m", buf, buf, buf);
	count = shortulist();
	if (uinfo.mode == MONITOR) {
		time_t thetime = time(NULL);

		move(t_lines - 1, 0);
		getdatestring(thetime);
		prints("\033[1;44;33m  目前有 \033[32m%3d\033[33m %6s上线, 时间: \033[32m%22.22s \033[33m, 目前状态：\033[36m%10s   \033[m",
			count, friendmode ? "好朋友" : "使用者", datestring,
			friendmode ? "你的好朋友" : "所有使用者");
	}
	refresh();
	return 0;
}

int
t_list(void)
{
	modify_user_mode(LUSERS);
	report("t_list");
	do_list("使用者状态");
	pressreturn();
	refresh();
	clear();
	return 0;
}

static void
sig_catcher(int signo)
{
	ulistpage++;
	if (uinfo.mode != MONITOR) {
#ifdef DOTIMEOUT
		init_alarm();
#else
		signal(SIGALRM, SIG_IGN);
#endif
		return;
	}
	if (signal(SIGALRM, sig_catcher) == SIG_ERR)
		exit(1);
	do_list("探视民情");
	alarm(M_INT);
}

int
t_monitor(void)
{
	int i;
	char modestr[] = "探视民情";

	alarm(0);
	signal(SIGALRM, sig_catcher);
// 	idle_monitor_time = 0;
	modify_user_mode(MONITOR);
	ulistpage = 0;
	do_list(modestr);
	alarm(M_INT);
	while (YEA) {
		i = egetch();
		if (i == 'f' || i == 'F') {
			if (friendmode == YEA)
				friendmode = NA;
			else
				friendmode = YEA;
			do_list(modestr);
		}
		if (i == KEY_DOWN) {
			ulistpage++;
			do_list(modestr);
		}
		if (i == KEY_UP) {
			ulistpage--;
			do_list(modestr);
		}
		if (i == Ctrl('D') || i == Ctrl('C') || i == KEY_LEFT)
			break;
/*        else if (i == -1) {
	    if (errno != EINTR) exit(1);
	} else idle_monitor_time = 0;*/
	}
	move(2, 0);
	clrtoeol();
	clear();
	return 0;
}

int
addtooverride(char *uident)
{
	struct override tmp;
	int n = 0;
	char buf[STRLEN];
	char desc[5];

	memset(&tmp, 0, sizeof (tmp));
	switch (friendflag) {
	case 1:
		setuserfile(buf, "friends");
		n = MAXFRIENDS;
		strcpy(desc, "好友");
		break;
	case 0:
		setuserfile(buf, "rejects");
		n = MAXREJECTS;
		strcpy(desc, "坏人");
		break;
	case 2:
		setuserfile(buf, "maildeny");
		n = MAXREJECTS;
		strcpy(desc, "拒收");
		break;
	}
	if (get_num_records(buf, sizeof (struct override)) >= n) {
		move(t_lines - 1, 0);
		clrtoeol();
		prints("抱歉，本站目前仅可以设定 %d 个%s, 请按任意键继续...", n, desc);
		igetkey();
		move(t_lines - 1, 0);
		clrtoeol();
		return -1;
	} else {
		switch (friendflag) {
		case 1:
			if (myfriend(searchuser(uident))) {
				snprintf(buf, sizeof(buf), "%s 已在好友名单", uident);
				show_message(buf);
				return -1;
			}
		case 0:
			if (search_record(buf, &tmp, sizeof (tmp), cmpfnames, uident) > 0) {
				snprintf(buf, sizeof(buf), "%s 已在坏人名单", uident);
				show_message(buf);
				return -1;
			}
		case 2:
			if (search_record(buf, &tmp, sizeof (tmp), cmpfnames, uident) > 0) {
				snprintf(buf, sizeof(buf), "%s 已在拒收名单", uident);
				show_message(buf);
				return -1;
			}
		}
	}

//      if (uinfo.mode != LUSERS && uinfo.mode != LAUSERS && uinfo.mode != FRIEND)
	if (uinfo.mode == GMENU)
		n = 2;
	else
		n = t_lines - 1;

	strcpy(tmp.id, uident);
	refresh();
	clear_line(n);
	snprintf(genbuf, sizeof(genbuf), "请输入给%s【%s】的说明: ", desc, tmp.id);
	getdata(n, 0, genbuf, tmp.exp, 40, DOECHO, YEA);

	n = append_record(buf, &tmp, sizeof (struct override));
	if (n != -1)
		switch (friendflag) {
		case 1:
			getfriendstr();
			break;
		case 0:
			getrejectstr();
			break;
		case 2:
			break;
	} else
		report("append override error");
	return n;
}

int
del_from_file(char *filename, char *str)
{
	FILE *fp, *nfp;
	int deleted = NA;
	char tmpbuf[1024], fnnew[PATH_MAX + 1];

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	snprintf(fnnew, sizeof(fnnew), "%s.%d", filename, getuid());
	if ((nfp = fopen(fnnew, "w")) == NULL) {
		fclose(fp);	/* add by quickmouse 01/03/09 */
		return -1;
	}
	while (fgets(tmpbuf, 1024, fp) != NULL) {
		if (strncmp(tmpbuf, str, strlen(str)) == 0
		    && (tmpbuf[strlen(str)] == '\0' ||
			tmpbuf[strlen(str)] == ' ' ||
			tmpbuf[strlen(str)] == '\n'))
			deleted = YEA;
		else if (*tmpbuf > ' ')
			fputs(tmpbuf, nfp);
	}
	fclose(fp);
	fclose(nfp);
	if (!deleted)
		return -1;
	return (rename(fnnew, filename) + 1);
}

int
deleteoverride(char *uident, char *filename)
{
	int deleted;
	struct override fh;
	char buf[STRLEN];

	setuserfile(buf, filename);
	deleted = search_record(buf, &fh, sizeof (fh), cmpfnames, uident);
	if (deleted > 0) {
		if (delete_record(buf, sizeof (fh), deleted) != -1) {
			switch (friendflag) {
			case 1:
				getfriendstr();
				break;
			case 0:
				getrejectstr();
				break;
			case 2:
				break;
			}
		} else {
			deleted = -1;
			report("delete override error");
		}
	}
	return (deleted > 0) ? 1 : -1;
}

void
override_title(void)
{
	char desc[5];

	strcpy(genbuf, chkmail(NA) ? "[您有信件]" : BoardName);
	switch (friendflag) {
	case 1:
		showtitle("编辑好友名单", genbuf);
		strcpy(desc, "好友");
		break;
	case 0:
		showtitle("编辑坏人名单", genbuf);
		strcpy(desc, "坏人");
		break;
	case 2:
		showtitle("编辑拒收名单", genbuf);
		strcpy(desc, "拒收");
		break;
	}
	prints
	    (" [\033[1;32m←\033[m,\033[1;32me\033[m] 离开 [\033[1;32mh\033[m] 求助 [\033[1;32m→\033[m,\033[1;32mRtn\033[m] %s说明档 [\033[1;32m↑\033[m,\033[1;32m↓\033[m] 选择 [\033[1;32ma\033[m] 增加%s [\033[1;32md\033[m] 删除%s\n",
	     desc, desc, desc);
	prints
	    ("\033[1;44m 编号  %s代号      %s说明                                                   \033[m\n",
	     desc, desc);
}

char *
override_doentry(int num, void *ent_ptr)
{
	static char buf[STRLEN];
	struct override *ent = (struct override *)ent_ptr;

	snprintf(buf, sizeof(buf), " %4d  %-12.12s  %s", num, ent->id, ent->exp);
	return buf;
}

int
override_edit(int ent, struct override *fh, char *direc)
{
	struct override nh;
	char buf[STRLEN / 2];
	int pos;

	clear_line(t_lines);
	if ((pos = search_record(direc, &nh, sizeof (nh), cmpfnames, fh->id)) > 0) {
		snprintf(buf, sizeof(buf), "请输入 %s 的新%s说明: ", fh->id,
			(friendflag) ? ((friendflag == 1) ? "好友" : "拒收") : "坏人");
		getdata(t_lines - 2, 0, buf, nh.exp, 40, DOECHO, NA);
	}
	substitute_record(direc, &nh, sizeof (nh), pos);
	clear_line(t_lines - 2);
	return NEWDIRECT;
}

int
override_add(int ent, struct override *fh, char *direct)
{
	char uident[IDLEN + 2];

	clear();
	move(1, 0);
	usercomplete("请输入要增加的代号: ", uident);
	if (uident[0] != '\0') {
		if (getuser(uident, NULL) == 0) {
			move(2, 0);
			outs("错误的使用者代号...");
			pressanykey();
			return FULLUPDATE;
		} else {
			addtooverride(uident);
		}
		prints("\n把 %s 加入%s名单中...", uident,
		       (friendflag) ? ((friendflag == 1) ? "好友" : "拒收") : "坏人");
		pressanykey();
	}
	return FULLUPDATE;
}

int
override_dele(int ent, struct override *fh, char *direct)
{
	char buf[STRLEN];
	char desc[5];
	char fname[10];

	switch (friendflag) {
	case 1:
		strcpy(desc, "好友");
		strcpy(fname, "friends");
		break;
	case 0:
		strcpy(desc, "坏人");
		strcpy(fname, "rejects");
		break;
	case 2:
		strcpy(desc, "拒收");
		strcpy(fname, "maildeny");
		break;
	}

	move(t_lines - 2, 0);
	snprintf(buf, sizeof(buf), "是否把【%s】从%s名单中去除", fh->id, desc);
	if (askyn(buf, NA, NA) == YEA) {
		clear_line(t_lines - 2);
		if (deleteoverride(fh->id, fname) == 1) {
			prints("已从%s名单中移除【%s】,按任何键继续...", desc, fh->id);
		} else {
			prints("找不到【%s】,按任何键继续...", fh->id);
		}
		egetch();
		return DIRCHANGED;
	} 

	clear_line(t_lines - 2);
	prints("取消删除%s...", desc);
	egetch();
	return PARTUPDATE;
}

int
friend_edit(int ent, struct override *fh, char *direct)
{
	friendflag = 1;
	return override_edit(ent, fh, direct);
}

int
friend_add(int ent, struct override *fh, char *direct)
{
	friendflag = 1;
	return override_add(ent, fh, direct);
}

int
friend_dele(int ent, struct override *fh, char *direct)
{
	friendflag = 1;
	return override_dele(ent, fh, direct);
}

int
friend_mail(int ent, struct override *fh, char *direct)
{
	if (!HAS_PERM(PERM_POST))
		return DONOTHING;
	m_send(fh->id);
	return FULLUPDATE;
}

int
friend_query(int ent, struct override *fh, char *direct)
{
	int ch;

	if (t_query(fh->id) == -1)
		return FULLUPDATE;
	move(t_lines - 1, 0);
	clrtoeol();
	prints
	    ("\033[0;1;44;31m[读取好友说明档]\033[33m 寄信给好友 m │ 结束 Q,← │上一位 ↑│下一位 <Space>,↓      \033[m");
	ch = egetch();
	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case 'm':
	case 'M':
		m_send(fh->id);
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	default:
		break;
	}
	return FULLUPDATE;
}

int
friend_help(void)
{
	show_help("help/friendshelp");
	return FULLUPDATE;
}

int
reject_edit(int ent, struct override *fh, char *direct)
{
	friendflag = 0;
	return override_edit(ent, fh, direct);
}

int
reject_add(int ent, struct override *fh, char *direct)
{
	friendflag = 0;
	return override_add(ent, fh, direct);
}

int
reject_dele(int ent, struct override *fh, char *direct)
{
	friendflag = 0;
	return override_dele(ent, fh, direct);
}

int
reject_query(int ent, struct override *fh, char *direct)
{
	int ch;

	if (t_query(fh->id) == -1)
		return FULLUPDATE;
	clear_line(t_lines - 1);
	outs("\033[0;1;44;31m[读取坏人说明档]\033[33m 结束 Q,← │上一位 ↑│下一位 <Space>,↓                      \033[m");
	ch = egetch();
	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	default:
		break;
	}
	return FULLUPDATE;
}

int
reject_help(void)
{
	show_help("help/rejectshelp");
	return FULLUPDATE;
}

/* Added by cancel at 01/09/18 : 信件黑名单 */
int
maildeny_edit(int ent, struct override *fh, char *direct)
{
	friendflag = 2;
	return override_edit(ent, fh, direct);
}

int
maildeny_add(int ent, struct override *fh, char *direct)
{
	friendflag = 2;
	return override_add(ent, fh, direct);
}

int
maildeny_dele(int ent, struct override *fh, char *direct)
{
	friendflag = 2;
	return override_dele(ent, fh, direct);
}

int
maildeny_query(int ent, struct override *fh, char *direct)
{
	int ch;

	if (t_query(fh->id) == -1)
		return FULLUPDATE;
	clear_line(t_lines - 1);
	outs("\033[0;1;44;31m[读取黑名单说明档]\033[33m 结束 Q,← │上一位 ↑│下一位 <Space>,↓                      \033[m");
	ch = egetch();
	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	default:
		break;
	}
	return FULLUPDATE;
}

int
maildeny_help(void)
{
	show_help("help/maildenyshelp");
	return FULLUPDATE;
}

/* Added End. */

int
t_friend(void)
{
	char buf[STRLEN];

	friendflag = 1;
	setuserfile(buf, "friends");
	i_read(GMENU, buf, NULL, NULL, override_title, override_doentry, update_endline, friend_list,
	       get_records, get_num_records, sizeof(struct override));
	clear();
	return 0;
}

int
t_reject(void)
{
	char buf[STRLEN];

	friendflag = 0;
	setuserfile(buf, "rejects");
	i_read(GMENU, buf, NULL, NULL, override_title, override_doentry, update_endline, reject_list,
	       get_records, get_num_records, sizeof(struct override));
	clear();
	return 0;
}

/* Added by cancel at 01/09/18 : 拒收邮件列表 */
int
t_maildeny(void)
{
	char buf[STRLEN];

	friendflag = 2;
	setuserfile(buf, "maildeny");
	i_read(GMENU, buf, NULL, NULL, override_title, override_doentry, update_endline, maildeny_list,
	       get_records, get_num_records, sizeof(struct override));
	clear();
	return 0;
}

/* Added End. */
struct user_info *
t_search(char *sid, int pid)
{
	int i;
	struct user_info *cur, *tmp = NULL;

	resolve_utmp();
	for (i = 0; i < USHM_SIZE; i++) {
		cur = &(utmpshm->uinfo[i]);
		if (!cur->active || !cur->pid)
			continue;
		if (!strcasecmp(cur->userid, sid)) {
			if (pid == 0)
				return isreject(cur) ? NULL : cur;
			tmp = cur;
			if (pid == cur->pid)
				break;
		}
	}
/*
if (tmp != NULL) {
		if (tmp->invisible && !HAS_PERM(PERM_SEECLOAK | PERM_SYSOP))
			return NULL;
	}
*/
	return isreject(cur) ? NULL : tmp;
}

int
cmpfuid(const void *a, const void *b)
{
	return *(short unsigned int *)a - *(short unsigned int *)b;
}

int
getfriendstr(void)
{
	int i, count = 0;
	struct override *tmp;

	memset(uinfo.friends, 0, sizeof (uinfo.friends));
	setuserfile(genbuf, "friends");
	uinfo.fnum = get_num_records(genbuf, sizeof (struct override));
	if (uinfo.fnum <= 0)
		return 0;
	uinfo.fnum = (uinfo.fnum >= MAXFRIENDS) ? MAXFRIENDS : uinfo.fnum;
	tmp = (struct override *) calloc(sizeof (struct override), uinfo.fnum);
	get_records(genbuf, tmp, sizeof (struct override), 1, uinfo.fnum);
	for (i = 0; i < uinfo.fnum; i++) {
		uinfo.friends[count] = searchuser(tmp[i].id);
		if (uinfo.friends[count] > 0)
			count++;
	}
	free(tmp);
	uinfo.fnum = count;
	qsort(&uinfo.friends, uinfo.fnum, sizeof (uinfo.friends[0]), cmpfuid);
	update_ulist(&uinfo, utmpent);
	return 0;
}

int
getrejectstr(void)
{
	int nr, i, count = 0;
	struct override *tmp;

	memset(uinfo.reject, 0, sizeof (uinfo.reject));
	setuserfile(genbuf, "rejects");
	nr = get_num_records(genbuf, sizeof (struct override));
	if (nr <= 0)
		return 0;
	nr = (nr >= MAXREJECTS) ? MAXREJECTS : nr;
	tmp = (struct override *) calloc(sizeof (struct override), nr);
	get_records(genbuf, tmp, sizeof (struct override), 1, nr);
	for (i = 0; i < nr; i++) {
		uinfo.reject[count] = searchuser(tmp[i].id);
		if (uinfo.reject[count] > 0)
			count++;
	}
	free(tmp);
	qsort(&uinfo.reject, count, sizeof (uinfo.reject[0]), cmpfuid);
	update_ulist(&uinfo, utmpent);
	return 0;
}

#ifdef CHK_FRIEND_BOOK
int
wait_friend()
{
	FILE *fp;
	int tuid;
	char buf[STRLEN];
	char uid[IDLEN + 2];

	modify_user_mode(WFRIEND);
	clear();
	move(1, 0);
	usercomplete("请输入使用者代号以加入系统的寻人名册: ", uid);
	if (uid[0] == '\0') {
		clear();
		return 0;
	}
	if (!(tuid = getuser(uid))) {
		move(2, 0);
		prints("\033[1m不正确的使用者代号\033[m\n");
		pressanykey();
		clear();
		return -1;
	}
	snprintf(buf, sizeof(buf), "你确定要把 \033[1m%s\033[m 加入系统寻人名单中", uid);
	move(2, 0);
	if (askyn(buf, YEA, NA) == NA) {
		clear();
		return;
	}
	if ((fp = fopen("friendbook", "a+")) == NULL) {
		prints("系统的寻人名册无法开启，请通知站长...\n");
		pressanykey();
		return NA;
	}
	snprintf(buf, sizeof(buf), "%d@%s", tuid, currentuser.userid);
	if (!seek_in_file("friendbook", buf))
		fprintf(fp, "%s\n", buf);
	fclose(fp);
	move(3, 0);
	prints("已经帮你加入寻人名册中，\033[1m%s\033[m 上站系统一定会通知你...\n",
	       uid);
	pressanykey();
	clear();
	return;
}
#endif

/* 统计当前隐身人数  */
/*
int
CountCloakMan(void)
{
	int     i, num = 0 ;
	struct user_info *cur;
	resolve_utmp();
	for (i = 0; i < USHM_SIZE; i++) {
		cur = &(utmpshm->uinfo[i]);
		if (!cur->active || !cur->pid || !cur->invisible)
			continue;
		num ++ ;
	}
	return num;
}
*/

#ifdef TALK_LOG
/* edwardc.990106 分别为两位聊天的人作纪录 */
/* -=> 自己说的话 */
/* --> 对方说的话 */

void
do_log(char *msg, int who)
{
	time_t now;
	char buf[100];

	now = time(NULL);
	if (msg[strlen(msg)] == '\n')
		msg[strlen(msg)] = '\0';

	if (msg[0] == '\0' || msg[0] == '\r' || msg[0] == '\n')
		return;

	/* 只帮自己做 */
	sethomefilewithpid(buf, currentuser.userid, "talklog");

	if (!dashf(buf) || talkrec == -1) {
		talkrec = open(buf, O_RDWR | O_CREAT | O_TRUNC, 0644);
		snprintf(buf, sizeof(buf), "\033[1;32m与 %s 的情话绵绵, 日期: %s \033[m\n",
			save_page_requestor, Cdate(&now));
		write(talkrec, buf, strlen(buf));
		snprintf(buf, sizeof(buf), "\t颜色分别代表: \033[1;33m%s\033[m \033[1;36m%s\033[m \n\n",
			currentuser.userid, partner);
		write(talkrec, buf, strlen(buf));
	}
	if (who == 1) {		/* 自己说的话 */
		snprintf(buf, sizeof(buf), "\033[1;33m-=> %s \033[m\n", msg);
		write(talkrec, buf, strlen(buf));
	} else if (who == 2) {	/* 别人说的话 */
		snprintf(buf, sizeof(buf), "\033[1;36m--> %s \033[m\n", msg);
		write(talkrec, buf, strlen(buf));
	}
}
#endif
