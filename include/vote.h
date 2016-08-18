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

#define VOTE_YN         (1)
#define VOTE_SINGLE     (2)
#define VOTE_MULTI      (3)
#define VOTE_VALUE      (4)
#define VOTE_ASKING     (5)

/*  LISTMASK	投票受投票名单限制 */
#define LISTMASK	PERM_POSTMASK
/*  VOTEMASK	投票受上站时间等条件限制 */
#define VOTEMASK	PERM_NOZAP
/* 如果票箱的 level 属性还含有其他权限限制，亦起作用 */

#define VOTE_BOARD 	"vote"
#define SYSTEM_VOTE 	"sysop_vote"

struct ballot {
	char uid[IDLEN + 1];	/* 投票人       */
	unsigned int voted;	/* 投票的内容  	*/
	char msg[3][STRLEN];	/* 建议事项     */
	char votehost[16];	/* 投票IP	*/
	time_t votetime;	/* 投票时间	*/
};

struct votebal {
	char userid[16];
	char title[STRLEN];
	char type;
	char report;		/* 是否公布投票IP/时间 */
	char items[32][40];
	int maxdays;
	int maxtkt;
	int totalitems;
	time_t opendate;
	unsigned int level;
	unsigned int x_logins;	/* 上站次数		*/
	unsigned int x_posts;	/* 发表文章数		*/
	unsigned int x_stay;	/* 上站的实际小时数	*/
	unsigned int x_live;	/* 注册的物理时间	*/
};
