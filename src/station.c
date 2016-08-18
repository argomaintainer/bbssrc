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

#define CHATD

#include "bbs.h"
#include "chat.h"

#ifdef AIX
#include <sys/select.h>
#endif

#if !RELIABLE_SELECT_FOR_WRITE
#include <fcntl.h>
#endif

#if USES_SYS_SELECT_H
#include <sys/select.h>
#endif

#if NO_SETPGID
#define setpgid setpgrp
#endif

#ifndef L_XTND
#define L_XTND          2	/* relative to end of file */
#endif

#define RESTRICTED(u)   (users[(u)].flags == 0)	/* guest */
#define SYSOP(u)        (users[(u)].flags & (PERM_ACHATROOM|PERM_SYSOP))
					/* PERM_ACHATROOM -- ÁÄÌìÊÒ¹ÜÀíÔ± */
#define CLOAK(u)        (users[(u)].flags & PERM_CHATCLOAK)
#define ROOMOP(u)       (users[(u)].flags & PERM_CHATROOM)
#define OUTSIDER(u)     (users[(u)].flags & PERM_DENYPOST)
#define PERM_TALK	PERM_NOZAP	/* ½èÓÃÒ»ÏÂÓÃÀ´¼ÇÂ¼·¢ÑÔÈ¨ */
#define CANTALK(u)	(users[(u)].flags & PERM_TALK)
#define MESG(u)		(users[(u)].flags & PERM_MESSAGE) /* ½èÓÃÀ´¼ÇÂ¼Âó¿Ë·ç×´Ì¬ */

#define ROOM_LOCKED     0x1
#define ROOM_SECRET     0x2
#define ROOM_NOEMOTE    0x4
#define ROOM_SCHOOL     0x8

#define LOCKED(r)       (rooms[(r)].flags & ROOM_LOCKED)
#define SECRET(r)       (rooms[(r)].flags & ROOM_SECRET)
#define NOEMOTE(r)      (rooms[(r)].flags & ROOM_NOEMOTE)
#define NOTALK(r)       (rooms[(r)].flags & ROOM_SCHOOL)

#define ROOM_ALL        (-2)
#define PERM_CHATROOM PERM_CHAT

struct chatuser {
	int sockfd;		/* socket to bbs server */
	int utent;		/* utable entry for this user */
	int room;		/* room: -1 means none, 0 means main */
	int flags;
	char cloak;
	char userid[IDLEN + 2];	/* real userid */
	char chatid[CHAT_IDLEN];	/* chat id */
	char ibuf[128];		/* buffer for sending/receiving */
	int ibufsize;		/* current size of ibuf */
	char host[30];
} users[MAXACTIVE];

struct chatroom {
	char name[IDLEN];	/* name of room; room 0 is "main" */
	short occupants;	/* number of users in room */
	short flags;		/* ROOM_LOCKED, ROOM_SECRET */
	char invites[MAXACTIVE];	/* Keep track of invites to rooms */
	char topic[48];		/* Let the room op to define room topic */
} rooms[MAXROOM];

struct chatcmd {
	char *cmdstr;
	void (*cmdfunc) ();
	int exact;
};

int chatroom, chatport;
int sock = -1;			/* the socket for listening */
int nfds;			/* number of sockets to select on */
int num_conns;			/* current number of connections */
fd_set allfds;			/* fd set for selecting */
struct timeval zerotv;		/* timeval for selecting */
char chatbuf[256];		/* general purpose buffer */
char chatname[19];

/* name of the main room (always exists) */

char mainroom[] = "main";
char *maintopic = "½ñÌì£¬ÎÒÃÇÒªÌÖÂÛµÄÊÇ.....";

char *msg_not_op = "\033[1;37m¡ï\033[32mÄú²»ÊÇÕâÏá·¿µÄÀÏ´ó\033[37m ¡ï\033[m";
char *msg_no_such_id =
    "\033[1;37m¡ï\033[32m [\033[36m%s\033[32m] ²»ÔÚÕâ¼äÏá·¿Àï\033[37m ¡ï\033[m";
char *msg_not_here =
    "\033[1;37m¡ï \033[32m[\033[36m%s\033[32m] ²¢Ã»ÓĞÇ°À´±¾»áÒéÌü\033[37m ¡ï\033[m";

static int local;	/* whether the chatroom accepts local connection only */

int
is_valid_chatid(char *id)
{
	int i;

	if (*id == '\0')
		return 0;

	for (i = 0; i < CHAT_IDLEN && *id; i++, id++) {
		if (strchr(BADCIDCHARS, *id))
			return 0;
	}
	return 1;
}

char *
nextword(char **str)
{
	char *p;

	while (isspace(**str))
		(*str)++;
	p = *str;
	while (**str && !isspace(**str))
		(*str)++;
	if (**str) {
		**str = '\0';
		(*str)++;
	}

	return p;
}

int
chatid_to_indx(int unum, char *chatid)
{
	int i;

	for (i = 0; i < MAXACTIVE; i++) {
		if (users[i].sockfd == -1)
			continue;
		if (!strcasecmp(chatid, users[i].chatid)) {
			if (users[i].cloak == 0 || !CLOAK(unum))
				return i;
		}
	}
	return -1;
}

int
fuzzy_chatid_to_indx(int unum, char *chatid)
{
	int i, indx = -1;
	unsigned int len = strlen(chatid);

	for (i = 0; i < MAXACTIVE; i++) {
		if (users[i].sockfd == -1)
			continue;
		if (!strncasecmp(chatid, users[i].chatid, len) ||
		    !strncasecmp(chatid, users[i].userid, len)) {
			if (len == strlen(users[i].chatid) ||
			    len == strlen(users[i].userid)) {
				indx = i;
				break;
			}
			if (indx == -1)
				indx = i;
			else
				return -2;
		}
	}
	if (users[indx].cloak == 0 || CLOAK(unum))
		return indx;
	else
		return -1;
}

int
roomid_to_indx(char *roomid)
{
	int i;

	for (i = 0; i < MAXROOM; i++) {
		if (i && rooms[i].occupants == 0)
			continue;
		if (!strcasecmp(roomid, rooms[i].name))
			return i;
	}
	return -1;
}

void
do_send(fd_set *writefds, char *str)
{
	int i;
	int len = strlen(str);

	if (select(nfds, NULL, writefds, NULL, &zerotv) > 0) {
		for (i = 0; i < nfds; i++) {
			if (FD_ISSET(i, writefds)) {
				send(i, str, len + 1, 0);
			}
		}
	}
}

void
send_to_room(int room, char *str)
{
	int i;
	fd_set writefds;

	FD_ZERO(&writefds);
	for (i = 0; i < MAXACTIVE; i++) {
		if (users[i].sockfd == -1)
			continue;
		if (room == ROOM_ALL || room == users[i].room)
			/* write(users[i].sockfd, str, strlen(str) + 1); */
			FD_SET(users[i].sockfd, &writefds);
	}
	do_send(&writefds, str);
}

void
send_to_unum(int unum, char *str)
{
	fd_set writefds;

	FD_ZERO(&writefds);
	FD_SET(users[unum].sockfd, &writefds);
	do_send(&writefds, str);
}

void
exit_room(int unum, int disp, char *msg)
{
	int oldrnum = users[unum].room;

	if (oldrnum != -1) {
		if (--rooms[oldrnum].occupants) {
			switch (disp) {
			case EXIT_LOGOUT:
				snprintf(chatbuf, sizeof(chatbuf),
					"\033[1;37m¡ï \033[32m[\033[36m%s\033[32m] ÂıÂıÀë¿ªÁË \033[37m¡ï\033[m",
					users[unum].chatid);
				if (msg && *msg) {
					strcat(chatbuf, ": ");
					strncat(chatbuf, msg, 80);
				}
				break;

			case EXIT_LOSTCONN:
				snprintf(chatbuf, sizeof(chatbuf),
					"\033[1;37m¡ï \033[32m[\033[36m%s\033[32m] Ïñ¶ÏÁËÏßµÄ·çóİ ... \033[37m¡ï\033[m",
					users[unum].chatid);
				break;

			case EXIT_KICK:
				snprintf(chatbuf, sizeof(chatbuf),
					"\033[1;37m¡ï \033[32m[\033[36m%s\033[32m] ±»ÀÏ´ó¸Ï³öÈ¥ÁË \033[37m¡ï\033[m",
					users[unum].chatid);
				break;
			}
			if (users[unum].cloak == 0)
				send_to_room(oldrnum, chatbuf);
		}
	}
	users[unum].flags &= ~PERM_CHATROOM;
	users[unum].room = -1;
}

void
chat_rename(int unum, char *msg)
{
	int rnum;

	rnum = users[unum].room;

	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if (*msg == '\0') {
		send_to_unum(unum,
			     "\033[1;31m¡ò \033[37mÇëÖ¸¶¨ĞÂµÄÁÄÌìÊÒÃû³Æ \033[31m¡ò\033[m");
		return;
	}
	strncpy(rooms[rnum].name, msg, IDLE);
	snprintf(chatbuf, sizeof(chatbuf), "/r%.11s", msg);
	send_to_room(rnum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m¡ï \033[32m[\033[36m%s\033[32m] ½«ÁÄÌìÊÒÃû³Æ¸ÄÎª \033[1;33m%.11s \033[37m¡ï\033[m",
		users[unum].chatid, msg);
	send_to_room(rnum, chatbuf);
}

void
chat_topic(int unum, char *msg)
{
	int rnum;

	rnum = users[unum].room;

	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if (*msg == '\0') {
		send_to_unum(unum, "\033[1;31m¡ò \033[37mÇëÖ¸¶¨»°Ìâ \033[31m¡ò\033[m");
		return;
	}
	strncpy(rooms[rnum].topic, msg, 43);
	rooms[rnum].topic[42] = '\0';
	snprintf(chatbuf, sizeof(chatbuf), "/t%.42s", msg);
	send_to_room(rnum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m¡ï \033[32m[\033[36m%s\033[32m] ½«»°Ìâ¸ÄÎª \033[1;33m%42.42s \033[37m¡ï\033[m",
		users[unum].chatid, msg);
	send_to_room(rnum, chatbuf);
}

int
enter_room(int unum, char *room, char *msg)
{
	int rnum;
	int op = 0;
	int i;

	rnum = roomid_to_indx(room);
	if (rnum == -1) {
		/* new room */
		if (OUTSIDER(unum)) {
			send_to_unum(unum,
				     "\033[1;31m¡ò \033[37m±§Ç¸£¬Äú²»ÄÜ¿ªĞÂ°üÏá \033[31m¡ò\033[m");
			return 0;
		}
		for (i = 1; i < MAXROOM; i++) {
			if (rooms[i].occupants == 0) {
				rnum = i;
				memset(rooms[rnum].invites, 0, MAXACTIVE);
				strcpy(rooms[rnum].topic, maintopic);
				strncpy(rooms[rnum].name, room, IDLEN - 1);
				rooms[rnum].name[IDLEN - 1] = '\0';
				rooms[rnum].flags = 0;
				op++;
				break;
			}
		}
		if (rnum == -1) {
			send_to_unum(unum,
				     "\033[1;31m¡ò \033[37mÎÒÃÇµÄ·¿¼äÂúÁËà¸ \033[31m¡ò\033[m");
			return 0;
		}
	}
	if (!SYSOP(unum))
		if (LOCKED(rnum) && rooms[rnum].invites[unum] == 0) {
			send_to_unum(unum,
				     "\033[1;31m¡ò \033[37m±¾·¿ÉÌÌÖ»úÃÜÖĞ£¬·ÇÇëÎğÈë \033[31m¡ò\033[m");
			return 0;
		}
	exit_room(unum, EXIT_LOGOUT, msg);
	users[unum].room = rnum;
	if (op)
		users[unum].flags |= PERM_CHATROOM;
	rooms[rnum].occupants++;
	rooms[rnum].invites[unum] = 0;
	if (users[unum].cloak == 0) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;31m¡õ \033[37m[\033[36;1m%s\033[37m] ½øÈë \033[35m%s \033[37m°üÏá \033[31m¡õ\033[m",
			users[unum].chatid, rooms[rnum].name);
		send_to_room(rnum, chatbuf);
	}
	snprintf(chatbuf, sizeof(chatbuf), "/r%s", room);
	send_to_unum(unum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "/t%s", rooms[rnum].topic);
	send_to_unum(unum, chatbuf);
	return 0;
}

void
logout_user(int unum)
{
	int i, sockfd = users[unum].sockfd;

	close(sockfd);
	FD_CLR(sockfd, &allfds);
	memset(&users[unum], 0, sizeof (users[unum]));
	users[unum].sockfd = users[unum].utent = users[unum].room = -1;
	for (i = 0; i < MAXROOM; i++) {
		if (rooms[i].invites != NULL)
			rooms[i].invites[unum] = 0;
	}
	num_conns--;
}

int
print_user_counts(int unum)
{
	int i, c, userc = 0, suserc = 0, roomc = 0;

	for (i = 0; i < MAXROOM; i++) {
		c = rooms[i].occupants;
		if (i > 0 && c > 0) {
			if (!SECRET(i) || SYSOP(unum))
				roomc++;
		}
		c = users[i].room;
		if (users[i].sockfd != -1 && c != -1 && users[i].cloak == 0) {
			if (SECRET(c) && !SYSOP(unum))
				suserc++;
			else
				userc++;
		}
	}
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;31m¡õ\033[37m »¶Ó­¹âÁÙ¡º\033[32m%s\033[37m¡»µÄ¡¾\033[36m%s\033[37m¡¿\033[31m¡õ\033[m", BBSNAME, chatname);
	send_to_unum(unum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;31m¡õ\033[37m Ä¿Ç°ÒÑ¾­ÓĞ \033[1;33m%d \033[37m¼ä»áÒéÊÒÓĞ¿ÍÈË \033[31m¡õ\033[m",
		roomc + 1);
	send_to_unum(unum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;31m¡õ \033[37m±¾»áÒéÌüÄÚ¹²ÓĞ \033[36m%d\033[37m ÈË ", userc + 1);
	if (suserc) {
		char buf[40];

		snprintf(buf, sizeof(buf), "[\033[36m%d\033[37m ÈËÔÚ¸ß»úÃÜÌÖÂÛÊÒ]", suserc);
		strlcat(chatbuf, buf, sizeof(chatbuf));
	}
	strlcat(chatbuf, "\033[31m¡õ\033[m", sizeof(chatbuf));
	send_to_unum(unum, chatbuf);
	return 0;
}

int
login_user(int unum, char *msg)
{
	int i, utent;		/* , fd = users[unum].sockfd; */
	char *utentstr;
	char *level;
	char *userid;
	char *chatid;
	char *cloak;

	utentstr = nextword(&msg);
	level = nextword(&msg);
	userid = nextword(&msg);
	chatid = nextword(&msg);
	cloak = nextword(&msg);

	utent = atoi(utentstr);
	for (i = 0; i < MAXACTIVE; i++) {
		if (users[i].sockfd != -1 && users[i].utent == utent) {
			send_to_unum(unum, CHAT_LOGIN_BOGUS);
			return -1;
		}
	}
	if (!is_valid_chatid(chatid)) {
		send_to_unum(unum, CHAT_LOGIN_INVALID);
		return 0;
	}
	if (chatid_to_indx(0, chatid) != -1) {
		/* userid in use */
		send_to_unum(unum, CHAT_LOGIN_EXISTS);
		return 0;
	}
	if (!strncasecmp("localhost" /* MY_BBS_DOMAIN */ , users[unum].host, 30)) {
		users[unum].flags = atoi(level) & ~(PERM_DENYPOST);
		users[unum].cloak = atoi(cloak);
	} else {
		if (!(atoi(level) & PERM_LOGINOK) && !strncasecmp(chatid, "guest", 8)) {
			send_to_unum(unum, CHAT_LOGIN_INVALID);
			return 0;
		}
		users[unum].flags = PERM_DENYPOST;
		users[unum].cloak = 0;
	}

	users[unum].utent = utent;
	strcpy(users[unum].userid, userid);
	strncpy(users[unum].chatid, chatid, CHAT_IDLEN - 1);
	users[unum].chatid[CHAT_IDLEN - 1] = '\0';
	send_to_unum(unum, CHAT_LOGIN_OK);
	print_user_counts(unum);
	enter_room(unum, mainroom, (char *)NULL);
	return 0;
}

void
chat_list_rooms(int unum, char *msg)
{
	int i, occupants;

	if (RESTRICTED(unum)) {
		send_to_unum(unum, "\033[1;31m¡ò \033[37m±§Ç¸£¡ÀÏ´ó²»ÈÃÄã¿´ÓĞÄÄĞ©·¿¼äÓĞ¿ÍÈË \033[31m¡ò\033[m");
		return;
	}
	send_to_unum(unum, "\033[1;33;44m Ì¸ÌìÊÒÃû³Æ  ©¦ÈËÊı©¦»°Ìâ            \033[m");
	for (i = 0; i < MAXROOM; i++) {
		occupants = rooms[i].occupants;
		if (occupants > 0) {
			if (!SYSOP(unum))
				if ((rooms[i].flags & ROOM_SECRET) &&
				    (users[unum].room != i))
					continue;
			snprintf(chatbuf, sizeof(chatbuf),
				" \033[1;32m%-12s\033[37m©¦\033[36m%4d\033[37m©¦\033[33m%s\033[m",
				rooms[i].name, occupants, rooms[i].topic);
			if (rooms[i].flags & ROOM_LOCKED)
				strlcat(chatbuf, " [Ëø×¡]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_SECRET)
				strlcat(chatbuf, " [»úÃÜ]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_NOEMOTE)
				strlcat(chatbuf, " [½ûÖ¹¶¯×÷]", sizeof(chatbuf));
			if (rooms[i].flags & ROOM_SCHOOL)
				strlcat(chatbuf, " [Ò»ÈËÌáÎÊ]", sizeof(chatbuf));
			send_to_unum(unum, chatbuf);
		}
	}
}

void
chat_do_user_list(int unum, char *msg, int whichroom)
{
	int start, stop, curr = 0;
	int i, rnum, myroom = users[unum].room;

	while (*msg && isspace(*msg))
		msg++;
	start = atoi(msg);
	while (*msg && isdigit(*msg))
		msg++;
	while (*msg && !isdigit(*msg))
		msg++;
	stop = atoi(msg);
	send_to_unum(unum,
		     "\033[1;33;44m ÁÄÌì´úºÅ©¦Ê¹ÓÃÕß´úºÅ  ©¦ÁÄ    Ìì    ÊÒ¡õOp¡õÀ´×Ô                          \033[m");
	for (i = 0; i < MAXACTIVE; i++) {
		rnum = users[i].room;
		if (users[i].sockfd != -1 && rnum != -1 &&
		    !(users[i].cloak == 1 && !CLOAK(unum))) {
			if (whichroom != -1 && whichroom != rnum)
				continue;
			if (myroom != rnum) {
				if (RESTRICTED(unum))
					continue;
				if ((rooms[rnum].flags & ROOM_SECRET) &&
				    !SYSOP(unum))
					continue;
			}
			curr++;
			if (curr < start)
				continue;
			else if (stop && (curr > stop))
				break;
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;5m%c\033[0;1;37m%-8s©¦\033[31m%s%-12s\033[37m©¦\033[32m%-14s\033[37m¡õ\033[34m%-2s\033[37m¡õ\033[35m%-30s\033[m",
				(users[i].cloak == 1) ? 'C' : ' ',
				users[i].chatid,
				OUTSIDER(i) ? "\033[1;35m" : "\033[1;36m",
				users[i].userid, rooms[rnum].name,
				ROOMOP(i) ? "ÊÇ" : "·ñ", users[i].host);
			send_to_unum(unum, chatbuf);
		}
	}
}

void
chat_list_by_room(int unum, char *msg)
{
	int whichroom;
	char *roomstr;

	roomstr = nextword(&msg);
	if (*roomstr == '\0')
		whichroom = users[unum].room;
	else {
		if ((whichroom = roomid_to_indx(roomstr)) == -1) {
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;31m¡ò \033[37mÃ» %s Õâ¸ö·¿¼äà¸ \033[31m¡ò\033[m",
				roomstr);
			send_to_unum(unum, chatbuf);
			return;
		}
		if ((rooms[whichroom].flags & ROOM_SECRET) && !SYSOP(unum)) {
			send_to_unum(unum,
				     "\033[1;31m¡ò \033[37m±¾»áÒéÌüµÄ·¿¼ä½Ô¹«¿ªµÄ£¬Ã»ÓĞÃØÃÜ \033[31m¡ò\033[m");
			return;
		}
	}
	chat_do_user_list(unum, msg, whichroom);
}

void
chat_list_users(int unum, char *msg)
{
	chat_do_user_list(unum, msg, -1);
}

void
chat_map_chatids(int unum, int whichroom)
{
	int i, c, myroom, rnum;

	myroom = users[unum].room;
	send_to_unum(unum,
		     "\033[1;33;44m ÁÄÌì´úºÅ Ê¹ÓÃÕß´úºÅ  ©¦ ÁÄÌì´úºÅ Ê¹ÓÃÕß´úºÅ  ©¦ ÁÄÌì´úºÅ Ê¹ÓÃÕß´úºÅ \033[m");
	for (i = 0, c = 0; i < MAXACTIVE; i++) {
		rnum = users[i].room;
		if (users[i].sockfd != -1 && rnum != -1 &&
		    !(users[i].cloak == 1 && !CLOAK(unum))) {
			if (whichroom != -1 && whichroom != rnum)
				continue;
			if (myroom != rnum) {
				if (RESTRICTED(unum))
					continue;
				if ((rooms[rnum].flags & ROOM_SECRET) &&
				    !SYSOP(unum))
					continue;
			}
			snprintf(chatbuf + (c * 50), sizeof(chatbuf) - c * 50,
				"\033[1;34;5m%c\033[m\033[1m%-8s%c%s%-12s%s\033[m",
				(users[i].cloak == 1) ? 'C' : ' ',
				users[i].chatid, (ROOMOP(i)) ? '*' : ' ',
				OUTSIDER(i) ? "\033[1;35m" : "\033[1;36m",
				users[i].userid, (c < 2 ? "©¦" : "  "));
			if (++c == 3) {
				send_to_unum(unum, chatbuf);
				c = 0;
			}
		}
	}
	if (c > 0)
		send_to_unum(unum, chatbuf);
}

void
chat_map_chatids_thisroom(int unum, char *msg)
{
	chat_map_chatids(unum, users[unum].room);
}

void
chat_setroom(int unum, char *msg)
{
	char *modestr;
	int rnum = users[unum].room;
	int sign = 1;
	int flag;
	char *fstr;

	modestr = nextword(&msg);
	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if (*modestr == '+')
		modestr++;
	else if (*modestr == '-') {
		modestr++;
		sign = 0;
	}
	if (*modestr == '\0') {
		send_to_unum(unum,
			     "\033[1;31m¡Ñ \033[37mÇë¸æËß¹ñÌ¨ÄúÒªµÄ·¿¼äÊÇ: {[\033[32m+\033[37m(Éè¶¨)][\033[32m-\033[37m(È¡Ïû)]}{[\033[32ml\033[37m(Ëø×¡)][\033[32ms\033[37m(ÃØÃÜ)]}\033[m");
		return;
	}
	while (*modestr) {
		flag = 0;
		switch (*modestr) {
		case 'l':
		case 'L':
			flag = ROOM_LOCKED;
			fstr = "Ëø×¡";
			break;
		case 's':
		case 'S':
			flag = ROOM_SECRET;
			fstr = "»úÃÜ";
			break;
		case 'e':
		case 'E':
			flag = ROOM_NOEMOTE;
			fstr = "½ûÖ¹¶¯×÷";
			break;
		case 'a':
		case 'A':
			flag = ROOM_SCHOOL;
			fstr = "Ò»ÈËÌáÎÊ";
			break;
		default:
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;31m¡ò \033[37m±§Ç¸£¬¿´²»¶®ÄãµÄÒâË¼£º[\033[36m%c\033[37m] \033[31m¡ò\033[m", *modestr);
			send_to_unum(unum, chatbuf);
			return;
		}
		if (flag && ((rooms[rnum].flags & flag) != sign * flag)) {
			rooms[rnum].flags ^= flag;
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;37m¡ï\033[32m Õâ·¿¼ä±» %s %s%sµÄĞÎÊ½ \033[37m¡ï\033[m",
				users[unum].chatid, sign ? "Éè¶¨Îª" : "È¡Ïû", fstr);
			send_to_room(rnum, chatbuf);
		}
		modestr++;
	}
}

void
chat_nick(int unum, char *msg)
{
	char *chatid;
	int othernum;

	chatid = nextword(&msg);
	chatid[CHAT_IDLEN - 1] = '\0';
	if (!is_valid_chatid(chatid)) {
		send_to_unum(unum,
			     "\033[1;31m¡ò \033[37mÄúµÄÃû×ÖÊÇ²»ÊÇĞ´´íÁË\033[31m ¡ò\033[m");
		return;
	}
	othernum = chatid_to_indx(0, chatid);
	if (othernum != -1 && othernum != unum) {
		send_to_unum(unum,
			     "\033[1;31m¡ò \033[37m±§Ç¸£¡ÓĞÈË¸úÄãÍ¬Ãû£¬ËùÒÔÄã²»ÄÜ½øÀ´ \033[31m¡ò\033[m");
		return;
	}
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;31m¡ò \033[36m%s \033[0;37mÒÑ¾­¸ÄÃûÎª \033[1;33m%s \033[31m¡ò\033[m",
		users[unum].chatid, chatid);
	send_to_room(users[unum].room, chatbuf);
	strcpy(users[unum].chatid, chatid);
	snprintf(chatbuf, sizeof(chatbuf), "/n%s", users[unum].chatid);
	send_to_unum(unum, chatbuf);
}

void
chat_private(int unum, char *msg)
{
	char *recipient;
	int recunum;

	recipient = nextword(&msg);
	recunum = fuzzy_chatid_to_indx(unum, recipient);
	if (recunum < 0) {
		/* no such user, or ambiguous */
		if (recunum == -1) {
			snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, recipient);
		} else {
			strlcpy(chatbuf, "\033[1;31m ¡ò\033[37m ÄÇÎ»²ÎÓëÕß½ĞÊ²Ã´Ãû×Ö \033[31m¡ò\033[m", sizeof(chatbuf));
		}
		send_to_unum(unum, chatbuf);
		return;
	}
	if (!MESG(recunum)) {
		strlcpy(chatbuf, "\033[1;31m ¡ò\033[37m ¶Ô·½¹Ø±ÕÁË±ã¼ãÏä \033[31m¡ò\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
		return;
	}
	if (*msg) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;32m ¡ù \033[36m%s \033[37m´«Ö½ÌõĞ¡ÃØÊéÀ´µ½\033[m: ",
			users[unum].chatid);
		strlcat(chatbuf, msg, sizeof(chatbuf));
		send_to_unum(recunum, chatbuf);
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;32m ¡ù \033[37mÖ½ÌõÒÑ¾­½»¸øÁË \033[36m%s\033[m: ", users[recunum].chatid);
		strlcat(chatbuf, msg, sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
	} else {
		strlcpy(chatbuf, "\033[1;31m ¡ò\033[37m ÄãÒª¸ú¶Ô·½ËµĞ©Ê²Ã´Ñ½£¿\033[31m¡ò\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
	}
}

void
put_chatid(int unum, char *str)
{
	int i;
	char *chatid = users[unum].chatid;

	memset(str, ' ', 10);
	for (i = 0; chatid[i]; i++)
		str[i] = chatid[i];
	str[i] = ':';
	str[10] = '\0';
}

void
chat_allmsg(int unum, char *msg)
{
	if (*msg) {
		put_chatid(unum, chatbuf);
		strcat(chatbuf, msg);
		send_to_room(users[unum].room, chatbuf);
	}
}

void
chat_act(int unum, char *msg)
{
	if (NOEMOTE(users[unum].room)) {
		send_to_unum(unum, "±¾ÁÄÌìÊÒ½ûÖ¹¶¯×÷");
		return;
	}

	if (*msg) {
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[37m%s\033[m", users[unum].chatid, msg);
		send_to_room(users[unum].room, chatbuf);
	}
}

void
chat_cloak(int unum, char *msg)
{
	if (CLOAK(unum)) {
		if (users[unum].cloak == 1)
			users[unum].cloak = 0;
		else
			users[unum].cloak = 1;
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[37m%s ÒşÉí×´Ì¬...\033[m",
			users[unum].chatid,
			(users[unum].cloak == 1) ? "½øÈë" : "Í£Ö¹");
		send_to_unum(unum, chatbuf);
	}
}

void
chat_join(int unum, char *msg)
{
	char *roomid;

	roomid = nextword(&msg);
	if (RESTRICTED(unum)) {
		send_to_unum(unum,
			     "\033[1;31m¡ò \033[37mÄãÖ»ÄÜÔÚÕâÀïÁÄÌì \033[31m¡ò\033[m");
		return;
	}
	if (*roomid == '\0') {
		send_to_unum(unum, "\033[1;31m¡ò \033[37mÇëÎÊÄÄ¸ö·¿¼ä \033[31m¡ò\033[m");
		return;
	}
	if (!is_valid_chatid(roomid)) {
		send_to_unum(unum,
			     "\033[1;31m¡ò\033[37m·¿¼äÃûÖĞ²»ÄÜÓĞ¡¾*:/%¡¿µÈ²»ºÏ·¨×Ö·û\033[31m¡ò\033[37m");
		return;
	}
	enter_room(unum, roomid, msg);
	return;
}

void
chat_kick(int unum, char *msg)
{
	char *twit;
	int rnum = users[unum].room;
	int recunum;

	twit = nextword(&msg);
	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if ((recunum = chatid_to_indx(unum, twit)) == -1) {
		snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, twit);
		send_to_unum(unum, chatbuf);
		return;
	}
	if (rnum != users[recunum].room) {
		snprintf(chatbuf, sizeof(chatbuf), msg_not_here, users[recunum].chatid);
		send_to_unum(unum, chatbuf);
		return;
	}
	exit_room(recunum, EXIT_KICK, (char *) NULL);

	if (rnum == 0) {
		logout_user(recunum);
	} else {
		enter_room(recunum, mainroom, (char *) NULL);
	}
}

void
chat_makeop(int unum, char *msg)
{
	char *newop = nextword(&msg);
	int rnum = users[unum].room;
	int recunum;

	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if ((recunum = chatid_to_indx(unum, newop)) == -1) {
		/* no such user */
		snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, newop);
		send_to_unum(unum, chatbuf);
		return;
	}
	if (unum == recunum) {
		strlcpy(chatbuf, "\033[1;37m¡ï \033[32mÄãÍüÁËÄã±¾À´¾ÍÊÇÀÏ´óà¸ \033[37m¡ï\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
		return;
	}
	if (rnum != users[recunum].room) {
		snprintf(chatbuf, sizeof(chatbuf), msg_not_here, users[recunum].chatid);
		send_to_unum(unum, chatbuf);
		return;
	}
	users[unum].flags &= ~PERM_CHATROOM;
	users[recunum].flags |= PERM_CHATROOM;
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m¡ï \033[36m %s\033[32m¾ö¶¨ÈÃ \033[35m%s \033[32mµ±ÀÏ´ó \033[37m¡ï\033[m",
		users[unum].chatid, users[recunum].chatid);
	send_to_room(rnum, chatbuf);
}

void
chat_invite(int unum, char *msg)
{
	char *invitee = nextword(&msg);
	int rnum = users[unum].room;
	int recunum;

	if (!ROOMOP(unum) && !SYSOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}
	if ((recunum = chatid_to_indx(unum, invitee)) == -1) {
		snprintf(chatbuf, sizeof(chatbuf), msg_not_here, invitee);
		send_to_unum(unum, chatbuf);
		return;
	}
	if (rooms[rnum].invites[recunum] == 1) {
		snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m¡ï \033[36m%s \033[32mµÈÒ»ÏÂ¾ÍÀ´ \033[37m¡ï\033[m", users[recunum].chatid);
		send_to_unum(unum, chatbuf);
		return;
	}
	rooms[rnum].invites[recunum] = 1;
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1;37m¡ï \033[36m%s \033[32mÑûÇëÄúµ½ [\033[33m%s\033[32m] ·¿¼äÁÄÌì\033[37m ¡ï\033[m",
		users[unum].chatid, rooms[rnum].name);
	send_to_unum(recunum, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m¡ï \033[36m%s \033[32mµÈÒ»ÏÂ¾ÍÀ´ \033[37m¡ï\033[m", users[recunum].chatid);
	send_to_unum(unum, chatbuf);
}

void
chat_broadcast(int unum, char *msg)
{
	if (!SYSOP(unum)) {
		send_to_unum(unum,
			     "\033[1;31m¡ò \033[37mÄã²»¿ÉÒÔÔÚ»áÒéÌüÄÚ´óÉùĞú»© \033[31m¡ò\033[m");
		return;
	}
	if (*msg == '\0') {
		send_to_unum(unum, "\033[1;37m¡ï \033[32m¹ã²¥ÄÚÈİÊÇÊ²Ã´ \033[37m¡ï\033[m");
		return;
	}
	snprintf(chatbuf, sizeof(chatbuf),
		"\033[1m Ding Dong!! ´«´ïÊÒ±¨¸æ£º \033[36m%s\033[37m ÓĞ»°¶Ô´ó¼ÒĞû²¼£º\033[m",
		users[unum].chatid);
	send_to_room(ROOM_ALL, chatbuf);
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;34m¡¾\033[33m%s\033[34m¡¿\033[m", msg);
	send_to_room(ROOM_ALL, chatbuf);
}

void
chat_goodbye(int unum, char *msg)
{
	exit_room(unum, EXIT_LOGOUT, msg);
}

/* -------------------------------------------- */
/* MUD-like social commands : action             */
/* -------------------------------------------- */

struct action {
	char *verb;		/* ¶¯´Ê */
	char *part1_msg;	/* ½é´Ê */
	char *part2_msg;	/* ¶¯×÷ */
};

struct action party_data[] = {
	{"admire", "¶Ô", "µÄ¾°ÑöÖ®ÇéÓÌÈçÌÏÌÏ½­Ë®Á¬Ãà²»¾ø"},
	{"agree", "ÍêÈ«Í¬Òâ", "µÄ¿´·¨"},
	{"bearhug", "ÈÈÇéµÄÓµ±§", ""},
	{"ben", "Ğ¦ºÇºÇµØ¶Ô", "Ëµ£º¡°±¿, Á¬Õâ¶¼²»ÖªµÀ... :P¡±"},
	{"bless", "×£¸£", "ĞÄÏëÊÂ³É"},
	{"bow", "±Ï¹ª±Ï¾´µÄÏò", "¾Ï¹ª"},
	{"bye", "Ïò", "Ò»¹°ÊÖµÀ£º¡°ÇàÉ½ÒÀ¾É£¬ÅçË®³£À´£¬ÔÛÃÇÏÂ»ØÔÙ»á!¡±"},
	{"bye1", "¿Ş¿ŞÌäÌäµØÀ­×Å", "µÄÒÂ½Ç:¡°Éá²»µÃÄã×ß°¡...¡±"},
	{"bye2", "ÆàÍñµØ¶Ô",
	 "ËµµÀ:¡°ÊÀÉÏÃ»ÓĞ²»É¢µÄÑçÏ¯£¬ÎÒÏÈ×ßÒ»²½ÁË£¬´ó¼Ò¶à±£ÖØ.¡±"},
	{"byebye", "Íû×Å", "ÀëÈ¥µÄ±³Ó°½¥½¥ÏûÊ§£¬Á½µÎ¾§Ó¨µÄÀá»¨´ÓÈù±ß»¬Âä"},
	{"bite", "ºİºİµÄÒ§ÁË", "Ò»¿Ú£¬°ÑËûÍ´µÃÍÛÍÛ´ó½Ğ...ÕæË¬,¹ş¹ş£¡"},
	{"blink", "¼Ù×°Ê²Ã´Ò²²»ÖªµÀ£¬¶Ô", "ÌìÕæµØÕ£ÁËÕ£ÑÛ¾¦"},
	{"breath", ":¸Ï¿ì¸ø", "×öÈË¹¤ºôÎü!"},
	{"brother", "¶Ô",
	 "Ò»±§È­,µÀ:¡°ÄÑµÃÄãÎÒ¸Îµ¨ÏàÕÕ,²»ÈôÄãÎÒÒå½á½ğÀ¼,ÒâÏÂÈçºÎ?¡±"},
	{"bigfoot", "Éì³ö´ó½Å£¬¶Ô×¼", "µÄÈı´ç½ğÁ«ºİºİµØ²ÈÏÂÈ¥"},
	{"consider", "¿ªÊ¼ÈÏÕæ¿¼ÂÇÉ±ËÀ", "µÄ¿ÉÄÜĞÔºÍ¼¼ÊõÄÑ¶È"},
	{"caress", "¸§Ãş", ""},
	{"cringe", "Ïò", "±°¹ªÇüÏ¥£¬Ò¡Î²ÆòÁ¯"},
	{"cry", "Ïò", "º¿ßû´ó¿Ş"},
	{"cry1", "Ô½ÏëÔ½ÉËĞÄ£¬²»½ûÅ¿ÔÚ", "µÄ¼ç°òÉÏº¿ßû´ó¿ŞÆğÀ´"},
	{"cry2", "ÆËÔÚ",
	 "ÉíÉÏ£¬¿ŞµÃËÀÈ¥»îÀ´:¡°±ğ×ßÑ½!¸ÕÅİµÄ·½±ãÃæÄã»¹Ã»³ÔÍêÄØ!¡±"},
	{"cold", "ÌıÁË", "µÄ»°£¬ÀäµÃÖ±Æğ¼¦Æ¤¸í´ñ"},
	{"comfort", "ÎÂÑÔ°²Î¿", ""},
	{"clap", "Ïò", "ÈÈÁÒ¹ÄÕÆ"},
	{"dance", "À­ÁË", "µÄÊÖôæôæÆğÎè"},
	{"die", "ºÜ¿áµØÌÍ³öÒ»°Ñ·ÀĞâË®Ç¹£¬¡¸Åö!¡¹Ò»Ç¹°Ñ", "¸ø±ĞÁË."},
	{"dog", "¹ØÃÅ¡¢·Å¹·£¡ °Ñ", "Ò§µÄÂä»¨Á÷Ë®"},
	{"dogleg", "ÊÇÕı×Ú¹·ÍÈÍõ¡«ÍôÍô¡«, Ê¹¾¢ÅÄÅÄ", "µÄÂíÆ¨"},
	{"dontno", "¶Ô", "Ò¡Ò¡Í·ËµµÀ: ¡°Õâ¸ö...ÎÒ²»ÖªµÀ...¡±"},
	{"dontdo", "¶Ô", "Ëµ:¡°Ğ¡ÅóÓÑ£¬ ²»¿ÉÒÔÕâÑùÅ¶£¬ÕâÑù×öÊÇ²»ºÃµÄÓ´¡±"},
	{"drivel", "¶ÔÖø", "Á÷¿ÚË®"},
	{"dunno", "µÉ´óÑÛ¾¦£¬ÌìÕæµØÎÊ£º", "£¬ÄãËµÊ²Ã´ÎÒ²»¶®Ò®... :("},
	{"dlook", "´ô´ôµØÍû×Å", "£¬¿ÚË®»©À²À²µØÁ÷ÁËÒ»µØ"},
	{"dove", "¸øÁË", "Ò»¿éDOVE£¬Ëµ:¡°ÄÅ£¬¸øÄãÒ»¿éDOVE£¬ÒªºÃºÃÌı»°Å¶¡±"},
	{"emlook", "ÉÏÏÂ¶ËÏêÁË",
	 "Á½ÑÛ¡°¾ÍÄãĞ¡×ÓÑ½,ÎÒµ±ÊÇË­ÄØ,Ò²¸ÒÔÚÕâÀïÈöÒ°!¡±"},
	{"face", "ÍçÆ¤µÄ¶Ô", "×öÁË¸ö¹íÁ³."},
	{"faceless", "¶Ô×Å", "´ó½ĞµÀ£º¡°ºÙºÙ, ÄãµÄÃæ×ÓÂô¶àÉÙÇ®Ò»½ï?¡±"},
	{"farewell", "º¬Àá,ÒÀÒÀ²»ÉáµØÏò", "µÀ±ğ"},
	{"fear", "¶Ô", "Â¶³öÅÂÅÂµÄ±íÇé"},
	{"flook", "³Õ³ÕµÄÍû×Å", ", ÄÇÉîÇéµÄÑÛÉñËµÃ÷ÁËÒ»ÇĞ."},
	{"forgive", "´ó¶ÈµÄ¶Ô", "Ëµ£ºËãÁË£¬Ô­ÁÂÄãÁË"},
	{"giggle", "¶ÔÖø", "ÉµÉµµÄ´ôĞ¦"},
	{"grin", "¶Ô", "Â¶³öĞ°¶ñµÄĞ¦Èİ"},
	{"growl", "¶Ô", "ÅØÏø²»ÒÑ"},
	{"hammer", "¾ÙÆğ»İÏãµÄ50000000TÌú´¸Íù$", "ÉÏÓÃÁ¦Ò»ÇÃ!,*** ¡¡¼ ïÏ ¡½!"},
	{"hand", "¸ú", "ÎÕÊÖ"},
	{"heng", "¿´¶¼²»¿´",
	 "Ò»ÑÛ£¬ ºßÁËÒ»Éù£¬¸ß¸ßµÄ°ÑÍ·ÑïÆğÀ´ÁË,²»Ğ¼Ò»¹ËµÄÑù×Ó..."},
	{"hi", "¶Ô", "ºÜÓĞÀñÃ²µØËµÁËÒ»Éù£º¡°Hi! ÄãºÃ!¡±"},
	{"hug", "ÇáÇáµØÓµ±§", ""},
	{"kick", "°Ñ", "ÌßµÄËÀÈ¥»îÀ´"},
	{"kiss", "ÇáÎÇ", "µÄÁ³¼Õ"},
	{"laugh", "´óÉù³°Ğ¦", ""},
	{"look", "ÔôÔôµØ¿´×Å", "£¬²»ÖªµÀÔÚ´òÊ²Ã´âÈÖ÷Òâ¡£"},
	{"love", "ÉîÇéµØÍû×Å", "£¬·¢ÏÖ×Ô¼º°®ÉÏÁËta"},
	{"love2", "¶Ô", "ÇéÉî¿î¿îµÄËµ£ºÔÚÌìÔ¸ÎªŒI¹«×Ğ£¬ÔÚµØÔ¸Îª<ÓÍÕ¨¹í>"},
	{"lovelook", "Ò»Ë«Ë®ÍôÍôµÄ´óÑÛ¾¦º¬ÇéÂöÂöµØ¿´×Å", "!"},
	{"missyou", "ÌğÌğÒ»Ğ¦£¬ÑÛÖĞÈ´Á÷ÏÂÑÛÀá:", "£¬ÕæµÄÊÇÄãÂğ£¬Õâ²»ÊÇ×÷ÃÎ£¿"},
	{"meet", "¶Ô", "Ò»±§È­£¬ËµµÀ£º¡°¾ÃÎÅ´óÃû£¬½ñÈÕÒ»¼û£¬ÕæÊÇÈıÉúÓĞĞÒ£¡¡±"},
	{"moon", "À­×Å",
	 "µÄĞ¡ÊÖ£¬Ö¸×Å³õÉıµÄÔÂÁÁËµ£º¡°ÌìÉÏÃ÷ÔÂ£¬ÊÇÎÒÃÇµÄÖ¤ÈË¡±"},
	{"nod", "Ïò", "µãÍ·³ÆÊÇ"},
	{"nothing", "¸ÏÃ¦·öÆğ", "£ºĞ¡ÊÂÒ»×®£¬ºÎ×ã¹Ò³İ£¿"},
	{"nudge", "ÓÃÊÖÖâ¶¥", "µÄ·Ê¶Ç×Ó"},
	{"oh", "¶Ô", "Ëµ£º¡°Å¶£¬½´×Ó°¡£¡¡±"},
	{"pad", "ÇáÅÄ", "µÄ¼ç°ò"},
	{"papa", "½ôÕÅµØ¶Ô", "Ëµ£º¡°ÎÒºÃÅÂÅÂÅ¶...¡±"},
	{"papaya", "ÇÃÁËÇÃ", "µÄÄ¾¹ÏÄÔ´ü"},
	{"praise", "¶Ô", "ËµµÀ: ¹ûÈ»¸ßÃ÷! Åå·şÅå·ş!"},
	{"pinch", "ÓÃÁ¦µÄ°Ñ", "Å¡µÄºÚÇà"},
	{"poor", "À­×Å", "µÄÊÖËµ£º¡°¿ÉÁ¯µÄº¢×Ó£¡¡±ÑÛÀáà§à§µØµôÁËÏÂÀ´....."},
	{"punch", "ºİºİ×áÁË", "Ò»¶Ù"},
	{"puke", "¶Ô×Å", "ÍÂ°¡ÍÂ°¡£¬¾İËµÍÂ¶à¼¸´Î¾ÍÏ°¹ßÁË"},
	{"pure", "¶Ô", "Â¶³ö´¿ÕæµÄĞ¦Èİ"},
	{"qmarry", "Ïò", "ÓÂ¸ÒµØ¹òÁËÏÂÀ´£ºÄãÔ¸Òâ¼Ş¸øÎÒÂğ---ÕæÊÇÓÂÆø¿É¼Î°¡!"},
	{"report", "ÍµÍµµØ¶Ô", "Ëµ£º¡°±¨¸æÎÒºÃÂğ£¿¡±"},
	{"rose", "Í»È»´ÓÉíºóÄÃ³öÒ»¶ä-`-,-<@ ÉîÇéµØÏ×¸ø", "£¡"},
	{"rose999", "¶Ô", "³ªµÀ£º¡°ÎÒÒÑ¾­ÎªÄãÖÖÏÂ£¬¾Å°Ù¾ÅÊ®¾Å¶äÃµ¹å¡­¡­¡±"},
	{"run", "Æø´­ÓõÓõµØ¶Ô",
	 "Ëµ:ÎÒ»»ÁË°ËÆ¥¿ìÂíÈÕÒ¹¼æ³Ì¸ÏÀ´.ÄÜÔÙ¼ûµ½ÄãËÀÁËÒ²ĞÄ¸Ê"},
	{"shrug", "ÎŞÄÎµØÏò", "ËÊÁËËÊ¼ç°ò"},
	{"sigh", "¶Ô", "Ì¾ÁËÒ»¿ÚÆø,µÀ: Ôø¾­²×º£ÄÑÎªË®,³ıÈ´Î×É½²»ÊÇÔÆ..."},
	{"slap", "Å¾Å¾µÄ°ÍÁË", "Ò»¶Ù¶ú¹â"},
	{"smooch", "ÓµÎÇÖø", ""},
	{"snicker", "ºÙºÙºÙ..µÄ¶Ô", "ÇÔĞ¦"},
	{"sniff", "¶Ô", "àÍÖ®ÒÔ±Ç"},
	{"spank", "ÓÃ°ÍÕÆ´ò", "µÄÍÎ²¿"},
	{"squeeze", "½ô½ôµØÓµ±§Öø", ""},
	{"sorry", "¸Ğµ½¶Ô", "Ê®¶şÍò·ÖµÄÇ¸Òâ, ÓÚÊÇµÍÉùËµµÀ:ÎÒ¸Ğµ½·Ç³£µÄ±§Ç¸!"},
	{"thank", "Ïò", "µÀĞ»"},
	{"thanks", "¹òÔÚµØÉÏ¶Ô", "¹§¾´µÄ¿ÄÁË¼¸¸öÍ·:ÄãµÄ´ó¶÷´óµÂ£¬Ã»³İÄÑÍö!"},
	{"tickle", "¹¾ß´!¹¾ß´!É¦", "µÄÑ÷"},
	{"wake", "Å¬Á¦µÄÒ¡Ò¡", "£¬ÔÚÆä¶ú±ß´ó½Ğ£º¡°¿ìĞÑĞÑ£¬»á×ÅÁ¹µÄ£¡¡±"},
	{"wakeup", "Ò¡Öø",
	 "£¬ÊÔÖø°ÑËû½ĞĞÑ¡£´óÉùÔÚËû¶ú±ß´ó½Ğ£º¡¸ Öí! ÆğÀ´ÁË! ¡¹"},
	{"wave", "¶ÔÖø", "Æ´ÃüµÄÒ¡ÊÖ"},
	{"welcome", "ÈÈÁÒ»¶Ó­", "µÄµ½À´"},
	{"wing", "ÄÃ×ÅÒ»ºĞÎ¬ËüÄÌÉµÉµµÄËµ£ºÄã¡£¡£¾ÍÊÇ", "Âğ£¿"},
	{"wink", "¶Ô", "ÉñÃØµÄÕ£Õ£ÑÛ¾¦"},
	{"zap", "¶Ô", "·è¿ñµÄ¹¥»÷"},
	{"xinku", "¸Ğ¶¯µÃÈÈÀáÓ¯¿ô£¬Ïò×Å", "Õñ±Û¸ßºô£º¡°Ê×³¤ĞÁ¿àÁË£¡¡±"},
	{"bupa", "°®Á¯µØÃş×Å",
	 "µÄÍ·,Ëµ:¡°Ğ¡ÃÃÃÃ,²»ÅÂ,ÊÜÁËÊ²Ã´Î¯Çü´ó¸ç¸çÌæÄã±¨³ğ!¡±"}, {"gril",
								  "¸Ï½ô¸ø",
								  "´·´·±³,ĞÄÏë:º¢×ÓĞ¡,±ğ²í¹ıÆøÈ¥."},
	{":)..", "¶Ô", "´¹ÏÑÈı³ß£¬²»ÖªµÀÏÂÒ»²½»áÓĞºÎ¾Ù¶¯"},
	{"?", "ºÜÒÉ»óµÄ¿´×Å", ""},
	{"@@", "Õö´óÁËÑÛ¾¦¾ªÆæµØ¶¢×Å", ":¡°Õâ...ÕâÒ²Ì«....¡±"},
	{"@@!", "ºİºİµØµÉÁË", "Ò»ÑÛ, ËûÁ¢¿Ì±»¿´µÃËõĞ¡ÁËÒ»°ë"},
	{"mail", "¸ø", "´ÓÃÀ¹ú·¢ÁËÒ»·âÓĞ°×É«·ÛÄ©µÄÓÊ¼ş"},
	{NULL, NULL, NULL}
};

int
party_action(int unum, char *cmd, char *party, int self)
{
	int i;

	for (i = 0; party_data[i].verb; i++) {
		if (!strcmp(cmd, party_data[i].verb)) {
			if (*party == '\0') {
				party = "´ó¼Ò";
			} else {
				int recunum = fuzzy_chatid_to_indx(unum, party);

				if (recunum < 0) {
					/* no such user, or ambiguous */
					if (recunum == -1) {
						snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, party);
					} else {
						strlcpy(chatbuf, "\033[1;31m¡ò \033[37mÇëÎÊÄÄ¼ä·¿¼ä \033[31m¡ò\033[m", sizeof(chatbuf));
					}
					send_to_unum(unum, chatbuf);
					return 0;
				}
				party = users[recunum].chatid;
			}
			snprintf(chatbuf, sizeof(chatbuf),
				"\033[1;36m%s \033[32m%s\033[33m %s \033[32m%s\033[37;0m",
				users[unum].chatid, party_data[i].part1_msg,
				party, party_data[i].part2_msg);
			if (YEA == self) {
				send_to_unum(unum, "\033[1;37m¶¯×÷ÑİÊ¾£º\033[m");
				send_to_unum(unum, chatbuf);
				send_to_unum(unum, "");
			} else {
				send_to_room(users[unum].room, chatbuf);
			}
			return 0;
		}
	}
	return 1;
}

/* -------------------------------------------- */
/* MUD-like social commands : speak              */
/* -------------------------------------------- */

struct action speak_data[] = {
	{"ask", "Ñ¯ÎÊ", NULL},
	{"chant", "¸èËÌ", NULL},
	{"cheer", "ºÈ²É", NULL},
	{"chuckle", "ÇáĞ¦", NULL},
	{"curse", "ÖäÂî", NULL},
	{"demand", "ÒªÇó", NULL},
	{"frown", "õ¾Ã¼", NULL},
	{"groan", "ÉëÒ÷", NULL},
	{"grumble", "·¢ÀÎÉ§", NULL},
	{"hum", "à«à«×ÔÓï", NULL},
	{"moan", "±¯Ì¾", NULL},
	{"notice", "×¢Òâ", NULL},
	{"order", "ÃüÁî", NULL},
	{"ponder", "ÉòË¼", NULL},
	{"pout", "àÙÖø×ìËµ", NULL},
	{"pray", "Æíµ»", NULL},
	{"request", "¿ÒÇó", NULL},
	{"shout", "´ó½Ğ", NULL},
	{"sing", "³ª¸è", NULL},
	{"smile", "Î¢Ğ¦", NULL},
	{"smirk", "¼ÙĞ¦", NULL},
	{"swear", "·¢ÊÄ", NULL},
	{"tease", "³°Ğ¦", NULL},
	{"whimper", "ÎØÑÊµÄËµ", NULL},
	{"yawn", "¹şÇ·Á¬Ìì", NULL},
	{"yell", "´óº°", NULL},
	{NULL, NULL, NULL}
};

int
speak_action(int unum, char *cmd, char *msg, int self)
{
	int i;

	for (i = 0; speak_data[i].verb; i++) {
		if (!strcmp(cmd, speak_data[i].verb)) {
			snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[32m%s£º\033[33m %s\033[37;0m",
				users[unum].chatid, speak_data[i].part1_msg, msg);
			if (YEA == self) {
				send_to_unum(unum, "\033[1;37m¶¯×÷ÑİÊ¾£º\033[m");
				send_to_unum(unum, chatbuf);
				send_to_unum(unum, "");
			} else {
				send_to_room(users[unum].room, chatbuf);
			}
			return 0;
		}
	}
	return 1;
}

/* -------------------------------------------- */
/* MUD-like social commands : condition          */
/* -------------------------------------------- */

struct action condition_data[] = {
	{"acid", "ËµµÀ£º¡°ºÃÈâÂïà¡~~~¡±", NULL},
	{"addoil", "Íû¿Õ¸ßº°: ¼ÓÓÍ!!", NULL},
	{"applaud", "Å¾Å¾Å¾Å¾Å¾Å¾Å¾....", NULL},
	{"blush", "Á³¶¼ºìÁË", NULL},
	{"boss", "·ÅÉù´ó½Ğ£ºÍÛÈû£¬²»µÃÁË£¬ÀÏ°åÀ´ÁË£¬ÎÒÒªÌÓÁË£¬ÔÙ¼ûÁË¸÷Î».",
	 NULL},
	{"bug", "´óÉùËµ¡°±¨¸æÕ¾³¤£¬ÎÒ×¥µ½Ò»Ö»³æ×Ó¡±¡£", NULL},
	{"cool", "´ó½ĞÆğÀ´£ºÍÛÈû¡«¡«¡«ºÃcoolÅ¶¡«¡«", NULL},
	{"cough", "¿ÈÁË¼¸Éù", NULL},
	{"die", "¿ÚÍÂ°×Ä­£¬Ë«ÑÛÒ»·­£¬ÉíÌå´¤ÁË¼¸ÏÂ£¬²»¶¯ÁË£¬ÄãÒ»¿´ËÀÁË¡£", NULL},
	{"goeat", "µÄ¶Ç×ÓÓÖ¹¾¹¾µÄ½ĞÁË,°¦,²»µÃ²»È¥³ÔÊ³ÌÃÄÇ#$%&µÄ·¹²ËÁË", NULL},
	{"faint", "ßÛµ±Ò»Éù£¬ÔÎµ¹ÔÚµØ...", NULL},
	{"faint1", "¿ÚÍÂ°×Ä­£¬»èµ¹ÔÚµØÉÏ¡£", NULL},
	{"faint2", "¿ÚÖĞ¿ñÅçÏÊÑª£¬·­Éíµ¹ÔÚµØÉÏ¡£", NULL},
	{"haha", "¹ş¹ş¹ş¹ş......", NULL},
	{"handup", "Æ´ÃüµØÉì³¤×Ô¼ºµÄÊÖ±Û£¬¸ßÉù½ĞµÀ£º¡°ÎÒ£¬ÎÒ£¬ÎÒ£¡¡±", NULL},
	{"happy", "r-o-O-m....ÌıÁËÕæË¬£¡", NULL},
	{"heihei", "ÀäĞ¦Ò»Éù", NULL},
	{"jump", "¸ßĞËµØÏñĞ¡º¢×ÓËÆµÄ£¬ÔÚÁÄÌìÊÒÀï±Ä±ÄÌøÌø¡£", NULL},
	{"pavid", "Á³É«²Ô°×!ºÃÏñ¾åÅÂÊ²Ã´!", NULL},
	{"puke", "Õæ¶ñĞÄ£¬ÎÒÌıÁË¶¼ÏëÍÂ", NULL},
	{"shake", "Ò¡ÁËÒ¡Í·", NULL},
	{"sleep", "Zzzzzzzzzz£¬ÕæÎŞÁÄ£¬¶¼¿ìË¯ÖøÁË", NULL},
	{"so", "¾Í½´×Ó!!", NULL},
	{"strut", "´óÒ¡´ó°ÚµØ×ß", NULL},
	{"suicide", "ÕæÏëÂò¿é¶¹¸¯À´Ò»Í·×²ËÀ, ÃşÁËÃşÉí±ßÓÖÃ»ÁãÇ®, Ö»ºÃÈÌÒ»ÊÖ",
	 NULL},
	{"toe", "¾õµÃÕæÊÇÎŞÁÄ, ÓÚÊÇ×¨ĞÄµØÍæÆğ×Ô¼ºµÄ½ÅÖºÍ·À´", NULL},
	{"tongue", "ÍÂÁËÍÂÉàÍ·", NULL},
	{"think", "ÍáÖøÍ·ÏëÁËÒ»ÏÂ", NULL},
	{"wa", "ÍÛ£¡:-O", NULL},
	{"wawl", "¾ªÌì¶¯µØµÄ¿Ş", NULL},
	{"xixi", "ÍµĞ¦£ºÎûÎû¡«¡«", NULL},
	{"ya", "µÃÒâµÄ×÷³öÊ¤ÀûµÄÊÖÊÆ! ¡¸ V ¡¹:¡° ¹ş¹ş¹ş...¡±", NULL},
	{":(", "³îÃ¼¿àÁ³µÄ,Ò»¸±µ¹Ã¹Ñù", NULL},
	{":)", "µÄÁ³ÉÏÂ¶³öÓä¿ìµÄ±íÇé.", NULL},
	{":D", "ÀÖµÄºÏ²»Â£×ì", NULL},
	{":P", "ÍÂÁËÍÂÉàÍ·,²îµãÒ§µ½×Ô¼º", NULL},
	{"@@", "ÒªËÀ²ËÁË %^#@%$#&^*&(&^$#%$#@(*&()*)_*&(#@%$^%&^.", NULL},
	{"sing1", "Á¹·çÓĞĞÅ£¬ÇïÔÂÎŞ±ß¡£¡£¡£¡£¡£¡£", NULL},
	{"mail", "¸ø", "´ÓÃÀ¹ú·¢ÁËÒ»·âÓĞ°×É«·ÛÄ©µÄÓÊ¼ş¡£"},
	{"wonder", "»³ÒÉµÄËµ£ºÊÇ²»ÊÇÕæµÄ£¿", NULL},
	{"apache", "×ø×Å°¢ÅÁÆæÖ±Éı»úÀë¿ª¡£ÕæÊÇ¿á´ôÁË£¡", NULL},
	{"parapara", "µÄÊÖÔÚ¿ÕÖĞÂÒÎè£¬ÄÇÑù×ÓËÆºõÔÚÌøparapara¡£", NULL},
	{NULL, NULL, NULL}
};

int
condition_action(int unum, char *cmd, int self)
{
	int i;

	for (i = 0; condition_data[i].verb; i++) {
		if (!strcmp(cmd, condition_data[i].verb)) {
			snprintf(chatbuf, sizeof(chatbuf), "\033[1;36m%s \033[33m%s\033[37;0m",
				users[unum].chatid, condition_data[i].part1_msg);
			if (YEA == self) {
				send_to_unum(unum, "\033[1;37m¶¯×÷ÑİÊ¾£º\033[m");
				send_to_unum(unum, chatbuf);
				send_to_unum(unum, "");
			} else {
				send_to_room(users[unum].room, chatbuf);
			}
			return 1;
		}
	}
	return 0;
}

/* -------------------------------------------- */
/* MUD-like social commands : help               */
/* -------------------------------------------- */

char *dscrb[] = {
	"\033[1m¡¾ Verb + Nick£º   ¶¯´Ê + ¶Ô·½Ãû×Ö ¡¿\033[m",
	"\033[1m¡¾ Verb + Message£º¶¯´Ê + ÒªËµµÄ»° ¡¿\033[m",
	"\033[1m¡¾ Verb£º¶¯´Ê ¡¿\033[m", NULL
};
struct action *verbs[] = { party_data, speak_data, condition_data, NULL };

#define SCREEN_WIDTH    80
#define MAX_VERB_LEN    10
#define VERB_NO         8

/* monster: following function was written and enhanced by me */

void
view_action_verb(int unum, char *id)
{
	int i, j;
	char *p;

	i = atoi(id) - 1;
	if (i >= 0 && i <= 2) {
		j = 0;
		chatbuf[0] = '\0';
		send_to_unum(unum, dscrb[i]);
		while ((p = verbs[i][j++].verb)) {
			strcat(chatbuf, p);
			if ((j % VERB_NO) == 0) {
				send_to_unum(unum, chatbuf);
				chatbuf[0] = '\0';
			} else {
				strncat(chatbuf, "        ",
					MAX_VERB_LEN - strlen(p));
			}
		}
		if (j % VERB_NO)
			send_to_unum(unum, chatbuf);

		send_to_unum(unum, " ");
		return;
	}

	if (party_action(unum, id, "", YEA) == 0)
		return;
	if (speak_action(unum, id, "Äã°®ÎÒÀ´ÎÒ°®Äã", YEA) == 0)
		return;
	if (condition_action(unum, id, YEA) == 1)
		return;

	send_to_unum(unum, "\033[1;37mÓÃ //help [±àºÅ] »ñµÃ¶¯×÷ÁĞ±í£¬ÓÃ //help [¶¯×÷] ¹Û¿´¶¯×÷Ê¾·¶\033[m");
	send_to_unum(unum, "  ");
	for (i = 0; dscrb[i]; i++) {
		snprintf(chatbuf, sizeof(chatbuf), "%d. %s", i + 1, dscrb[i]);
		send_to_unum(unum, chatbuf);
	}
	send_to_unum(unum, " ");
}

/* Henry: Ôö¼ÓÒ»ÈËÌáÎÊµÄÊµÏÖ£¬ÌáÎÊÇ°ĞèÒª»ñµÃÂó¿Ë·ç^_^ */

void
chat_maketalk(int unum, char *msg)
{
	char *newop = nextword(&msg);
	int recunum;

	if (!SYSOP(unum) && !ROOMOP(unum)) {
		send_to_unum(unum, msg_not_op);
		return;
	}

	if ((recunum = chatid_to_indx(unum, newop)) == -1) {
		/* no such user */
		snprintf(chatbuf, sizeof(chatbuf), msg_no_such_id, newop);
		send_to_unum(unum, chatbuf);
		return;
	}

	if (unum == recunum) {
		strlcpy(chatbuf, "\033[1;37m¡ï \033[32mÄãÊÇÀÏ´ó°¡£¬ÓĞ»°Ö±½ÓËµ¾ÍĞĞÁË \033[37m¡ï\033[m", sizeof(chatbuf));
		send_to_unum(unum, chatbuf);
		return;
	}

	users[recunum].flags ^= PERM_TALK;
	if (users[recunum].flags & PERM_TALK) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;37m¡ï \033[36m%s \033[32m¾ö¶¨°ÑÂó¿Ë·ç½»¸ø \033[35m%s \033[32m \033[37m¡ï\033[m",
			users[unum].chatid, users[recunum].chatid);
	} else {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;37m¡ï \033[36m%s \033[32m¾ö¶¨Ã»ÊÕ \033[35m%s \033[32mµÄÂó¿Ë·ç  \033[37m¡ï\033[m",
			users[unum].chatid, users[recunum].chatid);
	}
	send_to_room(users[recunum].room, chatbuf);
}

void
chat_hand(int unum, char *msg)
{
	int i;

	if (!NOTALK(users[unum].room)) {
		send_to_unum(unum, "\033[1;31m ¡ò\033[37m ÇëÄã³©ËùÓûÑÔ£¬ÎŞĞè¾ÙÊÖ \033[31m¡ò\033[m");
		return;
	}

	for (i = 0; i < MAXACTIVE; i++) {
		if (SYSOP(i) || ROOMOP(i)) {
			snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m¡ï \033[36m%s \033[32m¾ÙÆğÁËÊÖ£¬ºÃÏñÏëËµÊ²Ã´µÄÑù×Ó \033[37m¡ï\033[m", users[unum].chatid);
			send_to_unum(i, chatbuf);
		}
	}

	snprintf(chatbuf, sizeof(chatbuf), "\033[1;37m¡ï \033[36m%s \033[32mÏòÀÏ´óÃÇ¾ÙÊÖÊ¾Òâ \033[37m¡ï\033[m", users[unum].chatid);
	send_to_unum(unum, chatbuf);
}

/* Henry: Ôö¼Ó¹¦ÄÜ±ã¼ãÏä£¬¿ÉÒÔÇĞ»»×´Ì¬ÊÇ·ñ½ÓÊÕÖ½Ìõ */

void chat_paper(int unum, char *msg)
{
	users[unum].flags ^= PERM_MESSAGE;
	snprintf(chatbuf, sizeof(chatbuf), "\033[1;31m¡ò \033[37mÄúµÄ±ã¼ãÏäµÄ×´Ì¬ÇĞ»»Îª [%s] \033[31m¡ò\033[m",
		MESG(unum) ? "¿ªÆô" : "¹Ø±Õ" );
	send_to_unum(unum, chatbuf);
}

struct chatcmd chatcmdlist[] = {
	{ "act", 	chat_act, 			0 },
	{ "bye", 	chat_goodbye, 			0 },
	{ "flags", 	chat_setroom, 			0 },
	{ "invite", 	chat_invite, 			0 },
	{ "join", 	chat_join, 			0 },
	{ "kick", 	chat_kick, 			0 },
	{ "msg", 	chat_private, 			0 },
	{ "nick", 	chat_nick, 			0 },
	{ "operator", 	chat_makeop, 			0 },
	{ "talk", 	chat_maketalk,			0 },
	{ "rooms", 	chat_list_rooms, 		0 },
	{ "whoin", 	chat_list_by_room, 		1 },
	{ "wall", 	chat_broadcast, 		1 },
	{ "cloak", 	chat_cloak, 			1 },
	{ "who", 	chat_map_chatids_thisroom, 	0 },
	{ "list", 	chat_list_users, 		0 },
	{ "topic", 	chat_topic, 			0 },
	{ "hand", 	chat_hand,			0 },
	{ "rname", 	chat_rename, 			0 },
	{ "paper",	chat_paper,			0 },
	{ NULL, 	NULL, 				0 }
};

int
command_execute(int unum)
{
	char *msg = users[unum].ibuf;
	char *cmd;
	struct chatcmd *cmdrec;
	int match = 0;

	/* Validation routine */
	if (users[unum].room == -1) {
		/* MUST give special /! command if not in the room yet */
		if (msg[0] != '/' || msg[1] != '!')
			return -1;
		else
			return (login_user(unum, msg + 2));
	}
	/* If not a /-command, it goes to the room. */
	if (msg[0] != '/') {
		if (NOTALK(users[unum].room) && !SYSOP(unum) && !ROOMOP(unum) && !CANTALK(unum)) {
			strlcpy(chatbuf, "\033[1;31m ¡ò\033[37m ÄãÏÖÔÚÃ»ÓĞÂó¿Ë·ç£¬ÎŞ·¨Ëµ»° \033[31m¡ò\033[m", sizeof(chatbuf));
			send_to_unum(unum, chatbuf);
			return 0;
		}
		chat_allmsg(unum, msg);
		return 0;
	}
	msg++;
	cmd = nextword(&msg);

	if (cmd[0] == '/') {
		if (!strcmp(cmd + 1, "help") || cmd[1] == '\0') {
			cmd = nextword(&msg);
			view_action_verb(unum, cmd);
			match = 1;
		} else if (NOEMOTE(users[unum].room)) {
			send_to_unum(unum, "±¾ÁÄÌìÊÒ½ûÖ¹¶¯×÷");
			match = 1;
		} else if (party_action(unum, cmd + 1, msg, NA) == 0)
			match = 1;
		else if (speak_action(unum, cmd + 1, msg, NA) == 0)
			match = 1;
		else
			match = condition_action(unum, cmd + 1, NA);
	} else {
		for (cmdrec = chatcmdlist; !match && cmdrec->cmdstr; cmdrec++) {
			if (cmdrec->exact)
				match = !strcasecmp(cmd, cmdrec->cmdstr);
			else
				match =
				    !strncasecmp(cmd, cmdrec->cmdstr,
						 strlen(cmd));
			if (match)
				cmdrec->cmdfunc(unum, msg);
		}
	}

	if (!match) {
		snprintf(chatbuf, sizeof(chatbuf),
			"\033[1;32m ¡ò \033[37m±§Ç¸£¬¿´²»¶®ÄãµÄÒâË¼£º\033[36m/%s \033[31m¡ò\033[m", cmd);
		send_to_unum(unum, chatbuf);
	}
	memset(users[unum].ibuf, 0, sizeof (users[unum].ibuf));
	return 0;
}

int
process_chat_command(int unum)
{
	int i;
	int rc, ibufsize;

	if ((rc = recv(users[unum].sockfd, chatbuf, sizeof (chatbuf), 0)) <= 0) {
		/* disconnected */
		exit_room(unum, EXIT_LOSTCONN, (char *) NULL);
		return -1;
	}
	ibufsize = users[unum].ibufsize;
	for (i = 0; i < rc; i++) {
		/* if newline is two characters, throw out the first */
		if (chatbuf[i] == '\r')
			continue;

		/* carriage return signals end of line */
		else if (chatbuf[i] == '\n') {
			users[unum].ibuf[ibufsize] = '\0';
			if (command_execute(unum) == -1)
				return -1;
			ibufsize = 0;
		}
		/* add other chars to input buffer unless size limit exceeded */
		else {
			if (ibufsize < 127)
				users[unum].ibuf[ibufsize++] = chatbuf[i];
		}
	}
	users[unum].ibufsize = ibufsize;

	return 0;
}

int
bind_local(char *localname, struct sockaddr *addr, socklen_t *addrlen)
{
	int sock;
	static struct sockaddr_un addrun;

	if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
		return -1;

	file_append("reclog/chatd.trace", localname);
	file_append("reclog/chatd.trace", "\n");

	strlcpy(addrun.sun_path, localname, sizeof(addrun.sun_path));
	addrun.sun_family = AF_UNIX;
	*addrlen = sizeof(addrun) - sizeof(addrun.sun_path) + strlen(addrun.sun_path);
	addr = (struct sockaddr *)&addrun;

	if (bind(sock, addr, *addrlen) == -1)
		return -1;

	return sock;
}

#ifdef DUALSTACK

int
bind_port(char *portname, struct sockaddr *addr, socklen_t *addrlen)
{
	int sock, err;
	const int on = 1;
	struct addrinfo hints;
	struct addrinfo *res, *ressave;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;
	if ((err = getaddrinfo(NULL, portname, &hints, &res)) != 0) {
		freeaddrinfo(res);
		return -1;
	}
	ressave = res;

	do {
		if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0)
			continue;

		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
		if (bind(sock, res->ai_addr, res->ai_addrlen) == 0)
			break;
		close(sock);
	} while ((res = res->ai_next) != NULL);

	if (res == NULL) {
		freeaddrinfo(ressave);
		return -1;
	}

	*addrlen = res->ai_addrlen;
	freeaddrinfo(ressave);
	return sock;
}

void
getremotename(struct sockaddr *addr, int addrlen, char *rhost, int len)
{
	getnameinfo(addr, addrlen, rhost, len, NULL, 0, 0);
	if (strncmp(rhost, "::ffff:", 7) == 0)
		memmove(rhost, rhost + 7, len - 7);
}

#else

int
bind_port(char *portname, struct sockaddr *addr, socklen_t *addrlen)
{
	int port, sock;
	const int on = 1;
	static struct sockaddr_in addrin;

	/* we only bind to port larger than 1024 to avoid unnecessary security issues */
	if ((port = atoi(portname)) < 1024)
		return -1;

	if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		return -1;

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	memset(&addrin, 0, sizeof(addrin));
	addrin.sin_family = AF_INET;
	addrin.sin_port = htons(port);
	*addrlen = sizeof(addrin);
	addr = (struct sockaddr *)&addrin;

	if (bind(sock, addr, sizeof(addrin)) == -1)
		return -1;

	return sock;
}

void
getremotename(struct sockaddr *addr, int addrlen, char *rhost, int len)
{
	struct sockaddr_in *from = (struct sockaddr_in *)addr;

	strlcpy(rhost, inet_ntoa(from->sin_addr), len);
}

#endif

void 
show_usage(const char * program)
{
	printf( "Usage:\n" );
	printf( "%s <chatroom number> -port <port num>\n", program );
	printf( "%s <chatroom number> -localfile <local sockfile name>\n", program );
	printf( "Remember to check include/chat.h first.\n" );
	printf( "\n" );
}

int
main(int argc, char **argv)
{
	int i, sr, clisock;
	fd_set readfds;
	struct timeval tv;
	struct sockaddr *addr = NULL, *cliaddr = NULL;
	socklen_t addrlen;

	if (argc < 4) {
		show_usage(argv[0]);
		return -1;
	}

	if (chdir(BBSHOME) == -1)
		return -1;

	close(0);
	close(1);
	close(2);

	chatroom = atoi(argv[1]) - 1;
	if (chatroom < 0 || chatroom >= PREDEFINED_ROOMS)
		return -1;

	if (!strcmp(argv[2], "-port")) {
		local = NA;
		if ((sock = bind_port(argv[3], addr, &addrlen)) == -1)
			return -1;
	} else if (!strcmp(argv[2], "-localfile")) {
		local = YEA;
		if ((sock = bind_local(argv[3], addr, &addrlen)) == -1)
			return -1;
	} else {
		return -1;
	}

	if ((cliaddr = malloc(addrlen)) == NULL)
		return -1;

	if (listen(sock, 5) == -1)
		return -1;

	/* init chatroom */
	maintopic = chatrooms[chatroom].topic;
	strlcpy(chatname, chatrooms[chatroom].name, sizeof(chatname));
	strlcpy(rooms[0].name, mainroom, sizeof(rooms[0].name));
	strlcpy(rooms[0].topic, maintopic, sizeof(rooms[0].topic));

	for (i = 0; i < MAXACTIVE; i++) {
		users[i].chatid[0] = '\0';
		users[i].sockfd = users[i].utent = -1;
	}

	if (fork()) return 0;
	setpgid(0, 0);

	/* ------------------------------ */
	/* trap signals                   */
	/* ------------------------------ */

	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGURG, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	FD_ZERO(&allfds);
	FD_SET(sock, &allfds);
	nfds = sock + 1;

	while (1) {
		memcpy(&readfds, &allfds, sizeof (readfds));

		tv.tv_sec = 60 * 30;
		tv.tv_usec = 0;
		if ((sr = select(nfds, &readfds, NULL, NULL, &tv)) < 0) {
			if (errno == EINTR)
				sleep(50);
			continue;
		} else if (!sr)
			continue;

#if 0
		if (sr == 0) {
			exit(0);	/* normal chat server shutdown */
		}
#endif

		if (FD_ISSET(sock, &readfds)) {
			if ((clisock = accept(sock, cliaddr, &addrlen)) == -1)
				continue;

			for (i = 0; i < MAXACTIVE; i++) {
				if (users[i].sockfd == -1) {
					if (local == NA) {
						getremotename(cliaddr, addrlen, users[i].host, sizeof(users[i].host));
						if (!strcmp(users[i].host, "127.0.0.1") || !strcmp(users[i].host, "::"))
							strlcpy(users[i].host, "localhost", sizeof(users[i].host));
					} else {
						strlcpy(users[i].host, "localhost", sizeof(users[i].host));
					}
					users[i].sockfd = clisock;
					users[i].room = -1;
					break;
				}
			}

			if (i >= MAXACTIVE) {
				/* full -- no more chat users */
				close(clisock);
			} else {

#if !RELIABLE_SELECT_FOR_WRITE
				int flags = fcntl(clisock, F_GETFL, 0);

				flags |= O_NDELAY;
				fcntl(clisock, F_SETFL, flags);
#endif

				FD_SET(clisock, &allfds);
				if (clisock >= nfds)
					nfds = clisock + 1;
				num_conns++;
			}
		}

		for (i = 0; i < MAXACTIVE; i++) {
			/* we are done with clisock, so re-use the variable */
			clisock = users[i].sockfd;
			if (clisock != -1 && FD_ISSET(clisock, &readfds)) {
				if (process_chat_command(i) == -1) {
					logout_user(i);
				}
			}
		}
#if 0
		if (num_conns <= 0) {
			/* one more pass at select, then we go bye-bye */
			tv = zerotv;
		}
#endif
	}
	/* NOTREACHED */
}
