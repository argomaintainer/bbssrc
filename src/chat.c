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

#define CHAT

#include "bbs.h"
#include "chat.h"

char chatname[IDLEN];		/* Chat-Room Name */
int chatline;			/* Where to display message now */
int stop_line;			/* next line of bottom of message window area */
FILE *rec;
int recflag = 0;
char buftopic[STRLEN];
char chat_station[19];

extern char page_requestor[];

#define b_lines t_lines-1
#define cuser currentuser
char *msg_seperator = "\
———————————————————————————————————————";
char *msg_shortulist = "\033[1;33;44m\
 使用者代号    目前状态  │ 使用者代号    目前状态  │ 使用者代号    目前状态 \033[m";

struct chat_command {
	char *cmdname;		/* Char-room command length */
	void (*cmdfunc) ();	/* Pointer to function */
};

/* KCN alias emote , 1999.11.25 */
struct chatalias {
	char cmd[9];
	char action[81];
};

struct chatalias *chat_aliases;
int chat_alias_count;

void
chat_load_alias(void)
{
	char buf[256];
	int i;

	chat_alias_count = 0;
	chat_aliases =
	    (struct chatalias *) malloc(sizeof (struct chatalias) *
					MAXDEFINEALIAS);
	for (i = 0; i < MAXDEFINEALIAS; i++)
		chat_aliases[i].cmd[0] = 0;
	setuserfile(buf, "chatalias");
	chat_alias_count = get_num_records(buf, sizeof (struct chatalias));
	if (chat_alias_count > MAXDEFINEALIAS)
		chat_alias_count = MAXDEFINEALIAS;
	if (chat_alias_count != 0)
		get_records(buf, chat_aliases, sizeof (struct chatalias), 1,
			    chat_alias_count);
	for (i = 0; i < chat_alias_count; i++) {
		if (chat_aliases[i].cmd[0] == 0) {
			chat_alias_count = i;
			break;
		}
		chat_aliases[i].cmd[8] = 0;
		chat_aliases[i].action[80] = 0;
	}
}

void
set_rec(void)
{
	char fname[PATH_MAX + 1], tmptopic[256];
	time_t now;

	now = time(NULL);

	snprintf(fname, sizeof(fname), "tmp/chat.%s.%d", currentuser.userid, getpid());	/* monster: 避免多窗口录音缺失问题 */
	if (recflag == 0) {
		if ((rec = fopen(fname, "w")) == NULL)
			return;

		printchatline("\033[1;5;32m开始录音...\033[m");
		recflag = 1;
		clear_line(0);
		transPERstr(buftopic, tmptopic);
		prints("\033[1;44;33m 房间： \033[32m%-12.12s  \033[5;31;44m%6.6s\033[0;1;44;33m话题：\033[36m%-49.49s\033[m",
			chatname, (recflag == 1) ? "[录音]" : "      ", tmptopic);
		fprintf(rec, "本段由 %s", currentuser.userid);
		getdatestring(now);
		fprintf(rec, "所录下，开始时间： %s\n", datestring);
	} else {
		recflag = 0;
		clear_line(0);
		transPERstr(buftopic, tmptopic);
		prints("\033[1;44;33m 房间： \033[32m%-12.12s  \033[5;31;44m%6.6s\033[0;1;44;33m话题：\033[36m%-49.49s\033[m",
			chatname, (recflag == 1) ? "[录音]" : "      ", tmptopic);
		printchatline("\033[1;5;32m录音结束...\033[m");
		getdatestring(now);
		fprintf(rec, "结束时间：%s\n\n", datestring);
		fclose(rec);
		mail_sysfile(fname, currentuser.userid, "录音结果");
//		postfile(fname, "syssecurity", "录音结果", 2);
		unlink(fname);
	}
}

void
setpager(void)
{
	char buf[STRLEN];

	t_pager();
	snprintf(buf, sizeof(buf), "\033[1;32m◆ \033[31m呼叫器 %s 了\033[m",
		(uinfo.pager & ALL_PAGER) ? "打开" : "关闭");
	printchatline(buf);
}


/* KCN alias emote , 1999.11.25 */
void
transPERstr(char *str, char *tmpstr)
{
	char *p;
	int i;

	p = str;
	i = 0;
	memset(tmpstr, 0, strlen(tmpstr));
	while (*p != 0) {
		if (*p == '%') {
			if (*(p + 1) == 0) {
				tmpstr[i] = '%';
				i++;
				p++;
			} else if (*(p + 1) == '%') {
				tmpstr[i] = '%';
				i++;
				p++;
				p++;
			} else if (*(p + 1) > '0' && *(p + 1) <= '7') {
				tmpstr[i++] = 27;
				tmpstr[i++] = '[';
				tmpstr[i++] = '3';
				tmpstr[i++] = *(p + 1);
				tmpstr[i++] = 'm';
				p++;
				p++;
			} else if (*(p + 1) == '0') {
				tmpstr[i++] = 27;
				tmpstr[i++] = '[';
				tmpstr[i++] = '0';
				tmpstr[i++] = 'm';
				p++;
				p++;
			} else {
				tmpstr[i] = '%';
				i++;
				p++;
				tmpstr[i] = *p;
				i++;
				p++;
			}
		} else {
			tmpstr[i] = *p;
			i++;
			p++;
		}
	}
	tmpstr[i++] = 27;
	tmpstr[i++] = '[';
	tmpstr[i++] = '3';
	tmpstr[i++] = '7';
	tmpstr[i++] = 'm';
	tmpstr[i] = 0;
}

void
printchatline(char *str)
{
	char tmpstr[256];

	transPERstr(str, tmpstr);
	clear_line(chatline);
	outs(tmpstr);
	outc('\n');
	if (recflag == YEA)
		fprintf(rec, "%s\n", str);
	if (++chatline == stop_line)
		chatline = 2;
	clear_line(chatline);
	outs("→");
}

void
printchatnewline()
{
	if (recflag == YEA)
		fputc('\n', rec);
	clear_line(chatline);
	if (++chatline == stop_line)
		chatline = 2;
	clear_line(chatline);
	outs("→");
}

void
chat_clear(void)
{
	for (chatline = 2; chatline < stop_line; chatline++)
		clear_line(chatline);
	chatline = stop_line - 1;
	printchatnewline();
}

void
print_chatid(char *chatid)
{
	int i;

	move(b_lines, 0);
	for (i = 0; chatid[i] && i < 8; i++)
		outc(chatid[i]);
	outc(':');
}

int
chat_send(int fd, char *buf)
{
	int len;

	snprintf(genbuf, sizeof(genbuf), "%s\n", buf);
	len = strlen(genbuf);
	return (send(fd, genbuf, len, 0) == len);
}

int
chat_recv(int fd, char *chatid)
{
	static char buf[512];
	static int bufstart = 0;
	int c, len;
	char *bptr, tmptopic[256];

	len = sizeof (buf) - bufstart - 1;
	if ((c = recv(fd, buf + bufstart, len, 0)) <= 0)
		return -1;
	c += bufstart;
	bptr = buf;
	while (c > 0) {
		len = strlen(bptr) + 1;
		if (len > c && len < (sizeof (buf) / 2))
			break;

		if (*bptr == '/') {
			switch (bptr[1]) {
			case 'c':
				chat_clear();
				break;
			case 'n':
				strncpy(chatid, bptr + 2, 8);
				print_chatid(chatid);
				clrtoeol();
				break;
			case 'r':
			case 't':
				if (bptr[1] == 'r') {
					strncpy(chatname, bptr + 2, IDLEN - 1);
					chatname[IDLEN - 1] = '\0';
				} else {
					strncpy(buftopic, bptr + 2, 43);
					buftopic[42] = '\0';
				}
				transPERstr(buftopic, tmptopic);

				clear_line(0);
				prints("\033[1;44;33m 房间： \033[32m%-12.12s  \033[5;31;44m%6.6s\033[0;1;44;33m话题：\033[36m%-49.49s\033[m",
					chatname, (recflag == 1) ? "[录音]" : "      ", tmptopic);
			}
		} else {
			printchatline(bptr);
		}

		c -= len;
		bptr += len;
	}

	if (c > 0) {
		strlcpy(buf, bptr, sizeof(buf));
		bufstart = len - 1;
	} else {
		bufstart = 0;
	}
	return 0;
}

void
fixchatid(char *chatid)
{
	chatid[CHAT_IDLEN] = '\0';
	while (*chatid != '\0' && *chatid != '\n') {
		if (strchr(BADCIDCHARS, *chatid))
			*chatid = '_';
		chatid++;
	}
}

int
ent_chat(char *chatbuf)
{
	char inbuf[80], chatid[CHAT_IDLEN], lastcmd[MAXLASTCMD][80];
	int cmdpos, ch, currchar, len, cfd = -1;
	int chatroom, newmail;
	int page_pending = NA;
	int chatting = YEA;

	chatroom = atoi(chatbuf) - 1;
	if (chatroom < 0 || chatroom >= PREDEFINED_ROOMS)
		return -1;

	modify_user_mode(chatrooms[chatroom].usermode);
	strlcpy(chat_station, chatrooms[chatroom].name, sizeof(chat_station));
	if (chatrooms[chatroom].port <= 0) {
		struct sockaddr_un addr;

		if ((cfd = socket(AF_UNIX, SOCK_STREAM, 0)) > 0) {
			strlcpy(addr.sun_path, chatrooms[chatroom].sockname, sizeof(addr.sun_path));
			addr.sun_family = AF_UNIX;
			if (connect(cfd, (struct sockaddr *)&addr, sizeof(addr) - sizeof(addr.sun_path) + strlen(addr.sun_path)) == -1) {
				close(cfd);
				cfd = -1;
			}
		}
	} else {
		cfd = async_connect(NULL, chatrooms[chatroom].port, CHAT_CONNECT_TIMEOUT);
	}

	if (cfd <= 0) {
		move(3, 0);
		outs("无法开启聊天室");
		clrtoeol();
		refresh();
		sleep(1);
		bell();
		goto freeup;
	}

	getdata(2, 0, "请输入聊天代号(退出: *)：", inbuf, CHAT_IDLEN, DOECHO, YEA);
	if (inbuf[0] == '*')
		goto freeup;
	snprintf(chatid, sizeof(chatid), "%.8s",
		((inbuf[0] != '\0' && inbuf[0] != '\n') ? inbuf : cuser.userid));
	fixchatid(chatid);
	snprintf(inbuf, sizeof(inbuf), "/! %d %d %s %s %d", uinfo.uid,
		cuser.userlevel, cuser.userid, chatid,
		HAS_PERM(PERM_CHATCLOAK) ? uinfo.invisible : 0);
	chat_send(cfd, inbuf);

	usleep(100);	/* monster: the delay is needed for some operating system like FreeBSD 4.X */
	while ((len = recv(cfd, inbuf, 3, 0)) < 3) {
		if (len == -1) 
			goto freeup;
	}

	move(3, 0);
	if (strcmp(inbuf, CHAT_LOGIN_OK)) {
		if (!strcmp(inbuf, CHAT_LOGIN_EXISTS)) {
			outs("这个代号已经有人用了");
		} else if (!strcmp(inbuf, CHAT_LOGIN_INVALID)) {
			outs("这个代号是错误的");
		} else {
			outs("你已经有另一个视窗进入此聊天室。");
		}
		clrtoeol();
		refresh();
		sleep(1);
		bell();
		goto freeup;
	}

	endmsg(SIGALRM);
	add_io(cfd, 0);

	newmail = cmdpos = currchar = 0;
	memset(lastcmd, 0, MAXLASTCMD * 80);

	uinfo.in_chat = YEA;
	strlcpy(uinfo.chatid, chatid, sizeof(uinfo.chatid));
	update_utmp();

	clear();
	chatline = 2;
	strlcpy(inbuf, chatid, sizeof(inbuf));
	stop_line = t_lines - 2;
	move(stop_line, 0);
	outs(msg_seperator);
	move(1, 0);
	outs(msg_seperator);
	print_chatid(chatid);
	memset(inbuf, 0, sizeof(inbuf));

	chat_load_alias();	/* KCN alias emote , 1999.11.25 */

	while (chatting) {
		move(b_lines, currchar + 10);
		ch = igetkey();
		talkidletime = 0;
		if (talkrequest)
			page_pending = YEA;
		if (page_pending)
			page_pending = servicepage(0, NULL);
		switch (ch) {
		case KEY_UP:
			cmdpos += MAXLASTCMD - 2;
		case KEY_DOWN:
			cmdpos++;
			cmdpos %= MAXLASTCMD;
			strlcpy(inbuf, lastcmd[cmdpos], sizeof(inbuf));
			move(b_lines, 10);
			clrtoeol();
			outs(inbuf);
			currchar = strlen(inbuf);
			continue;
		case KEY_LEFT:
			if (currchar)
				--currchar;
			continue;
		case KEY_RIGHT:
			if (inbuf[currchar])
				++currchar;
			continue;
		}
		if (!newmail && chkmail(YEA)) {
			newmail = 1;
			printchatline("\033[1;32m◆ \033[31m当！邮差送信来了...\033[m");
		}
		if (ch == I_OTHERDATA) {	/* incoming */
			if (chat_recv(cfd, chatid) == -1)
				break;
			continue;
		}
#ifdef BIT8
		if (isprint2(ch)) {
#else
		if (isprint(ch)) {
#endif
			if (currchar < 68) {
				if (inbuf[currchar]) {	/* insert */
					int i;

					for (i = currchar; inbuf[i] && i < 68;
					     i++) ;
					inbuf[i + 1] = '\0';
					for (; i > currchar; i--)
						inbuf[i] = inbuf[i - 1];
				} else {	/* append */
					inbuf[currchar + 1] = '\0';
				}
				inbuf[currchar] = ch;
				move(b_lines, currchar + 10);
				outs(&inbuf[currchar++]);
			}
			continue;
		}
		if (ch == '\n' || ch == '\r') {
			if (currchar) {
				chatting = chat_cmd(inbuf, cfd);
				if (chatting == 0)
					chatting = chat_send(cfd, inbuf);
				if (!strncmp(inbuf, "/b", 2))
					break;
				for (cmdpos = MAXLASTCMD - 1; cmdpos; cmdpos--)
					strcpy(lastcmd[cmdpos], lastcmd[cmdpos - 1]);
				strlcpy(lastcmd[0], inbuf, sizeof(lastcmd[0]));
				inbuf[0] = '\0';
				currchar = cmdpos = 0;
				move(b_lines, 10);
				clrtoeol();
			}
			continue;
		}
		if (ch == Ctrl('R')) {
			enabledbchar = !enabledbchar;
			continue;
		}
		if (ch == Ctrl('H') || ch == '\177') {
			if (currchar) {
				int i, patch = 0;

				if (enabledbchar) {
					for (i = currchar - 1;
					     i >= 0 && inbuf[i] & 0x80; i--)
						patch++;
					if (patch && patch % 2 == 0)
						patch = 1;
					else if (patch % 2 == 1 &&
						 inbuf[currchar] & 0x80) {
						currchar++;
						patch = 1;
					} else
						patch = 0;
				}	// 到此  1009
				currchar--;
				if (currchar && patch)
					currchar--;
				else
					patch = 0;
				inbuf[69] = '\0';
				memcpy(&inbuf[currchar],
				       &inbuf[currchar + 1 + patch],
				       69 - currchar);
				move(b_lines, currchar + 10);
				clrtoeol();
				outs(&inbuf[currchar]);
			}
			continue;
		}
		if (ch == Ctrl('Q')) {
			inbuf[0] = '\0';
			currchar = 0;
			move(b_lines, 10);
			clrtoeol();
			continue;
		}
		if (ch == Ctrl('C') || ch == Ctrl('D')) {
			chat_send(cfd, "/b");
			if (recflag == 1) {
				set_rec();
			}
			break;
		}
	}

freeup:
	if (cfd > 0) close(cfd);
	add_io(0, 0);
	uinfo.in_chat = NA;
	uinfo.chatid[0] = '\0';
	update_utmp();
	clear();
	refresh();
	if (chat_aliases != NULL) free(chat_aliases);	/* KCN alias emote , 1999.11.25 */
	return 0;
}

int
printuserent(struct user_info *uentp)
{
	static char uline[256];
	static int cnt;
	char pline[50];

	if (!uentp) {
		if (cnt)
			printchatline(uline);
		memset(uline, 0, 256);
		cnt = 0;
		return 0;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	if ((uentp->invisible &&
	     !(HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_SEECLOAK) ||
	       usernum == uentp->uid)) || isreject(uentp))
		return 0;

#if 0
	if (kill(uentp->pid, 0) == -1)
		return 0;
#endif

	snprintf(pline, sizeof(pline), " %s%-13s\033[m%c%-10.10s",
		myfriend(uentp->uid) ? "\033[1;32m" : "", uentp->userid,
		uentp->invisible ? '#' : ' ', modetype(uentp->mode));
	if (cnt < 2)
		strcat(pline, "│");
	strcat(uline, pline);
	if (++cnt == 3) {
		printchatline(uline);
		memset(uline, 0, 256);
		cnt = 0;
	}
	return 0;
}

void
chat_help(char *arg)
{
	char *ptr;
	char buf[256];
	FILE *fp;

	if (strstr(arg, " op")) {
		if ((fp = fopen("help/chatophelp", "r")) == NULL)
			return;
		while (fgets(buf, 255, fp) != NULL) {
			ptr = strstr(buf, "\n");
			*ptr = '\0';
			printchatline(buf);
		}
		fclose(fp);
	} else {
		if ((fp = fopen("help/chathelp", "r")) == NULL)
			return;
		while (fgets(buf, 255, fp) != NULL) {
			ptr = strstr(buf, "\n");
			*ptr = '\0';
			printchatline(buf);
		}
		fclose(fp);
	}
}

/* youzi 1997.7.25 */
void
query_user(char *arg)
{
	char *userid, msg[STRLEN * 2], name[STRLEN];
	char qry_mail_dir[STRLEN];
	int tuid, online = 0;
	time_t now;
	struct user_info uin;
	struct userec lookupuser;

	if ((userid = strrchr(arg, ' ')) == NULL) {
		printchatline("\033[1;37m★ \033[33m请输入您要查寻的 ID \033[37m★\033[m");
		return;
	}
	userid++;

	if ((tuid = getuser(userid, &lookupuser)) == 0) {
		printchatline("\033[1;31m不正确的使用者代号\033[m");
		return;
	}
	online = t_search_ulist(&uin, t_cmpuids, tuid, NA, NA);
	snprintf(qry_mail_dir, sizeof(qry_mail_dir), "mail/%c/%s/%s",
		mytoupper(lookupuser.userid[0]), lookupuser.userid, DOT_DIR);
	snprintf(msg, sizeof(msg), "\033[1;37m%s\033[m (\033[1;33m%s\033[m) 共上站 \033[1;32m%d\033[m 次, 发表"
		"过 \033[1;32m%d\033[m 篇文章", lookupuser.userid,
		lookupuser.username, lookupuser.numlogins, lookupuser.numposts);
	printchatline(msg);

	getdatestring(lookupuser.lastlogin);
	if (lookupuser.lasthost[0] == '\0') ;
	else if ((lookupuser.userdefine & DEF_NOTHIDEIP) ||
		 (HAS_PERM(PERM_SYSOP)) || (HAS_PERM(PERM_SEEIP))) {
		snprintf(msg, sizeof(msg),
			"上次在 [\033[1;32m%s\033[m] 由 [\033[1;32m%s\033[m] 到本站一游",
			datestring, lookupuser.lasthost);
	} else {
		snprintf(msg, sizeof(msg),
			"上次在 [\033[1;32m%s\033[m] 由 [\033[1;32m不知道哪里\033[m] 到本站一游",
			datestring);
	}
	printchatline(msg);

	snprintf(msg, sizeof(msg), 
		"信箱：[\033[1;5;32m%2s\033[m],生命力：[\033[1;32m%d\033[m]",
		(check_query_mail(qry_mail_dir) == 1) ? "信" : "  ",
		compute_user_value(&lookupuser));

	if (lookupuser.usertitle == 0) {
		snprintf(msg, sizeof(msg), "%s\n", msg);
	} else {
		getusertitlestr(lookupuser.usertitle, name);
		snprintf(msg, sizeof(msg), "%s\n称号：[\033[1;33m%s\033[m]\n", msg, name);
	}
	printchatline(msg);

	if (online) {
		snprintf(msg, sizeof(msg), "\033[1;37m目前在站上\033[m");
	} else {
		if (lookupuser.lastlogout < lookupuser.lastlogin) {
			now = ((time(NULL) - lookupuser.lastlogin) / 120) % 47 + 1 + lookupuser.lastlogin;
		} else {
			now = lookupuser.lastlogout;
		}
		getdatestring(now);

		snprintf(msg, sizeof(msg), "\033[1;37m离站时间：[\033[1;32m%s\033[1;37m]\033[m", datestring);
	}
	printchatline(msg);
}

void
call_user(char *arg)
{
	char *userid, msg[STRLEN * 2];
	struct user_info *uin;

	if ((userid = strrchr(arg, ' ')) == NULL) {
		printchatline("\033[1;37m★ \033[32m请输入你要邀请的 ID\033[37m ★\033[m");
		return;
	}
	++userid;

	/* monster: 给自己发邀请信息会导致bbs死锁或断线 */
	if (!strcasecmp(userid, currentuser.userid)) {
		printchatline("\033[1;32m你不用邀请自己啊\033[m");
		return;
	}

	uin = t_search(userid, NA);
	if (uin == NULL ||
	    (uin->invisible && (!HAS_PERM(PERM_SYSOP | PERM_SEECLOAK)))
	    || !canmsg(uin)) {
		snprintf(msg, sizeof(msg), "\033[1;32m%s\033[37m %s\033[m", userid,
			uin && !uin->invisible ? "无法呼叫" : "并没有上站");
	} else {
		snprintf(msg, sizeof(msg), "到 %s 的 %s 包厢聊聊天", chat_station, chatname);
		do_sendmsg(uin, msg, 1, uin->pid);
		snprintf(msg, sizeof(msg), "\033[1;37m已经帮你邀请 \033[32m%s\033[37m 了\033[m", uin->userid);
	}
	printchatline(msg);
}

void
chat_date(void)
{
	time_t thetime;

	time(&thetime);
	snprintf(genbuf, sizeof(genbuf), "\033[1m %s标准时间: \033[32m%s\033[m", BoardName, Cdate(&thetime));
	printchatline(genbuf);
}

void
chat_users(void)
{
	printchatnewline();
	snprintf(genbuf, sizeof(genbuf), "\033[1m【 \033[36m%s \033[37m的访客列表 】\033[m", BoardName);
	printchatline(genbuf);
	printchatline(msg_shortulist);

	if (apply_ulist(printuserent) == -1) {
		printchatline("\033[1m空无一人\033[m");
	}
	printuserent(NULL);
}

int
print_friend_ent(struct user_info *uentp)		/* print one user & status if he is a friend */
{
	static char uline[256];
	static int cnt;
	char pline[50];

	if (!uentp) {
		if (cnt)
			printchatline(uline);
		memset(uline, 0, 256);
		cnt = 0;
		return 0;
	}
	if (!uentp->active || !uentp->pid)
		return 0;
	if (!
	    (HAS_PERM(PERM_SEECLOAK) || HAS_PERM(PERM_SYSOP) ||
	     usernum == uentp->uid) && uentp->invisible)
		return 0;

#if 0
	if (kill(uentp->pid, 0) == -1)
		return 0;
#endif

	/*if (!myfriend(uentp->userid)) Leeward 99.02.01 */
	if (!myfriend(uentp->uid))
		return 0;

	snprintf(pline, sizeof(pline), " %-13s%c%-10s",
		uentp->userid, uentp->invisible ? '#' : ' ',
		modetype(uentp->mode));
	//modestring(uentp->mode, uentp->destuid, 0, NULL));
	if (cnt < 2)
		strcat(pline, "│");
	strcat(uline, pline);
	if (++cnt == 3) {
		printchatline(uline);
		memset(uline, 0, 256);
		cnt = 0;
	}
	return 0;
}

void
chat_friends(void)
{
	printchatnewline();
	printchatline("\033[1m【 当前线上的好友列表 】\033[m");
	printchatline(msg_shortulist);

	if (apply_ulist(print_friend_ent) == -1) {
		printchatline("\033[1m没有朋友在线上\033[m");
	}
	print_friend_ent(NULL);
}

/* KCN alias emote , 1999.11.25 */
void
chat_sendmsg(char *arg)
{
	struct user_info *uentp;
	char *uident, *msgstr, showstr[80];

	uident = strchr(arg, ' ');
	if (uident != NULL) {
		for (; *uident != 0 && *uident == ' '; uident++) ;
	}
	if (uident == NULL || *uident == 0) {
		printchatline("\033[1;32m请输入你要发消息的 ID\033[m");
		return;
	}

	msgstr = strchr(uident, ' ');
	if (msgstr != NULL) {
		*msgstr = 0;
		msgstr++;
		for (; *msgstr != 0 && *msgstr == ' '; msgstr++) ;
	}
	if (msgstr == NULL || *msgstr == 0) {
		printchatline("\033[1;32m请输入你要发的消息\033[m");
		return;
	}

	uentp = t_search(uident, NA);
	if (uentp == NULL || (uentp->invisible &&
	    (!HAS_PERM(PERM_SYSOP | PERM_SEECLOAK)))) {
		printchatline("\033[1m线上没有这个ID\033[m");
		return;
	}

	if (do_sendmsg(uentp, msgstr, 2, 0) != 1) {
		snprintf(showstr, sizeof(showstr), "\033[1m无法发消息给 %s \033[m", uentp->userid);
		printchatline(showstr);
		return;
	}
	snprintf(showstr, sizeof(showstr), "\033[1m已经给 %s 发出消息\033[m", uentp->userid);
	printchatline(showstr);
}

void
chat_showmail(void)
{
	struct fileheader *ents;
	char maildir[PATH_MAX + 1];
	int i;
	int nums;
	int lines = t_lines - 7;
	int base;

	printchatline("\033[1;32m当前信件列表\033[0m");
	printchatline("\033[1;36m----------------------------------------------\033[0m");

	if ((ents = (struct fileheader *)malloc(sizeof(struct fileheader) * lines)) == NULL)
		return;

	setmaildir(maildir, currentuser.userid);
	nums = get_num_records(maildir, sizeof(struct fileheader));
	base = nums - lines + 1;
	if (base <= 0)
		base = 1;

	lines = get_records(maildir, ents, sizeof(struct fileheader), base, lines);

	for (i = 0; i < lines; i++) {
		printchatline(maildoent(i + base, &ents[i]));
	}
	free(ents);
}

/* monster: following fuction was written by lourie, modified by me */
void
chat_showmsg(void)
{
	char mf[STRLEN], genbuf[1024];
	char *line;
	FILE *mfp;
	int l, i;

	setuserfile(mf, "msgfile.me");
	if ((mfp = fopen(mf, "r")) == NULL) {
		printchatline("\033[1;36m没有任何信息存在！\033[m");
		return;
	}

	if (fseek(mfp, -1000, SEEK_END) == -1) {
		rewind(mfp);
	}
	l = fread(genbuf, sizeof (char), 1000, mfp);
	fclose(mfp);
	if (l > 0) {
		genbuf[l] = '\0';
	} else {
		printchatline("\033[1;36m没有任何信息存在！\033[m");
		return;
	}

	printchatline("\x1b[1;36m最新的信息如下：\x1b[m");
	for (i = 0; i < 10; i++) {
		line = strrchr(genbuf, '\n');
		if (line == NULL) {
			if (l < 1000)	//the msg file is less than 1000 bytes
				printchatline(genbuf);
			break;
		}
		printchatline(line + 1);
		*line = '\0';
	}

	printchatline("\033[m\n");
	return;
}

void
check_unread(void)
{
	static time_t last_use_time = 0;
	time_t now;

	now = time(NULL);
	if (now - last_use_time < 5) {
		printchatline("请稍后再使用本命令");
		return;
	}
	last_use_time = now;
	printchatline("\033[1;32m有新文章的个人收藏版面列表\033[m");
	load_GoodBrd();
	apply_boards(c_mygrp_unread);
	c_mygrp_unread(NULL);
}

void
define_alias(char *arg)
{
	int i;
	int del = 0;
	char buf[256];
	char *action = NULL;

	for (i = 0; (i < 255) && (arg[i] != 0) && !isspace((unsigned int)(arg[i])); i++) ;

	if (arg[i] == 0) {
		if (chat_alias_count != 0) {
			printchatline("已定义的alias:\n");
			for (i = 0; i < chat_alias_count; i++) {
				if (chat_aliases[i].cmd[0] != 0) {
					snprintf(buf, sizeof(buf), "%-9s %s\n",
						chat_aliases[i].cmd,
						chat_aliases[i].action);
					printchatline(buf);
				};
			};
			return;
		} else
			printchatline("未定义alias\n");
	};

	arg = &arg[i + 1];
	for (i = 0; (i < 9) && (arg[i] != 0) && !isspace((unsigned int)(arg[i])); i++) ;
	if (i >= 9) {
		printchatline("alias太长!\n");
		return;
	};

	if (arg[i] == 0) {
		del = 1;
	} else {
		action = &arg[i + 1];
	}

	arg[i] = 0;

	if (del == 0) {
		if (chat_alias_count == MAXDEFINEALIAS) {
			printchatline("自定义alias已经满了\n");
			return;
		}
	}

	for (i = 0; i < chat_alias_count; i++)
		if (!strncasecmp(chat_aliases[i].cmd, arg, 8)) {
			if (del) {
				chat_alias_count--;
				setuserfile(buf, "chatalias");
				if (chat_alias_count != 0) {
					memcpy(&chat_aliases[i],
					       &chat_aliases[chat_alias_count],
					       sizeof (struct chatalias));
					chat_aliases[chat_alias_count].cmd[0] =
					    0;
					substitute_record(buf,
							  &chat_aliases
							  [chat_alias_count],
							  sizeof (chat_aliases
								  [chat_alias_count]),
							  chat_alias_count + 1);
				} else {
					chat_aliases[i].cmd[0] = 0;
				}
				substitute_record(buf, &chat_aliases[i],
						  sizeof (chat_aliases[i]),
						  i + 1);
				printchatline("自定义alias已经删除\n");
				return;
			} else {
				snprintf(buf, sizeof(buf), "自定义alias-%s已经存在\n",
					chat_aliases[i].cmd);
				printchatline(buf);
				return;
			}
		}
	if (!del) {
		strlcpy(chat_aliases[chat_alias_count].cmd, arg, sizeof(chat_aliases[chat_alias_count].cmd));
		strncpy(chat_aliases[chat_alias_count].action, action, sizeof(chat_aliases[chat_alias_count].action));
		snprintf(buf, sizeof(buf), "自定义alias-%s已经创建\n", arg);
		printchatline(buf);
		i = chat_alias_count;
		chat_alias_count++;
	} else {
		printchatline("没找到自定义alias\n");
		return;
	}
	setuserfile(buf, "chatalias");
	substitute_record(buf, &chat_aliases[i], sizeof(chat_aliases[i]), i + 1);
}

int
use_alias(char *arg, int cfd)
{
	char buf[256];
	char buf1[256];
	char *args[10];
	int arg_count;
	int i;
	int slen;
	char *fmt;

	strncpy(buf, arg, 255);
	arg_count = 0;
	args[0] = buf;
	args[1] = buf;
	while (*args[arg_count + 1] != 0)
		if (isspace((unsigned int)(*args[arg_count + 1]))) {
			arg_count++;
			if (arg_count == 9)
				break;
			*args[arg_count] = 0;
			args[arg_count]++;
			args[arg_count + 1] = args[arg_count];
		} else
			args[arg_count + 1]++;
	for (i = arg_count + 1; i < 10; i++)
		args[i] = "大家";

	for (i = 0; i < chat_alias_count; i++) {
		if (!strncasecmp(chat_aliases[i].cmd, args[0], 8)) {
			strcpy(buf1, "/a ");
			slen = 3;
			fmt = chat_aliases[i].action;
			while (*fmt != 0) {
				if (*fmt == '$') {
					fmt++;
					if (isdigit(*fmt)) {
						int index = *fmt - '0';

						if (slen + strlen(args[index]) > 255 - 8)
							break;
						buf1[slen] = 0;
						strcat(buf1, "\033[1m");
						strcat(buf1, args[index]);
						strcat(buf1, "\033[m");
						slen += strlen(args[index]) + 7;
					} else if (*fmt == 's') {
						if (slen + strlen(args[1]) > 255 - 8)
							break;
						buf1[slen] = 0;
						strcat(buf1, "\033[1m");
						strcat(buf1, args[1]);
						strcat(buf1, "\033[m");
						slen += strlen(args[1]) + 7;
					} else if (slen < 253) {
						buf1[slen++] = '$';
						buf1[slen++] = *fmt;
					} else
						break;
				} else
					buf1[slen++] = *fmt;
				fmt++;
			}
			buf1[slen] = 0;
			chat_send(cfd, buf1);
			return 1;
		}
	}
	return 0;
}

struct chat_command chat_cmdtbl[] = {
	{"alias", define_alias},	/* KCN alias emote , 1999.11.25 */
	{"pager", setpager},
	{"help", chat_help},
	{"?", chat_help},
	{"clear", chat_clear},
	{"date", chat_date},
	{"users", chat_users},
	{"set", set_rec},
	{"g", chat_friends},
	{"call", call_user},
	{"query", query_user},	/* 1997.7.25 youzi */
	{"mail", chat_showmail},
	{"sm", chat_showmail},
	{"msg", chat_showmsg},
	{"see", chat_showmsg},	/* 2001.2.2 yy */
	{"favor", check_unread},
	{"sn", check_unread},	/* 2001 2.3 yy */
	{NULL, NULL}
};

int
chat_cmd_match(char *buf, char *str)
{
	while (*str && *buf && !isspace((unsigned int)(*buf))) {
		if (tolower((unsigned int)(*buf++)) != *str++)
			return 0;
	}
	return 1;
}

/* KCN alias emote , 1999.11.25 */
int
chat_cmd(char *buf, int cfd)
{
	int i;

	if (*buf++ != '/')
		return 0;

	if (*buf == '/') {
		if (!use_alias(buf + 1, cfd)) {
			return 0;
		} else {
			return 1;
		}
	}

	if ((tolower((unsigned int)(*buf)) == 'm') && isspace((unsigned int)(*(buf + 1))))
		return 0;
	if ((tolower((unsigned int)(*buf)) == 'a') && isspace((unsigned int)(*(buf + 1))))
		return 0;
	if ((tolower((unsigned int)(*buf)) == 's') && isspace((unsigned int)(*(buf + 1)))) {
		chat_sendmsg(buf);
		return 1;
	} else {
		for (i = 0; chat_cmdtbl[i].cmdname; i++) {
			if (*buf != '\0' &&
			    chat_cmd_match(buf, chat_cmdtbl[i].cmdname)) {
				chat_cmdtbl[i].cmdfunc(buf);
				return 1;
			}
		}
	}
	return 0;
}
