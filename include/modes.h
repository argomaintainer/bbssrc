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

/* Lots o' modes! */

#define IDLE            0	/* Modes a user can be in */
#define TALK            1
#define NEW             2
#define READNEW         3
#define POSTING         4
#define LAUSERS         6
#define LUSERS          7
#define SMAIL           8
#define RMAIL           9
#define MMENU           10
#define TMENU           11
#define XMENU           12
#define READING         13
#define PAGE            14
#define ADMIN           15
#define READBRD         16
#define SELECT          17
#define LOGIN           18
#define MONITOR         19
#define CHAT1           20
#define CHAT2           21
#define CHAT3           22
#define CHAT4           23
#define CHAT5           24
#define ZAP             25
#define EDITUFILE       26
#define EDITSFILE       27
#define QUERY           28
#define CNTBRDS         29
#define VOTING          30
#define VISIT           31
#define BBSNET          33
#define FRIEND          35
#define YANKING         36
#define GMENU           38
#define DICT            39
#define LOCKSCREEN      40
#define NOTEPAD         41
#define MSG             42
#define USERDEF         43
#define EDIT            44
#define OFFLINE         45
#define EDITANN         46
#define LOOKMSGS        49
#define WFRIEND         50
#define SYSINFO	        51
#define WNOTEPAD	54
#define WINMINE		62
#define FIVE            63
#define PAGE_FIVE       64
#define	WORKER		65
#define ALL_GAMES	71
#define DATAMANAGE	73
#define DIGEST          80
#define DIGESTRACE	81
#define MOBILE		99	/* gcc: 2010-10-30 */

#define WWW		0x10000		/* 当 mode 的第17位置1时，表明用户处于 WWW */

#define INBBSGAME(mode) (mode == FIVE || mode == WINMINE || mode == WORKER)
#define INMAIL(mode)	(mode == RMAIL || mode == SMAIL)
