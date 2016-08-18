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

#ifdef MULTILINE_MESSAGE


/* Multiline Message Version by Pudding 04/04/01 */

/* 公用函数 */
extern int RMSG;
extern int msg_num;
extern int enable_arrow;

void count_msg(int signo);
char msg_buf[BUFLEN];

/* monster: added by cancel at 01/09/11 : 拒收群体信息, modified by me  */
int
can_friendwall(struct user_info *uin)
{
	struct userec msguser;
	int uid;

	if ((uid = searchuser(uin->userid)) == 0)
		return 0;
	if (get_record(PASSFILE, &msguser, sizeof(msguser), uid) != 0)
		return 0;
	if (!(msguser.userdefine & DEF_FRIENDWALL))
		return 0;

	return 1;
}


char
msgchar(struct user_info *uin)
{
	if (uin->mode == BBSNET || INBBSGAME(uin->mode))
		return '@';
	if (isreject(uin))
		return '*';
	if ((uin->pager & ALLMSG_PAGER))
		return ' ';
	if (hisfriend(uin))
		return (uin->pager & FRIENDMSG_PAGER) ? 'O' : '#';

	return '*';
}

int
canmsg(struct user_info *uin)
{
	if (HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_FORCEPAGE))
		return YEA;
	if (uin->invisible)
		return NA;
	if (!strcmp(uin->userid, "guest"))
		return NA;
	if (isreject(uin))
		return NA;
	if (uin->pager & ALLMSG_PAGER)
		return YEA;
	if ((uin->pager & FRIENDMSG_PAGER) && hisfriend(uin))
		return YEA;
	return NA;
}

int
s_msg(void)
{
	do_sendmsg(NULL, NULL, 0, 0);
	return 0;
}

int
dowall(struct user_info *uin)
{
	if (!uin->active || !uin->pid || uin->invisible)
		return -1;
	move(1, 0);
	clrtoeol();
	prints("\033[1;32m正对 %s 广播.... Ctrl-D 停止对此位 User 广播。\033[m", uin->userid);
	refresh();
	do_sendmsg(uin, msg_buf, 0, uin->pid);
	return 0;
}

int
myfriend_wall(struct user_info *uin)
{
	if (!can_friendwall(uin))
		return -1;
	if ((uin->pid - uinfo.pid == 0) || !uin->active || !uin->pid || isreject(uin) || uin->invisible)
		return -1;
	if (myfriend(uin->uid)) {
		move(1, 0);
		clrtoeol();
		prints("\033[1;32m正在送讯息给 %s...  \033[m", uin->userid);
		refresh();
		do_sendmsg(uin, msg_buf, 3, uin->pid);
	}
	return 0;
}

int
hisfriend_wall(struct user_info *uin)
{
	if (!can_friendwall(uin))
		return -1;
	if ((uin->pid - uinfo.pid == 0) || !uin->active || !uin->pid || isreject(uin) || uin->invisible)
		return -1;
	if (hisfriend(uin)) {
		move(1, 0);
		clrtoeol();
		prints("\033[1;32m正在送讯息给 %s...  \033[m", uin->userid);
		refresh();
		do_sendmsg(uin, msg_buf, 3, uin->pid);
	}
	return 0;
}

void
r_msg_sig(int signo)
{
	r_msg();
}

int
friend_login_wall(struct user_info *pageinfo)
{
	struct userec lookupuser;
	char msg[STRLEN];
	int x, y;

	if (!pageinfo->active || !pageinfo->pid || isreject(pageinfo))
		return 0;
	if (hisfriend(pageinfo)) {
		if (getuser(pageinfo->userid, &lookupuser) == 0)
			return 0;
		if (!(lookupuser.userdefine & DEF_LOGINFROM))
			return 0;
		if (pageinfo->uid == usernum)
			return 0;
		/* edwardc.990427 好友隐身就不显示送出上站通知 */
		if (pageinfo->invisible)
			return 0;
		/* monster: 若对方据收群体信息，则不对其送出上站通知 */
		if (!can_friendwall(pageinfo))
			return 0;
		getyx(&y, &x);
		if (y > 22) {
			pressanykey();
			move(7, 0);
			clrtobot();
		}
		prints("送出好友上站通知给 %s\n", pageinfo->userid);
		sprintf(msg, "你的好朋友 %s 已经上站啰！", currentuser.userid);
		do_sendmsg(pageinfo, msg, 2, pageinfo->pid);
	}
	return 0;
}


int
get_msg(char *uid, char *msg, int line)
{
	char genbuf[3];
	int inc = 0;

	move(line, 0);
	clrtoeol();
	refresh();
	prints("\033[0m送音信给：%s", uid);
	memset(msg, 0, sizeof (msg));
	msg[0] = '\0';
	while (1) {
		inc = multi_getdata(line + 1, 0, "音信 : ", msg, MSGLEN, 78, MSGLINE, NA);

      		if (msg[0] == '\0')
			return NA;
		getdata(line + inc + 1, 0,
			"确定要送出吗(Y)是的 (N)不要 (E)再编辑? [Y]: ", genbuf,
			2, DOECHO, YEA);
		if (genbuf[0] == 'e' || genbuf[0] == 'E') {
			move(line + inc + 1, 0);
			clrtoeol();
       			continue;
		}
		if (genbuf[0] == 'n' || genbuf[0] == 'N')
			return NA;
		else
			return YEA;
	}
}


int
show_allmsgs(void)
{
	char fname[PATH_MAX + 1];

	if (guestuser)
		return DONOTHING;

#ifdef LOG_MY_MESG
	setuserfile(fname, "allmsgfile");
#else  /* LOG_MY_MESG */
	return DONOTHING;			/* Pudding: 多行版本msgfile不能直接显示 */
#endif /* LOG_MY_MESG */

	if (dashf(fname)) {
		modify_user_mode(LOOKMSGS);
		mesgmore(fname);
	} else {
		clear();
		move(5, 30);
		prints("没有任何的讯息存在！！");
		pressanykey();
	}
	clear();
	return FULLUPDATE;
}

/* Pudding: 生成消息 */
#define MODE_MSG_SEND		0
#define MODE_MSG_LOG_SEND	1
#define MODE_MSG_LOG_RECV	2
#define MODE_MSG_LOG_WALL	3
#define MODE_MSG_SENDWALL	4
#define MODE_MSG_LOG_RECVWALL	5

/* make_msg */
char*
make_msg(char *buf, char *who, char *body, int upid, int mode)
{
	char timestr[20];
	char *timep;
	time_t now;
	struct tm *tm;
	char temp[MSGSIZE];
	char temp2[BUFLEN];
	
	memset(buf, 0, MSGSIZE);
	strlcpy(temp,body,MSGLEN); /* 杜绝缓冲区溢出 */

	/* 时钟相关 */
	now = time(NULL);
	timep = ctime(&now) + 11;
	tm = localtime(&now);
	sprintf(timestr, "%02d-%02d %02d:%02d", tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min);

	if (mode == MODE_MSG_SEND) {
		snprintf(buf, MSGSIZE,
			"\033[1;32m%-12.12s\033[33m(\033[36m%-5.5s\033[33m): \033[37m%s\033[m\033[%05dm",
			who, timep, temp, upid);
		return buf;
	}
	if (mode == MODE_MSG_SENDWALL) {
		snprintf(buf, MSGSIZE,
			"\033[1;36m%-12.12s\033[33m(\033[36m%-5.5s\033[33m): \033[37m%s\033[m\033[%05dm",
			who, timep, temp, upid);
		return buf;
	}

	/* 写进allmsgfile的部分 */
	if (mode == MODE_MSG_LOG_RECV) {
		sprintf(temp2,
			"\033[1;32mFrom %-12.12s\033[33m %-11s:\n\033[37m%s\033\[m\033[%05dm",
			who, timestr, temp, upid);
		/* 插入换行符 */
		make_multiline(buf, BUFLEN, temp2, 80, "\033[1;44m");
		return buf;
	}
       	if (mode == MODE_MSG_LOG_RECVWALL) {
		sprintf(temp2,
			"\033[1;36mFrom %-12.12s\033[33m %-11s:\n\033[37m%s\033\[m\033[%05dm",
			who, timestr, temp, upid);
		/* 插入换行符 */
		make_multiline(buf, BUFLEN, temp2, 80, "\033[1;45m");
		return buf;
	}

	if (mode == MODE_MSG_LOG_SEND) {
		sprintf(temp2,
			"\033[1;36mTo   %-12.12s\033[33m %-11s:\n\033[37m%s\033[m",
			who, timestr, temp);
		make_multiline(buf, BUFLEN, temp2, 80, "\033[1;46m");
		return buf;
	}
	if (MODE_MSG_LOG_WALL) {
		sprintf(temp2,
			"\033[1;36mTo   %-12.12s\033[33m %-11s:\n\033[37m%s\033[m",
			who, timestr, temp);
		make_multiline(buf, BUFLEN, temp2, 80, "\033[1;45m");
		return buf;
	}

	return NULL;
}

int
do_sendmsg(struct user_info *uentp, char *msgstr, int mode, int userpid)
{
	char uident[STRLEN];
	struct user_info *uin;
	char buf[BUFLEN];
	char fname[256];
	char msgbuf[BUFLEN];
	char msgbuf1[BUFLEN];
#ifdef LOG_MY_MESG
	char mymsg[BUFLEN];
	char buf3[STRLEN];
	int ishello = 0;
#endif
	if (mode == 0) {
		move(2, 0);
		clrtobot();
		if (uinfo.invisible && !HAS_PERM(PERM_SYSOP)) {
			move(2, 0);
			prints("抱歉, 此功能在隐身状态下不能执行...\n");
			pressreturn();
			return 0;
		}
		modify_user_mode(MSG);
	}
	/* 获得对方用户名 */
	if (uentp == NULL) {
		prints("<输入使用者代号>\n");
		move(1, 0);
		clrtoeol();
		prints("送讯息给: ");
		creat_list();
		namecomplete(NULL, uident);
		if (uident[0] == '\0') {
			clear();
			return 0;
		}
		uin = t_search(uident, NA);
		if (uin == NULL) {
			move(2, 0);
			prints("对方目前不在线上...\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
		if (uin->mode == BBSNET || INBBSGAME(uin->mode)
		    || uin->mode == LOCKSCREEN
		    || uin->mode == PAGE || uin->mode == LOGIN /* Pudding: 进站收讯息被Kick下站问题 */
		    || !canmsg(uin) || !strcmp(uident, "guest")) {
			move(2, 0);
			prints("目前无法传送讯息给对方.\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
	} else {
		if (uentp->uid == usernum) return 0;
		uin = uentp;
		if (uin->mode == BBSNET || INBBSGAME(uin->mode)
		    || uin->mode == PAGE || uin->mode == LOGIN /* Pudding: 进站收讯息被Kick下站问题 */
		    || uin->mode == LOCKSCREEN || !canmsg(uin))
			return 0;
		strcpy(uident, uin->userid);
	}
	/* 获取信息内容 */
	if (msgstr == NULL) {
		if (!get_msg(uident, buf, 1)) {
			move(1, 0);
			clrtoeol();
			move(2, 0);
			clrtoeol();
			return 0;
		}
       	} else strlcpy(buf, msgstr, BUFLEN);

	/* 生成信息 */
	char temp[BUFLEN];
	if (msgstr == NULL || mode == 2) {
		make_msg(msgbuf,
			 currentuser.userid, buf, uinfo.pid,
			 MODE_MSG_SEND);
		make_msg(msgbuf1,
			 currentuser.userid, buf, uinfo.pid,
			 MODE_MSG_LOG_RECV);
#ifdef LOG_MY_MESG
		make_msg(mymsg,
			 uin->userid, buf, uinfo.pid,
			 MODE_MSG_LOG_SEND);
		sprintf(buf3, "你的好朋友 %s 已经上站啰！", currentuser.userid);
		if (msgstr != NULL) {
			if (strcmp(buf, buf3) == 0) ishello = 1;
		}
#endif
	} else if (mode == 3) {
		make_msg(msgbuf,
			 currentuser.userid, buf, uinfo.pid,
			 MODE_MSG_SENDWALL);
		make_msg(msgbuf1,
			 currentuser.userid, buf, uinfo.pid,
			 MODE_MSG_LOG_RECVWALL);
	} else if (mode == 0) {
		sprintf(temp, "广播: %s", msgstr);
		make_msg(msgbuf,
			 "站长    于", temp, uinfo.pid,
			 MODE_MSG_SEND);
		make_msg(msgbuf1,
			 "站长", temp, uinfo.pid,
			 MODE_MSG_LOG_RECV);
	} else if (mode == 1) {
		sprintf(temp, "邀请你: %s", msgstr);
		make_msg(msgbuf,
			 currentuser.userid, temp, uinfo.pid,
			 MODE_MSG_SEND);
		make_msg(msgbuf1,
			 currentuser.userid, temp, uinfo.pid,
			 MODE_MSG_LOG_RECV);
	}

	if (userpid) {
		if (userpid != uin->pid) {
			saveline(0, 0);	/* Save line */
			move(0, 0);
			clrtoeol();
			prints("\033[1m对方已经离线...\033[m\n");
			sleep(1);
			saveline(0, 1);	/* restore line */
			return -1;
		}
	}
	if (!uin->active || kill(uin->pid, 0) == -1) {
		if (msgstr == NULL) {
			prints("\n对方已经离线...\n");
			pressreturn();
			clear();
		}
		return -1;
	}
	if (uin->mode == BBSNET || INBBSGAME(uin->mode)
	    || uin->mode == LOCKSCREEN
	    || uin->mode == PAGE || uin->mode == LOGIN /* Pudding: 进站收讯息被Kick下站问题 */
	    || !canmsg(uin) || !strcmp(uident, "guest")) {		
		prints("目前无法传送讯息给对方...\n");
		pressreturn();
		clear();
		return -1;
	}
	sethomefile(fname, uident, "msgfile");
	append_record(fname, msgbuf, MSGSIZE);	/* 干活 */
#ifdef LOG_MY_MESG
	if (mode == 2 || (mode == 0 && msgstr == NULL)) {
		if (!ishello) {
			sethomefile(fname, currentuser.userid, "allmsgfile");
			file_append(fname, mymsg);
		}
	}
	sethomefile(fname, uident, "allmsgfile");
	file_append(fname, msgbuf1); /* Here is a 纯文本文件 */
#endif
	if (uin->pid) {
		kill(uin->pid, SIGUSR2);
	}
	if (msgstr == NULL) {
		prints("\n已送出讯息...\n");
		pressreturn();
		clear();
	}
	return 1;
}

int
friend_wall(void)
{
	char buf[3];
	char msgbuf[BUFLEN], filename[80];

	if (uinfo.invisible) {
		move(2, 0);
		prints("抱歉, 此功能在隐身状态下不能执行...\n");
		pressreturn();
		return 0;
	}
	modify_user_mode(MSG);
	move(2, 0);
	clrtobot();
	getdata(1, 0, "送讯息给 [1] 我的好朋友，[2] 与我为友者: ",
		buf, 2, DOECHO, YEA);
	switch (buf[0]) {
	case '1':
		if (!get_msg("我的好朋友", msg_buf, 1))
			return 0;
		if (apply_ulist(myfriend_wall) == -1) {
			move(12, 0);
			prints("线上空无一人\n");
			pressanykey();
		} else {
			/* 记录送讯息给好友 */
			make_msg(msgbuf,"我的好友", msg_buf, uinfo.pid, MODE_MSG_LOG_WALL);
			setuserfile(filename, "allmsgfile");
			file_append(filename, msgbuf);
		}
		break;
	case '2':
		if (!get_msg("与我为友者", msg_buf, 1))
			return 0;
		if (apply_ulist(hisfriend_wall) == -1) {
			move(12, 0);
			prints("线上空无一人\n");
			pressanykey();
		} else {
			/* 记录送讯息给与我为友者 */
			make_msg(msgbuf,"与我为友者", msg_buf, uinfo.pid, MODE_MSG_LOG_WALL);
			setuserfile(filename, "allmsgfile");
			file_append(filename, msgbuf);
		}
		break;
	default:
		return 0;
	}
	move(8, 0);
	prints("讯息传送完毕...");
	pressanykey();
	return 1;
}

/* ------------------ 以下为界面相关部分 --------------- */

/* Pudding: 显示多行讯息 */
int
show_msg(char *msg)
{
	int x, y;
	int offset;
	int lines;
	char color[6];
	char ret_str[] = "\033[1;31m(^Z回)";
	char *ptr;
	int issysop;
	char tmp[BUFLEN];

	/* 确定是否站长 */
	issysop = 0;
	strlcpy(tmp, msg, BUFLEN);
	ptr = strtok(tmp + MSGHEADP, " \033[");
	if (ptr != NULL) {
		if (strncmp(ptr, "站长", 4) == 0) issysop = 1;
	}

        /* 确定是否群发讯息 */
	if (strncmp("\033[1;32m", msg, MSGHEADP) == 0) {
		strcpy(color, "\033[44m");
	}else strcpy(color, "\033[45m");

	getyx(&y, &x);
	move(0, 0);
	outs("\033[0m");
 	lines = show_multiline(0, 0, msg, t_realcols, color, &offset);
	outs("\033[0m");
	if (offset > 0) {
		prints(color);
		if (offset < t_realcols - 6 && !issysop) {
			for (; offset < t_realcols - 6; offset++) outc(' ');
			outs(ret_str);
		} else {
			for (; offset < t_realcols; offset++) outc(' ');
		}
	}
	outs("\033[0m");
	move(y, x);
	redoscr();
	return lines;
}

void
r_msg2(void)
{
	char buf[BUFLEN];
	char msg[BUFLEN];
	char fname[STRLEN];
	int tmpansi;
	int y, x, ch, i,j;
	int MsgNum;
	struct sigaction act;
	int msg_lines;
	int tmpmode;

	if (guestuser) return;
	setuserfile(fname, "msgfile");
	i = get_num_records(fname, MSGSIZE);
	if (i == 0) return;

	signal(SIGUSR2, count_msg);

        /* 保存屏幕状态here */
	getyx(&y, &x);
	int bottom = (t_lines < 24) ? t_lines : 24;
	struct screenline scr[25];
	for (j = 0; j < bottom; j++)
		saveline_buf(j, 0, scr + j);

	tmpansi = showansi;
	showansi = 1;
	oflush();

	tmpmode = uinfo.mode;
	modify_user_mode(MSG);
	MsgNum = 0;
	RMSG = YEA;

	while (1) {
		MsgNum = (MsgNum % i); /* 仅作回卷之用 */

		if (!dashf(fname)) {
			RMSG = NA;
			if (msg_num)
				r_msg();
			else {
				sigemptyset(&act.sa_mask);
				act.sa_flags = SA_NODEFER;
				act.sa_handler = r_msg_sig;
				sigaction(SIGUSR2, &act, NULL);
			}
			return;
		}
		get_record(fname, msg, MSGSIZE, i - MsgNum);
		msg[MSGSIZE] = '\0';

		msg_lines = show_msg(msg);
		move(msg_lines, 0);
		clrtoeol();
		prints("\033[0m---------------------------------------------------- 第%4d 条  共%4d 条 ------\033[m",
		       i - MsgNum, i);
		msg_lines++;
		refresh();
		
 		struct user_info *uin = NULL;
		int send_pid;
		char *ptr, usid[STRLEN];
		char comment[STRLEN];
		int send_ok = 0;

		if ((ptr = strrchr(msg, '[')) != NULL) {
			send_pid = atoi(ptr + 1);
			ptr = strtok(msg + MSGHEADP, " \033[");
			if (ptr != NULL/* && !strcmp(ptr, currentuser.userid) */) {
				strcpy(usid, ptr);
				uin = t_search(usid, send_pid);
			}
		}
		if (uin != NULL) {
			int userpid;
			userpid = uin->pid;
			char prompt[64];
			snprintf(prompt, 64, "回讯息给 %s: ", usid);

			msg_lines += multi_getdata(msg_lines, 0, prompt, buf, MSGLEN, 78, MSGLINE, YEA);

			if (buf[0] == Ctrl('Z')) {
				MsgNum++;
				goto rep_select;
			}
			if (buf[0] == Ctrl('A')) {
				MsgNum--;
				if (MsgNum < 0)
					MsgNum = i - 1;
				goto rep_select;
			}

			if (buf[0]  == '\0' || !isprint2(buf[0])) {
				sprintf(comment,
					"\033[0;1;33m空讯息, 所以不送出\033[m");
       			} else if (do_sendmsg(uin, buf, 2, userpid) == 1) {
				sprintf(comment,
					"\033[0;1;32m帮你送出讯息给 %s 了!\033[m", usid);
				send_ok = 1;
			} else {
				sprintf(comment,
					"\033[0;1;32m讯息无法送出.\033[m");
			}
			/* Show comment */
			move(msg_lines, 0);
			clrtoeol();
			refresh();
			outs(comment);
			if (!send_ok) {
				refresh();
				sleep(1);
			}
		} else {
			if (strncmp(usid, "站长", 4) != 0) {
				sprintf(comment,
					"\033[0;1;32m找不到发讯息的 %s! 请按上:[^Z↑] "
					"或下:[^A↓] 或其他键离开.\033[m",
					usid);
			}else {
				sprintf(comment,
					"\033[0;1;32m站长广播，不能回复! 请按上:[^Z↑] "
					"或下:[^A↓] 或其他键离开.\033[m");
			}
			/* Show comment */
			move(msg_lines, 0);
			clrtoeol();
			refresh();
			outs(comment);
			refresh();

			ch = igetkey();
			if ((ch == Ctrl('Z') || ch == KEY_UP)) {
				MsgNum++;
				goto rep_select;
			}
			if (ch == Ctrl('A') || ch == KEY_DOWN) {
				MsgNum--;
				if (MsgNum < 0)
					MsgNum = i - 1;
				goto rep_select;
			}
		}
		break;

		/* 按上下键选择的阶段 */
	rep_select:
		for (j = 0; j <= msg_lines; j++)
			saveline_buf(j, 1, scr + j);
		continue;
		
	}
	/* 恢复屏幕, 走人 */
	for (j = 0; j <= msg_lines; j++)
		saveline_buf(j, 1, scr + j);

       	showansi = tmpansi;
	move(y, x);
	redoscr();
	RMSG = NA;

	if (msg_num) {
		msg_num--;
		r_msg();
	} else {
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_NODEFER;
		act.sa_handler = r_msg_sig;
		sigaction(SIGUSR2, &act, NULL);
	}
	modify_user_mode(tmpmode);
	return;
}



extern int child_pid;

void
count_msg(int signo)
{
	signal(SIGUSR2, count_msg);
	msg_num++;
}

void
r_msg(void)
{
 	char buf[BUFLEN];
	char msg[BUFLEN];
	char fname[STRLEN];
	int tmpansi;
	int y, x, i, j, premsg = 0;
	char ch;
	int msg_lines;
	int tmpmode;
	struct sigaction act;

	signal(SIGUSR2, count_msg);
	msg_num++;				/* 来一条新讯息 */

	tmpmode = uinfo.mode;
	modify_user_mode(LOOKMSGS);

	/* 保存屏幕状态 */
        getyx(&y, &x);

	int bottom = (t_lines < 24) ? t_lines : 24;
	struct screenline scr[25];
	for (i = 0; i < bottom; i++)
		saveline_buf(i, 0, scr + i);

	tmpansi = showansi;
	showansi = 1;
	oflush();
	
       	if (DEFINE(DEF_MSGGETKEY)) {
		premsg = RMSG;
	}
	
	while (msg_num) {
		setuserfile(fname, "msgfile");

		i = get_num_records(fname, MSGSIZE);
		if (i == 0) {
			sigemptyset(&act.sa_mask);
			act.sa_flags = SA_NODEFER;
			act.sa_handler = r_msg_sig;
			sigaction(SIGUSR2, &act, NULL);
			modify_user_mode(tmpmode);
			return;
		}
		get_record(fname, msg, MSGSIZE, i - msg_num + 1);
		msg[MSGSIZE] = '\0';

		/* Show message here */
		msg_lines = show_msg(msg);
		if (msg_lines > 1) {
			move(msg_lines, 0);
			clrtoeol();
			prints("\033[0m-------------------------------------------------- %4d 条未读  共%4d 条 ------\033[m",
			       msg_num - 1 , i);
			refresh();
		}
		if (DEFINE(DEF_SOUNDMSG)) bell();

		msg_num--;

		if (DEFINE(DEF_MSGGETKEY)) {
			RMSG = YEA;
			ch = 0;
			while (1) {
				ch = igetkey();
#ifdef MSG_CANCLE_BY_CTRL_C
				if (ch == Ctrl('C')) break;
#else
				if (ch == '\r' || ch == '\n') break;
#endif
				if (ch != Ctrl('R') && ch != 'R'
				    && ch != 'r' && ch != Ctrl('Z')) continue;

				modify_user_mode(MSG);
				struct user_info *uin = NULL;
				int send_pid;
				char *ptr, usid[STRLEN];
				char comment[STRLEN];
				int send_ok = 0;

				if ((ptr = strrchr(msg, '[')) != NULL) {
					send_pid = atoi(ptr + 1);
					ptr = strtok(msg + MSGHEADP, " \033[");
					if (ptr != NULL/* && !strcmp(ptr, currentuser.userid) */) {
						strcpy(usid, ptr);
						uin = t_search(usid, send_pid);
					}
				}

				if (uin != NULL) {
					int userpid;
					userpid = uin->pid;
					char prompt[64];

					/* 回讯息界面 */
					snprintf(prompt, 64, "立即回讯息给 %s : ", usid);
					msg_lines += multi_getdata(msg_lines, 0, prompt, buf, MSGLEN, 78, MSGLINE, YEA);
					
					if (buf[0] == '\0' || !isprint2(buf[0])) {
						if (buf[0] == Ctrl('A') || buf[0] == Ctrl('Z')) {
							sprintf(comment,
								"\033[0;1;33m动作已取消\033[m");
						} else {
							sprintf(comment,
								"\033[0;1;33m空讯息, 所以不送出\033[m");
						}
       					} else if (do_sendmsg(uin, buf, 2, userpid) == 1) {
						sprintf(comment,
							"\033[0;1;32m帮你送出讯息给 %s 了!\033[m", usid);
						send_ok = 1;
					} else {
						sprintf(comment,
							"\033[0;1;32m讯息无法送出.\033[m");
					}
				} else {
					if (strncmp(usid, "站长", 4) == 0)
						sprintf(comment,
							"\033[0;1;32m站长广播，不能回复!\033[m");
					else
						sprintf(comment,
							"\033[0;1;32m找不到发讯息的 %s.\033[m", usid);
				}

				/* output comment */
				move(msg_lines, 0);
				clrtoeol();
				refresh();
				outs(comment);
				if (!send_ok) {
					refresh();
					sleep(1);
				}
				break;
			} /* main key loop */
			modify_user_mode(LOOKMSGS); /* 精确点好 */
		}
		/* 恢复屏幕 */
		for (j = 0; j <= msg_lines; j++)
			saveline_buf(j, 1, scr + j);
		move(y, x);
		refresh();
      	}
	if (DEFINE(DEF_MSGGETKEY)) {
		RMSG = premsg;
	}

       	move(y, x);
	redoscr();

	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
	act.sa_handler = r_msg_sig;
	sigaction(SIGUSR2, &act, NULL);

	modify_user_mode(tmpmode);
}



#else


/* ------------------------------------------------------------------- */
/*                                                                     */
/*     sendmsg.c 单行版本. 为了CVS从上传不conflict 只好这样做了:(      */
/*                                                                     */
/* ------------------------------------------------------------------- */
/* Single-Line Message Version */

extern int RMSG;
extern int msg_num;

void count_msg(int signo);
char msg_buf[STRLEN];

/* monster: added by cancel at 01/09/11 : 拒收群体信息, modified by me  */
int
can_friendwall(struct user_info *uin)
{
	struct userec msguser;
	int uid;

	if ((uid = searchuser(uin->userid)) == 0)
		return 0;
	if (get_record(PASSFILE, &msguser, sizeof(msguser), uid) != 0)
		return 0;
	if (!(msguser.userdefine & DEF_FRIENDWALL))
		return 0;

	return 1;
}

int
get_msg(char *uid, char *msg, int line)
{
	char genbuf[3];

	move(line, 0);
	clrtoeol();
	prints("送音信给：%s", uid);
	memset(msg, 0, sizeof (msg));
	while (1) {
		getdata(line + 1, 0, "音信 : ", msg, 55, DOECHO, NA);
		if (msg[0] == '\0')
			return NA;
		getdata(line + 2, 0,
			"确定要送出吗(Y)是的 (N)不要 (E)再编辑? [Y]: ", genbuf,
			2, DOECHO, YEA);
		if (genbuf[0] == 'e' || genbuf[0] == 'E')
			continue;
		if (genbuf[0] == 'n' || genbuf[0] == 'N')
			return NA;
		else
			return YEA;
	}
}

char
msgchar(struct user_info *uin)
{
	if (uin->mode == BBSNET || INBBSGAME(uin->mode))
		return '@';
	if (isreject(uin))
		return '*';
	if ((uin->pager & ALLMSG_PAGER))
		return ' ';
	if (hisfriend(uin))
		return (uin->pager & FRIENDMSG_PAGER) ? 'O' : '#';

	return '*';
}

int
canmsg(struct user_info *uin)
{
	if (HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_FORCEPAGE))
		return YEA;
	if (uin->invisible)
		return NA;
	if (!strcmp(uin->userid, "guest"))
		return NA;
	if (isreject(uin))
		return NA;
	if (uin->pager & ALLMSG_PAGER)
		return YEA;
	if ((uin->pager & FRIENDMSG_PAGER) && hisfriend(uin))
		return YEA;
	return NA;
}

int
s_msg(void)
{
	do_sendmsg(NULL, NULL, 0, 0);
	return 0;
}

int
show_allmsgs(void)
{
	char fname[PATH_MAX + 1];

	if (guestuser)
		return DONOTHING;

#ifdef LOG_MY_MESG
	setuserfile(fname, "allmsgfile");
#else
	setuserfile(fname, "msgfile");
#endif

	if (dashf(fname)) {
		modify_user_mode(LOOKMSGS);
		mesgmore(fname);
	} else {
		clear();
		move(5, 30);
		prints("没有任何的讯息存在！！");
		pressanykey();
	}
	clear();

	return FULLUPDATE;
}

int
do_sendmsg(struct user_info *uentp, char *msgstr, int mode, int userpid)
{
	char uident[STRLEN], ret_str[20];
	time_t now;
	struct tm *tm;
	struct user_info *uin;
	char buf[80], *msgbuf, *msgbuf1, *timestr, *timestr1;

#ifdef LOG_MY_MESG
	char *mymsg, buf3[80];
	int ishelo = 0;		/* 是不是好友上站通知讯息 */

	mymsg = (char *) malloc(256);
#endif
	msgbuf = (char *) malloc(256);
	msgbuf1 = (char *) malloc(256);
	timestr1 = (char *) malloc(80);

	if (mode == 0) {
		move(2, 0);
		clrtobot();
		if (uinfo.invisible && !HAS_PERM(PERM_SYSOP)) {
			move(2, 0);
			prints("抱歉, 此功能在隐身状态下不能执行...\n");
			pressreturn();
			return 0;
		}
		modify_user_mode(MSG);
	}
	if (uentp == NULL) {
		prints("<输入使用者代号>\n");
		move(1, 0);
		clrtoeol();
		prints("送讯息给: ");
		creat_list();
		namecomplete(NULL, uident);
		if (uident[0] == '\0') {
			clear();
			return 0;
		}
		uin = t_search(uident, NA);
		if (uin == NULL) {
			move(2, 0);
			prints("对方目前不在线上...\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
		if (uin->mode == BBSNET || INBBSGAME(uin->mode)
		    || uin->mode == LOCKSCREEN
		    || uin->mode == PAGE
		    || !canmsg(uin) || !strcmp(uident, "guest")) {
			move(2, 0);
			prints("目前无法传送讯息给对方.\n");
			pressreturn();
			move(2, 0);
			clrtoeol();
			return -1;
		}
	} else {
		if (uentp->uid == usernum)
			return 0;
		uin = uentp;
		if (uin->mode == BBSNET || INBBSGAME(uin->mode)
		    || uin->mode == PAGE
		    || uin->mode == LOCKSCREEN || !canmsg(uin))
			return 0;
		strcpy(uident, uin->userid);
	}
	if (msgstr == NULL) {
		if (!get_msg(uident, buf, 1)) {
			move(1, 0);
			clrtoeol();
			move(2, 0);
			clrtoeol();
			return 0;
		}
	}
	now = time(NULL);
	timestr = ctime(&now) + 11;
	*(timestr + 8) = '\0';
//cancel
	tm = localtime(&now);
	sprintf(timestr1, "%02d-%02d %02d:%02d", tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min);
	strcpy(ret_str, "^Z回");
	if (msgstr == NULL || mode == 2) {
		sprintf(msgbuf,
			"\033[0;1;44;36m%-12.12s\033[33m(\033[36m%-5.5s\033[33m):\033[37m%-54.54s\033[31m(%s)\033[m\033[%05dm\n",
			currentuser.userid, timestr,
			(msgstr == NULL) ? buf : msgstr, ret_str, uinfo.pid);
		sprintf(msgbuf1,
			"\033[0;1;44;36m%-12.12s\033[33m %-11s  \033[37m%-48.48s\033[31m(%s)\033[m\033[%05dm\n",
			currentuser.userid, timestr1,
			(msgstr == NULL) ? buf : msgstr, ret_str, uinfo.pid);
#ifdef LOG_MY_MESG
		sprintf(mymsg,
			"\033[1;32;40mTo \033[1;33;40m%-12.12s\033[m %-11s %-50.50s\n",
			uin->userid, timestr1, (msgstr == NULL) ? buf : msgstr);
		sprintf(buf3, "你的好朋友 %s 已经上站啰！", currentuser.userid);

		if (msgstr != NULL) {
			if (strcmp(msgstr, buf3) == 0) {
				ishelo = 1;
			} else if (strcmp(buf, buf3) == 0) {
				ishelo = 1;
			}
		}
#endif
	} else if (mode == 0) {
		sprintf(msgbuf,
			"\033[0;1;5;44;33m站长 于\033[36m %8.8s \033[33m广播：\033[m\033[1;37;44m%-57.57s\033[m\033[%05dm\n",
			timestr, msgstr, uinfo.pid);
		sprintf(msgbuf1,
			"\033[0;1;5;44;33m站长 于\033[36m %-11s \033[33m广播：\033[m\033[1;37;44m%-50.50s\033[m\033[%05dm\n",
			timestr1, msgstr, uinfo.pid);
	} else if (mode == 1) {
		sprintf(msgbuf,
			"\033[0;1;44;36m%-12.12s\033[37m(\033[36m%-5.5s\033[37m) 邀请你\033[37m%-48.48s\033[31m(%s)\033[m\033[%05dm\n",
			currentuser.userid, timestr, msgstr, ret_str,
			uinfo.pid);
		sprintf(msgbuf1,
			"\033[0;1;44;36m%-12.12s\033[37m \033[36m%-11s\033[37m 邀请你\033[37m%-41.41s\033[31m(%s)\033[m\033[%05dm\n",
			currentuser.userid, timestr1, msgstr, ret_str,
			uinfo.pid);
	} else if (mode == 3) {
		sprintf(msgbuf,
			"\033[0;1;45;36m%-12.12s\033[33m(\033[36m%-5.5s\033[33m):\033[37m%-54.54s\033[31m(%s)\033[m\033[%05dm\n",
			currentuser.userid, timestr,
			(msgstr == NULL) ? buf : msgstr, ret_str, uinfo.pid);
		sprintf(msgbuf1,
			"\033[0;1;45;36m%-12.12s\033[33m(\033[36m%-5.5s\033[33m):\033[37m%-48.48s\033[31m(%s)\033[m\033[%05dm\n",
			currentuser.userid, timestr1,
			(msgstr == NULL) ? buf : msgstr, ret_str, uinfo.pid);
	}
	if (userpid) {
		if (userpid != uin->pid) {
			saveline(0, 0);	/* Save line */
			move(0, 0);
			clrtoeol();
			prints("\033[1m对方已经离线...\033[m\n");
			sleep(1);
			saveline(0, 1);	/* restore line */
			return -1;
		}
	}
	if (!uin->active || kill(uin->pid, 0) == -1) {
		if (msgstr == NULL) {
			prints("\n对方已经离线...\n");
			pressreturn();
			clear();
		}
		return -1;
	}
	sethomefile(buf, uident, "msgfile");
	file_append(buf, msgbuf);

#ifdef LOG_MY_MESG
	/*
	 * 990610.edwardc 除了我直接送讯息给某人外, 其他如广拨给站上
	 * 拨好友一律不纪录 .. :)
	 */
	if (mode == 2 || (mode == 0 && msgstr == NULL)) {
		if (ishelo == 0) {
			sethomefile(buf, currentuser.userid, "allmsgfile");
			file_append(buf, mymsg);
		}
	}
	sethomefile(buf, uident, "allmsgfile");
	file_append(buf, msgbuf1);
	free(mymsg);

#endif
	free(msgbuf);
	free(msgbuf1);
	free(timestr1);
	if (uin->pid) {
		kill(uin->pid, SIGUSR2);
	}
	if (msgstr == NULL) {
		prints("\n已送出讯息...\n");
		pressreturn();
		clear();
	}
	return 1;
}

int
dowall(struct user_info *uin)
{
	if (!uin->active || !uin->pid || uin->invisible)
		return -1;
	move(1, 0);
	clrtoeol();
	prints("\033[1;32m正对 %s 广播.... Ctrl-D 停止对此位 User 广播。\033[m", uin->userid);
	refresh();
	do_sendmsg(uin, msg_buf, 0, uin->pid);
	return 0;
}

int
myfriend_wall(struct user_info *uin)
{
	if (!can_friendwall(uin))
		return -1;
	if ((uin->pid - uinfo.pid == 0) || !uin->active || !uin->pid || isreject(uin) || uin->invisible)
		return -1;
	if (myfriend(uin->uid)) {
		move(1, 0);
		clrtoeol();
		prints("\033[1;32m正在送讯息给 %s...  \033[m", uin->userid);
		refresh();
		do_sendmsg(uin, msg_buf, 3, uin->pid);
	}
	return 0;
}

int
hisfriend_wall(struct user_info *uin)
{
	if (!can_friendwall(uin))
		return -1;
	if ((uin->pid - uinfo.pid == 0) || !uin->active || !uin->pid || isreject(uin) || uin->invisible)
		return -1;
	if (hisfriend(uin)) {
		move(1, 0);
		clrtoeol();
		prints("\033[1;32m正在送讯息给 %s...  \033[m", uin->userid);
		refresh();
		do_sendmsg(uin, msg_buf, 3, uin->pid);
	}
	return 0;
}

int
friend_wall(void)
{
	char buf[3];
	char msgbuf[256], filename[80];
	time_t now;
	char *timestr;

	now = time(NULL);
	timestr = ctime(&now) + 11;
	*(timestr + 8) = '\0';

	if (uinfo.invisible) {
		move(2, 0);
		prints("抱歉, 此功能在隐身状态下不能执行...\n");
		pressreturn();
		return 0;
	}
	modify_user_mode(MSG);
	move(2, 0);
	clrtobot();
	getdata(1, 0, "送讯息给 [1] 我的好朋友，[2] 与我为友者: ",
		buf, 2, DOECHO, YEA);
	switch (buf[0]) {
	case '1':
		if (!get_msg("我的好朋友", msg_buf, 1))
			return 0;
		if (apply_ulist(myfriend_wall) == -1) {
			move(2, 0);
			prints("线上空无一人\n");
			pressanykey();
		} else {
			/* 记录送讯息给好友 */
			sprintf(msgbuf,
				"\033[0;1;45;36m送讯息给好友\033[33m(\033[36m%-5.5s\033[33m):\033[37m%-54.54s\033[31m(^Z回)\033[m\033[%05dm\n",
				timestr, msg_buf, uinfo.pid);
			setuserfile(filename, "allmsgfile");
			file_append(filename, msgbuf);
		}
		break;
	case '2':
		if (!get_msg("与我为友者", msg_buf, 1))
			return 0;
		if (apply_ulist(hisfriend_wall) == -1) {
			move(2, 0);
			prints("线上空无一人\n");
			pressanykey();
		} else {
			/* 记录送讯息给与我为友者 */
			sprintf(msgbuf,
				"\033[0;1;45;36m送给与我为友\033[33m(\033[36m%-5.5s\033[33m):\033[37m%-54.54s\033[31m(^Z回)\033[m\033[%05dm\n",
				timestr, msg_buf, uinfo.pid);
			setuserfile(filename, "allmsgfile");
			file_append(filename, msgbuf);

		}
		break;
	default:
		return 0;
	}
	move(6, 0);
	prints("讯息传送完毕...");
	pressanykey();
	return 1;
}

void
r_msg2(void)
{
	FILE *fp;
	char buf[256];
	char msg[256];
	char fname[STRLEN];
	int line, tmpansi;
	int y, x, ch, i, j;
	int MsgNum;
	struct sigaction act;

	if (guestuser)
		return;
	getyx(&y, &x);
	if (uinfo.mode == TALK)
		line = t_lines / 2 - 1;
	else
		line = 0;
	setuserfile(fname, "msgfile");
	i = get_num_records(fname, 129);
	if (i == 0)
		return;
	signal(SIGUSR2, count_msg);
	tmpansi = showansi;
	showansi = 1;
	oflush();
	if (RMSG == NA) {
		saveline(line, 4);
		saveline(line + 1, 6);
	}
	MsgNum = 0;
	RMSG = YEA;
	while (1) {
		MsgNum = (MsgNum % i);
		if ((fp = fopen(fname, "r")) == NULL) {
			RMSG = NA;
			if (msg_num)
				r_msg();
			else {
				sigemptyset(&act.sa_mask);
				act.sa_flags = SA_NODEFER;
				act.sa_handler = r_msg_sig;
				sigaction(SIGUSR2, &act, NULL);
			}
			return;
		}
		for (j = 0; j < (i - MsgNum); j++) {
			if (fgets(buf, 256, fp) == NULL)
				break;
			else
				strcpy(msg, buf);
		}
		fclose(fp);
		move(line, 0);
		clrtoeol();
		outs(msg);
		refresh();
		{
			struct user_info *uin = NULL;
			char msgbuf[STRLEN];
			int send_pid;
			char *ptr, usid[STRLEN];

			if ((ptr = strrchr(msg, '[')) != NULL) {
				send_pid = atoi(ptr + 1);
				ptr = strtok(msg + 12, " \033[");
				if (ptr != NULL/* && strcmp(ptr, currentuser.userid) */) {
					strcpy(usid, ptr);
					uin = t_search(usid, send_pid);
				}
			}

			if (uin != NULL && canmsg(uin)) {
				int userpid;

				userpid = uin->pid;
				move(line + 1, 0);
				clrtoeol();
				sprintf(msgbuf, "回讯息给 %s: ", usid);
				getdata(line + 1, 0, msgbuf, buf, 55, DOECHO,
					YEA);
				if (buf[0] == Ctrl('Z')) {
					MsgNum++;
					continue;
				} else if (buf[0] == Ctrl('A')) {
					MsgNum--;
					if (MsgNum < 0)
						MsgNum = i - 1;
					continue;
				}
				if (buf[0] != '\0') {
					if (do_sendmsg(uin, buf, 2, userpid) ==
					    1)
						sprintf(msgbuf,
							"\033[1;32m帮你送出讯息给 %s 了!\033[m",
							usid);
					else
						sprintf(msgbuf,
							"\033[1;32m讯息无法送出.\033[m");
				} else
					sprintf(msgbuf,
						"\033[1;33m空讯息, 所以不送出.\033[m");
				move(line + 1, 0);
				clrtoeol();
				refresh();
				outs(msgbuf);
				refresh();
				if (!strstr(msgbuf, "帮你"))
					sleep(1);
			} else {
				if (!isdigit(usid[0])) {
					sprintf(msgbuf,
						"\033[1;32m找不到发讯息的 %s! 请按上:[^Z ↑] "
						"或下:[^A ↓] 或其他键离开.\033[m",
						usid);
				} else {
					sprintf(msgbuf,
						"\033[1;32m站长广播，不能回复! 请按上:[^Z ↑] "
						"或下:[^A ↓] 或其他键离开.\033[m");
				}
				move(line + 1, 0);
				clrtoeol();
				refresh();
				outs(msgbuf);
				refresh();
				if ((ch = igetkey()) == Ctrl('Z') ||
				    ch == KEY_UP) {
					MsgNum++;
					continue;
				}
				if (ch == Ctrl('A') || ch == KEY_DOWN) {
					MsgNum--;
					if (MsgNum < 0)
						MsgNum = i - 1;
					continue;
				}
			}
		}
		break;
	}
	saveline(line, 5);
	saveline(line + 1, 7);
	showansi = tmpansi;
	move(y, x);
	refresh();
	RMSG = NA;
	if (msg_num) {
		msg_num--;
		r_msg();
	} else {
		sigemptyset(&act.sa_mask);
		act.sa_flags = SA_NODEFER;
		act.sa_handler = r_msg_sig;
		sigaction(SIGUSR2, &act, NULL);
	}
	return;
}

extern int child_pid;

void
count_msg(int signo)
{
	signal(SIGUSR2, count_msg);
	msg_num++;
}

void
r_msg(void)
{
	FILE *fp;
	char buf[256];
	char msg[256];
	char fname[STRLEN];
	int line, tmpansi;
	int y, x, i, j, premsg = 0;
	char ch;
	struct sigaction act;

	signal(SIGUSR2, count_msg);
	msg_num++;
	getyx(&y, &x);
	tmpansi = showansi;
	showansi = 1;
	if (uinfo.mode == TALK)
		line = t_lines / 2 - 1;
	else
		line = 0;
	if (DEFINE(DEF_MSGGETKEY)) {
		oflush();
		saveline(line, 0);
		premsg = RMSG;
	}
	while (msg_num) {
		if (DEFINE(DEF_SOUNDMSG)) {
			bell();
		}
		setuserfile(fname, "msgfile");
		i = get_num_records(fname, 129);
		if ((fp = fopen(fname, "r")) == NULL) {
			sigemptyset(&act.sa_mask);
			act.sa_flags = SA_NODEFER;
			act.sa_handler = r_msg_sig;
			sigaction(SIGUSR2, &act, NULL);
			return;
		}
		for (j = 0; j <= (i - msg_num); j++) {
			if (fgets(buf, 256, fp) == NULL)
				break;
			else
				strcpy(msg, buf);
		}
		fclose(fp);
		move(line, 0);
		clrtoeol();
		outs(msg);
		refresh();
		msg_num--;
		if (DEFINE(DEF_MSGGETKEY)) {
			RMSG = YEA;
			ch = 0;
#ifdef MSG_CANCLE_BY_CTRL_C
			while (ch != Ctrl('C'))
#else
			while (ch != '\r' && ch != '\n')
#endif
			{
				ch = igetkey();
#ifdef MSG_CANCLE_BY_CTRL_C
				if (ch == Ctrl('C'))
					break;
#else
				if (ch == '\r' || ch == '\n')
					break;
#endif
				else if (ch == Ctrl('R') || ch == 'R' || ch == 'r' || ch == Ctrl('Z')) {
					struct user_info *uin = NULL;
					char msgbuf[STRLEN];
					int send_pid;
					char *ptr, usid[STRLEN];

					if ((ptr = strrchr(msg, '[')) != NULL) {
						send_pid = atoi(ptr + 1);
						ptr = strtok(msg + 12, " \033[");
						if (ptr != NULL/* && !strcmp(ptr, currentuser.userid) */) {
							strcpy(usid, ptr);
							uin = t_search(usid, send_pid);
						}
					}

					oflush();
					saveline(line + 1, 2);
					if (uin != NULL) {
						int userpid;

						userpid = uin->pid;
						move(line + 1, 0);
						clrtoeol();
						sprintf(msgbuf,
							"立即回讯息给 %s: ",
							usid);
						getdata(line + 1, 0, msgbuf,
							buf, 55, DOECHO, YEA);
						if (buf[0] != '\0' &&
						    buf[0] != Ctrl('Z') &&
						    buf[0] != Ctrl('A')) {
							if (do_sendmsg
							    (uin, buf, 2,
							     userpid))
								sprintf(msgbuf,
									"\033[1;32m帮你送出讯息给 %s 了!\033[m",
									usid);
							else
								sprintf(msgbuf,
									"\033[1;32m讯息无法送出.\033[m");
						} else
							sprintf(msgbuf,
								"\033[1;33m空讯息, 所以不送出. \033[m");
					} else {
						sprintf(msgbuf,
							"\033[1;32m找不到发讯息的 %s.\033[m",
							usid);
					}
					move(line + 1, 0);
					clrtoeol();
					refresh();
					outs(msgbuf);
					refresh();
					if (!strstr(msgbuf, "帮你"))
						sleep(1);
					saveline(line + 1, 3);
					refresh();
					break;
				}	/* if */
			}	/* while */
		}		/* if */
	}			/* while */
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_NODEFER;
	act.sa_handler = r_msg_sig;
	sigaction(SIGUSR2, &act, NULL);
	if (DEFINE(DEF_MSGGETKEY)) {
		RMSG = premsg;
		saveline(line, 1);
	}
	showansi = tmpansi;
	move(y, x);
	refresh();
	return;
}

void
r_msg_sig(int signo)
{
	r_msg();
}

int
friend_login_wall(struct user_info *pageinfo)
{
	struct userec lookupuser;
	char msg[STRLEN];
	int x, y;

	if (!pageinfo->active || !pageinfo->pid || isreject(pageinfo))
		return 0;
	if (hisfriend(pageinfo)) {
		if (getuser(pageinfo->userid, &lookupuser) == 0)
			return 0;
		if (!(lookupuser.userdefine & DEF_LOGINFROM))
			return 0;
		if (pageinfo->uid == usernum)
			return 0;
		/* edwardc.990427 好友隐身就不显示送出上站通知 */
		if (pageinfo->invisible)
			return 0;
		/* monster: 若对方据收群体信息，则不对其送出上站通知 */
		if (!can_friendwall(pageinfo))
			return 0;
		getyx(&y, &x);
		if (y > 22) {
			pressanykey();
			move(7, 0);
			clrtobot();
		}
		prints("送出好友上站通知给 %s\n", pageinfo->userid);
		sprintf(msg, "你的好朋友 %s 已经上站啰！", currentuser.userid);
		do_sendmsg(pageinfo, msg, 2, pageinfo->pid);
	}
	return 0;
}

#endif
