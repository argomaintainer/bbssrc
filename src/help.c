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

void
show_help(char *fname)
{
	/*---       Modified by period      2000-10-26      according to ylsdd's warning    ---*/
	static short int cnt;
	int tmpansi;

	if (cnt < 2) {
		tmpansi = showansi;
		showansi = YEA;
		++cnt;
		clear();
		ansimore(fname, YEA);
		clear();
		--cnt;
		showansi = tmpansi;
	}
}

//Added by cancel at 02.03.10
int
registerhelp(void)
{
	show_help("help/registerhelp");
	return DIRCHANGED;
}

/*void
standhelp( mesg )
char    *mesg;
{
    prints("\033[1;32;44m");
    prints( mesg ) ;
    prints("\033[m");
}*/

int
mainreadhelp(void)
{
	show_help("help/mainreadhelp");
	return FULLUPDATE;
}

int
mailreadhelp(void)
{
	show_help("help/mailreadhelp");
	return FULLUPDATE;
}
