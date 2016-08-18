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

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#if defined(CHAT) || defined(CHATD)

struct chatlist {
	char name[40];
	char topic[40];
	char sockname[120];		/* only valid when port = 0 */
	int port;
	int usermode;
};

#define PREDEFINED_ROOMS	5	

struct chatlist chatrooms[] = {
	{ "上班一族", 	"上班的感觉就是分特",	{ '\0' },		7401,	CHAT1	},
	{ "你一瓢来我一瓢", 	"水王争霸",	{ '\0' },		7402,	CHAT2	},
	{ "爱是我们的责任", 	"因为爱情",	{ '\0' },		7403,	CHAT3	},
	{ "不吐不快", 	"吐槽",	{ '\0' },		7404,	CHAT4	},
	{ "女巫咖啡店", 	"你说我听",	{ '\0' },		7405,	CHAT5	},
};

#define MAXDEFINEALIAS  60	/* MAX User Define Alias  */
#define MAXROOM         32	/* MAX number of Chat-Room */
#define MAXLASTCMD      6	/* MAX preserved chat input */
#define CHAT_IDLEN      9	/* Chat ID Length in Chat-Room */
#define CHAT_NAMELEN    20	/* MAX 20 characters of CHAT-ROOM NAME */
#define CHAT_TITLELEN   40	/* MAX 40 characters of CHAT-ROOM TITLE */

#define EXIT_LOGOUT     0
#define EXIT_LOSTCONN   -1
#define EXIT_CLIERROR   -2
#define EXIT_TIMEDOUT   -3
#define EXIT_KICK       -4

#define CHAT_LOGIN_OK       "OK"
#define CHAT_LOGIN_EXISTS   "EX"
#define CHAT_LOGIN_INVALID  "IN"
#define CHAT_LOGIN_BOGUS    "BG"

/*
 *  This defines the set of characters disallowed in chat id's. These
 *  characters get translated to underscores (_) when someone tries to use
 *  them. At the very least you should disallow spaces and '*'.
 */

#define BADCIDCHARS " *:/%"

#endif
