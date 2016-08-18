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

/*  五子棋程式   Programmed by Birdman     */
/*  140.116.102.125 连珠哇哈哈小站         */
/*  成大电机88级                           */

#include "functions.h"
#ifdef FIVEGAME

#include "bbs.h"

#define black 1
#define white 2
#define FDATA "five"
#define b_lines 24
#define LCECHO (2)
#define cuser currentuser
#define setutmpmode(a) modify_user_mode( a )

int player, winner = 0, quitf;
int px, py, hand, tdeadf, tlivef, livethree, threefour;
int chess[250][2] = { {0, 0} };
int playboard[15][15] = { {0, 0} };
char abcd[15] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O' };
extern int RMSG;

void five_chat(char *, int);
void press(void);

void
Box(int x, int y, int x1, int y1)
{
	char *lt = "┌", *rt = "┐", *hor = "─", *ver = "│", *lb = "└", *rb =
	    "┘";
	int i;

	move(x, y);
	outs(lt);
	for (i = y + 2; i <= y1 - 2; i += 2)
		outs(hor);
	outs(rt);
	for (i = x + 1; i <= x1 - 1; i++) {
		move(i, y);
		outs(ver);
		move(i, y1);
		outs(ver);
	}
	move(x1, y);
	outs(lb);
	for (i = y + 2; i <= y1 - 2; i += 2)
		outs(hor);
	outs(rb);
}

void
InitScreen(void)
{
	int i;

	for (i = 0; i < 16; i++) {
		move(i, 0);
		clrtoeol();
	}
	move(0, 0);
	outs("┌┬┬┬┬┬┬┬┬┬┬┬┬┬┐15\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤14\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤13\n"
	     "├┼┼＋┼┼┼┼┼┼┼＋┼┼┤12\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤11\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤10\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤9\n"
	     "├┼┼┼┼┼┼＋┼┼┼┼┼┼┤8\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤7\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤6\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤5\n"
	     "├┼┼＋┼┼┼┼┼┼┼＋┼┼┤4\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤3\n"
	     "├┼┼┼┼┼┼┼┼┼┼┼┼┼┤2\n"
	     "└┴┴┴┴┴┴┴┴┴┴┴┴┴┘1\n"
	     "A B C D E F G H I J K L M N O\n");

	/* user guide */
	move(4, 64);
	outs("切换:   Tab键");
	move(5, 64);
	outs("移动:  方向键");
	move(6, 64);
	outs("      H,J,K,L");
	move(7, 64);
	outs("下子:  空格键");
	move(8, 64);
	outs("重开:  N 或者");
	move(9, 64);
	outs("       Ctrl+N");
	move(10, 64);
	outs("退出:  Q 或者");
	move(11, 64);
	outs("       Ctrl+C");
	move(12, 64);
	outs("  黑先有禁手");
	Box(3, 62, 13, 78);
	move(3, 64);
	outs("用法");

	move(0, 33);
	outs("\033[35;43m◆五子棋对战◆\033[30;42m  程式:成大电机88级 Birdman  \033[m");
}

void
haha(int what)
{
	char *logo[3] = { " 活三喽! ", "哈哈活四!", " 小心冲四! " };

	move(15, 64);
	if (what >= 3)
		outs("            ");
	else
		outs(logo[what]);
}

void
win(int who)
{
	move(12, 35);
	outs("\033[47m\033[31m┌————┐\033[m");
	move(13, 35);
	if (who == black)
		outs("\033[47m\033[31m│  \033[30;42m黑胜\033[m\033[47m \033[31m │\033[m");
	else
		outs("\033[47m\033[31m│  \033[30;42m白胜\033[m\033[47m \033[31m │\033[m");
	move(14, 35);
	outs("\033[47m\033[31m└————┘\033[m");
	refresh();
	winner = who;
	press();
}

void
quit(void)
{
	move(12, 35);
	outs("\033[47m\033[31m┌———————┐\033[m");
	move(13, 35);
	outs("\033[47m\033[31m│  \033[30;42m对方退出了\033[m\033[47m \033[31m │\033[m");
	move(14, 35);
	outs("\033[47m\033[31m└———————┘\033[m");
	refresh();
	bell();
	press();
}

void
bandhand(int style)
{
	if (style == 3) {
		move(12, 35);
		outs("\033[47m\033[31m┌黑败——————┐\033[m");
		move(13, 35);
		outs("\033[47m\033[31m│  \033[37;41m黑禁手双活三\033[m\033[47m  \033[31m│\033[m");
		move(14, 35);
		outs("\033[47m\033[31m└————————┘\033[m");
	} else if (style == 4) {
		move(12, 35);
		outs("\033[47m\033[31m┌黑败——————┐\033[m");
		move(13, 35);
		outs("\033[47m\033[31m│  \033[37;41m黑禁手双  四\033[m\033[47m  \033[31m│\033[m");
		move(14, 35);
		outs("\033[47m\033[31m└————————┘\033[m");
	} else {
		move(12, 35);
		outs("\033[47m\033[31m┌黑败——————┐\033[m");
		move(13, 35);
		outs("\033[47m\033[31m│  \033[37;41m黑禁手连六子\033[m\033[47m  \033[31m│\033[m");
		move(14, 35);
		outs("\033[47m\033[31m└————————┘\033[m");
	}

	winner = white;
	press();
	return;
}

void
callfour(int x1, int y1, int x2, int y2, int x3, int y3,
	 int x4, int y4, int x5, int y5, int x6, int y6)
{
	int n_black, n_white, dead /* ,i,j,k */ ;

	n_black = n_white = dead = 0;

	if (x1 < 0 || x2 < 0 || x3 < 0 || x4 < 0 || x5 < 0 || x6 < 0 ||
	    x1 > 14 || x2 > 14 || x3 > 14 || x4 > 14 || x5 > 14 || x6 > 14)
		return;

	if (winner != 0)
		return;

	if ((playboard[x1][y1] != 0 && playboard[x6][y6] == 0) ||
	    (playboard[x1][y1] == 0 && playboard[x6][y6] != 0))
		dead = 1;	/* for checking  冲四 */

	if (playboard[x2][y2] == black)
		n_black += 1;
	if (playboard[x2][y2] == white)
		n_white += 1;
	if (playboard[x3][y3] == black)
		n_black += 1;
	if (playboard[x3][y3] == white)
		n_white += 1;
	if (playboard[x4][y4] == black)
		n_black += 1;
	if (playboard[x4][y4] == white)
		n_white += 1;
	if (playboard[x5][y5] == black)
		n_black += 1;
	if (playboard[x5][y5] == white)
		n_white += 1;

	if (playboard[x1][y1] == 0 && playboard[x6][y6] == 0 &&
	    (playboard[x3][y3] == 0 || playboard[x4][y4] == 0)) {
		if (n_black == 3 || n_white == 3)
			haha(0);
		if (n_black == 3)
			livethree += 1;
	}

	if (n_black == 4) {
		if (playboard[x1][y1] == black && playboard[x6][y6] == black)
			bandhand(6);
		if (playboard[x1][y1] != 0 && playboard[x6][y6] != 0)
			return;

		if (dead) {
/* add by satan Mar 19, 1999 start*/
			if ((playboard[x1][y1] == 0 && playboard[x5][y5] == 0) ||
			    (playboard[x2][y2] == 0 && playboard[x6][y6] == 0))
				livethree -= 1;
/* add by satan Mar 19, 1999 end*/

			haha(2);
			tdeadf += 1;
			tlivef += 1;	/*黑死四啦 */
			threefour = 0;
			return;
		}

		threefour = black;
		tlivef += 1;	/*活四也算双四 */
	}
	if (n_white == 4) {
		if (playboard[x1][y1] != 0 && playboard[x6][y6] != 0)
			return;
		if (dead) {
			haha(2);
			tdeadf += 1;
			threefour = 0;
			return;
		}

		threefour = white;
		tlivef += 1;

	}
	if (playboard[x1][y1] == black)
		n_black += 1;	/*check 连子 */
	if (playboard[x6][y6] == black)
		n_black += 1;

	if (n_black == 5 && (playboard[x3][y3] == 0 || playboard[x4][y4] == 0 ||
			     playboard[x5][y5] == 0 || playboard[x2][y2] == 0))
		tlivef -= 1;	/* 六缺一型, 不算冲四 */

	if (n_black >= 6)
		bandhand(6);
	return;
}

void
calvalue(int x1, int y1,
	 int x2, int y2, int x3, int y3, int x4, int y4, int x5, int y5)
{
	int n_black, n_white, empty, i, j /* ,k */ ;

	n_black = n_white = empty = 0;

	if (x1 < 0 || x2 < 0 || x3 < 0 || x4 < 0 || x5 < 0 ||
	    x1 > 14 || x2 > 14 || x3 > 14 || x4 > 14 || x5 > 14)
		return;
	if (winner != 0)
		return;
	if (playboard[x2][y2] == 0 || playboard[x3][y3] == 0
	    || playboard[x4][y4] == 0)
		empty = 1;	/*check 10111型死四 */

	if (playboard[x1][y1] == black)
		n_black += 1;
	if (playboard[x1][y1] == white)
		n_white += 1;
	if (playboard[x2][y2] == black)
		n_black += 1;
	if (playboard[x2][y2] == white)
		n_white += 1;
	if (playboard[x3][y3] == black)
		n_black += 1;
	if (playboard[x3][y3] == white)
		n_white += 1;
	if (playboard[x4][y4] == black)
		n_black += 1;
	if (playboard[x4][y4] == white)
		n_white += 1;
	if (playboard[x5][y5] == black)
		n_black += 1;
	if (playboard[x5][y5] == white)
		n_white += 1;

	if (playboard[x1][y1] == 0 && playboard[x5][y5] == 0) {
		if (n_white == 3 || n_black == 3)
			haha(0);

		if (n_black == 3)
			livethree += 1;
	}

	if ((n_white == 4 || n_black == 4) && (empty == 1)) {
		tdeadf += 1;
		tlivef += 1;
		haha(2);
		return;
	}

	if (n_black == 5) {	/*再扫连六 */
		tlivef = -1;
		tdeadf = 0;
		livethree = 0;
		for (i = 0; i <= 14; i++)	/*四纵向 */
			for (j = 0; j <= 9; j++)
				callfour(i, j, i, j + 1, i, j + 2, i, j + 3, i,
					 j + 4, i, j + 5);
		for (i = 0; i <= 9; i++)	/*四横向 */
			for (j = 0; j <= 14; j++)
				callfour(i, j, i + 1, j, i + 2, j, i + 3, j,
					 i + 4, j, i + 5, j);
		for (i = 0; i <= 9; i++)	/*四斜右下 */
			for (j = 0; j <= 9; j++) {
				callfour(i, j, i + 1, j + 1, i + 2, j + 2,
					 i + 3, j + 3, i + 4, j + 4, i + 5,
					 j + 5);
				/*四斜左下 */
				callfour(i, j + 5, i + 1, j + 4, i + 2, j + 3,
					 i + 3, j + 2, i + 4, j + 1, i + 5, j);
			}
		if (winner == 0)
			win(black);
	}
	if (n_white == 5)
		win(white);
	return;
}

char save_page_requestor[40];

void
five_pk(int fd, int first)
{
	int cx, ch, cy, datac, fdone, x = 0;
	char genbuf[100], data[90], xy_po[5], genbuf1[20] =
	    { '\0' } /* ,x1[1],y1[1],done[1] */ ;
/*    struct user_info *opponent; */
/*     char fname[50]; */
	int i, j /* ,k */ , fway, banf, idone;

/*
 *      增加聊天功能. Added by satan. 99.04.02
 */

#undef END
#undef MAX

#define START    17
#define END      21
#define PROMPT   23
#define MAX      (END - START)
#define BSIZE    60

	char chatbuf[80], *cbuf;
	int ptr = 0, chating = 0 /*, over = 0 */ ;

	setutmpmode(FIVE);	/*用户状态设置 */
	clear();
	InitScreen();
	five_chat(NULL, 1);

	cbuf = chatbuf + 19;
	chatbuf[0] = '\0';
	chatbuf[79] = '\0';
	cbuf[0] = '\0';
	snprintf(chatbuf + 1, sizeof(chatbuf) - 2, "%-16s: ", cuser.username);

	add_io(fd, 0);

      begin:
	for (i = 0; i <= 14; i++)
		for (j = 0; j <= 14; j++)
			playboard[i][j] = 0;

	hand = 1;
	winner = 0;
	quitf = 0;
	px = 14;
	py = 7;
	fway = 1;
	banf = 1;
	idone = 0;

	snprintf(genbuf, sizeof(genbuf), "%s (%s)", cuser.userid, cuser.username);

	if (first) {
		move(1, 33);
		prints("黑●先手 %s  ", genbuf);
		move(2, 33);
		prints("白○后手 %s  ", save_page_requestor);
	} else {
		move(1, 33);
		prints("白○后手 %s  ", genbuf);
		move(2, 33);
		prints("黑●先手 %s  ", save_page_requestor);
	}

	move(15, 35);
	if (first)
		outs("★等待对方下子★");
	else
		outs("◆现在该自己下◆");
	move(7, 14);
	outs("●");
	player = white;
	playboard[7][7] = black;
	chess[1][0] = 14;	/*纪录所下位置 */
	chess[1][1] = 7;
	move(4, 35);
	outs("第 1手 ●H 8");

	if (!first) {		/*超怪! */
		move(7, 14);
		fdone = 1;
	} else
		fdone = 0;	/*对手完成 */

	while (1) {
		ch = igetkey();

		if (ch == I_OTHERDATA) {
			datac = recv(fd, data, sizeof (data), 0);
			if (datac <= 0) {
				move(17, 30);
				outs("\033[47m\033[31;47m 对方投降了...@_@ \033[m");
				break;
			}
			if (data[0] == '\0') {
				five_chat(data + 1, 0);
				if (chating)
					move(PROMPT, ptr + 6);
				else
					move(py, px);
				continue;
			} else if (data[0] == '\1') {
				bell();
				RMSG = YEA;
				saveline(PROMPT, 0);
				snprintf(genbuf, sizeof(genbuf),
					"%s 说: 重来一盘好吗? (Y/N)[Y]:",
					save_page_requestor);
				getdata(PROMPT, 0, genbuf, genbuf1, 2, LCECHO,
					YEA);
				RMSG = NA;
				if (genbuf1[0] == 'n' || genbuf1[0] == 'N') {
					saveline(PROMPT, 1);
					send(fd, "\3", 1, 0);
					continue;
				} else {
					saveline(PROMPT, 1);
					InitScreen();
					first = 0;
					send(fd, "\2", 1, 0);
					goto begin;
				}
			} else if (data[0] == '\2') {
				bell();
				saveline(PROMPT, 0);
				move(PROMPT, 0);
				clrtoeol();
				prints("%s 接受了你的请求 :-)",
				       save_page_requestor);
				refresh();
				sleep(1);
				saveline(PROMPT, 1);
				InitScreen();
				first = 1;
				goto begin;
			} else if (data[0] == '\3') {
				bell();
				saveline(PROMPT, 0);
				move(PROMPT, 0);
				clrtoeol();
				prints("%s 拒绝了你的请求 :-(",
				       save_page_requestor);
				refresh();
				sleep(1);
				saveline(PROMPT, 1);
				if (chating)
					move(PROMPT, ptr + 6);
				else
					move(py, px);
				continue;
			} else if (data[0] == '\xff') {
				move(PROMPT, 0);
				quit();
				break;
			}
			i = atoi(data);
			cx = i / 1000;	/*解译data成棋盘资料 */
			cy = (i % 1000) / 10;
			fdone = i % 10;
			hand += 1;

			if (hand % 2 == 0)
				move(((hand - 1) % 20) / 2 + 4, 48);
			else
				move(((hand - 1) % 19) / 2 + 4, 35);

			prints("第%2d手 %s%c%2d", hand,
			       (player == black) ? "●" : "○", abcd[cx / 2],
			       15 - cy);

			move(cy, cx);
			x = cx / 2;
			playboard[x][cy] = player;
			if (player == black) {
				outs("●");
				player = white;
			} else {
				outs("○");
				player = black;
			}
			move(cy, cx);
			refresh();
			bell();
			move(15, 35);
			outs("◆现在该自己下◆");
			haha(5);

			tdeadf = tlivef = livethree = threefour = 0;
			for (j = 0; j <= 10; j++)
				calvalue(cx / 2, j, cx / 2, j + 1, cx / 2,
					 j + 2, cx / 2, j + 3, cx / 2, j + 4);
			for (i = 0; i <= 10; i++)	/*横向 */
				calvalue(i, cy, i + 1, cy, i + 2, cy, i + 3, cy,
					 i + 4, cy);
			for (i = -4; i <= 0; i++)	/*斜右下 */
				calvalue(cx / 2 + i, cy + i, cx / 2 + i + 1,
					 cy + i + 1, cx / 2 + i + 2, cy + i + 2,
					 cx / 2 + i + 3, cy + i + 3,
					 cx / 2 + i + 4, cy + i + 4);
			for (i = -4; i <= 0; i++)	/*斜左下 */
				calvalue(cx / 2 - i, cy + i, cx / 2 - i - 1,
					 cy + i + 1, cx / 2 - i - 2, cy + i + 2,
					 cx / 2 - i - 3, cy + i + 3,
					 cx / 2 - i - 4, cy + i + 4);

			for (j = 0; j <= 9; j++)
				callfour(cx / 2, j, cx / 2, j + 1, cx / 2,
					 j + 2, cx / 2, j + 3, cx / 2, j + 4,
					 cx / 2, j + 5);
			for (i = 0; i <= 9; i++)	/*四横向 */
				callfour(i, cy, i + 1, cy, i + 2, cy, i + 3, cy,
					 i + 4, cy, i + 5, cy);
			for (i = -5; i <= 0; i++) {	/*四斜右下 */
				callfour(cx / 2 + i, cy + i, cx / 2 + i + 1,
					 cy + i + 1, cx / 2 + i + 2, cy + i + 2,
					 cx / 2 + i + 3, cy + i + 3,
					 cx / 2 + i + 4, cy + i + 4,
					 cx / 2 + i + 5, cy + i + 5);
				/*四斜左下 */
				callfour(cx / 2 - i, cy + i, cx / 2 - i - 1,
					 cy + i + 1, cx / 2 - i - 2, cy + i + 2,
					 cx / 2 - i - 3, cy + i + 3,
					 cx / 2 - i - 4, cy + i + 4,
					 cx / 2 - i - 5, cy + i + 5);
			}

			py = cy;
			px = cx;
			if (tlivef >= 2 && winner == 0)
				bandhand(4);
			if (livethree >= 2 && tlivef == 0)
				bandhand(3);
			if (threefour == black)
				haha(1);
			else if (threefour == white)
				haha(1);
			if (chating) {
				sleep(1);
				move(PROMPT, ptr + 6);
			} else
				move(py, px);
			if (winner) {
				InitScreen();
				goto begin;
			}
		} else {
			if (ch == Ctrl('X')) {
				quitf = 1;
			} else if (ch == Ctrl('C') || ch == Ctrl('D') ||
				   ((ch == 'Q' || ch == 'q') && !chating)) {
				RMSG = YEA;
				saveline(PROMPT, 0);
				refresh();
				getdata(PROMPT, 0,
					 "您确定要离开吗? (Y/N)?[N] ", genbuf1,
					 2, LCECHO, YEA);
				if (genbuf1[0] == 'Y' || genbuf1[0] == 'y')
					quitf = 1;
				else
					quitf = 0;
				saveline(PROMPT, 1);
				RMSG = NA;
			} else if (ch == Ctrl('N') ||
				   ((ch == 'N' || ch == 'n') && !chating)) {
				saveline(PROMPT, 0);
				RMSG = YEA;
				getdata(PROMPT, 0,
					"您确定要重新开始吗? (Y/N)?[N] ",
					genbuf1, 2, LCECHO, YEA);
				if (genbuf1[0] == 'Y' || genbuf1[0] == 'y') {
					send(fd, "\1", 1, 0);
					move(PROMPT, 0);
					bell();
					clrtoeol();
					move(PROMPT, 0);
					outs("已经已经替您发出请求了");
					refresh();
					sleep(1);
				}
				RMSG = NA;
				saveline(PROMPT, 1);
				if (chating)
					move(PROMPT, ptr + 6);
				else
					move(py, px);
				continue;
			} else if (ch == '\t') {
				if (chating) {
					chating = 0;
					move(py, px);
				} else {
					chating = 1;
					move(PROMPT, 6 + ptr);
				}
				continue;
			} else if (ch == '\0')
				continue;
			else if (chating) {
				if (ch == '\n' || ch == '\r') {
					if (!cbuf[0])
						continue;
					ptr = 0;
					five_chat(chatbuf + 1, 0);
					send(fd, chatbuf,
					     strlen(chatbuf + 1) + 2, 0);
					cbuf[0] = '\0';
					move(PROMPT, 6);
					clrtoeol();
				} else if (ch == KEY_LEFT) {
					if (ptr)
						ptr--;
				} else if (ch == KEY_RIGHT) {
					if (cbuf[ptr])
						ptr++;
				} else if (ch == Ctrl('H') || ch == '\177') {
					if (ptr) {
						ptr--;
						memcpy(&cbuf[ptr],
						       &cbuf[ptr + 1],
						       BSIZE - ptr);
						move(PROMPT, ptr + 6);
						clrtoeol();
						outs(&cbuf[ptr]);
					}
				} else if (ch == KEY_DEL) {
					if (cbuf[ptr]) {
						memcpy(&cbuf[ptr],
						       &cbuf[ptr + 1],
						       BSIZE - ptr);
						clrtoeol();
						outs(&cbuf[ptr]);
					}
				} else if (ch == Ctrl('A')) {
					ptr = 0;
				} else if (ch == Ctrl('E')) {
					while (cbuf[++ptr]) ;
				} else if (ch == Ctrl('K')) {
					ptr = 0;
					cbuf[ptr] = '\0';
					move(PROMPT, ptr + 6);
					clrtoeol();
				} else if (ch == Ctrl('U')) {
					memmove(cbuf, &cbuf[ptr],
						BSIZE - ptr + 1);
					ptr = 0;
					move(PROMPT, ptr + 6);
					clrtoeol();
					outs(cbuf);
				} else if (ch == Ctrl('W')) {
					if (ptr) {
						int optr;

						optr = ptr;
						ptr--;
						do {
							if (cbuf[ptr] != ' ')
								break;
						}
						while (--ptr);
						do {
							if (cbuf[ptr] == ' ') {
								if (cbuf
								    [ptr + 1] !=
								    ' ')
									ptr++;
								break;
							}
						}
						while (--ptr);
						memcpy(&cbuf[ptr], &cbuf[optr],
						       BSIZE - optr + 1);
						move(PROMPT, ptr + 6);
						clrtoeol();
						outs(&cbuf[ptr]);
					}
				} else if (isprint2(ch)) {
					if (ptr == BSIZE)
						continue;
					if (!cbuf[ptr]) {
						cbuf[ptr] = ch;
						move(PROMPT, 6 + ptr);
						outc(ch);
						cbuf[++ptr] = 0;
					} else {
						memmove(&cbuf[ptr + 1],
							&cbuf[ptr],
							BSIZE - ptr + 1);
						cbuf[ptr] = ch;
						move(PROMPT, 6 + ptr);
						outs(&cbuf[ptr]);
						ptr++;
					}
				}
				move(PROMPT, 6 + ptr);
				continue;
			}
		}

		if (fdone == 1 && !chating && ch != I_OTHERDATA) {	/*换我 */

			move(py, px);
			switch (ch) {

			case KEY_DOWN:
			case 'j':
			case 'J':
				py = py + 1;
				if (py > 14)
					py = 0;
				break;

			case KEY_UP:
			case 'k':
			case 'K':
				py = py - 1;
				if (py < 0)
					py = 14;
				break;

			case KEY_LEFT:
			case 'h':
			case 'H':
				px = px - 1;
				if (px < 0)
					px = 28;
				break;

			case KEY_RIGHT:
			case 'l':
			case 'L':
				px = px + 1;
				if (px > 28) {
					px = 0;	//px=px-1;
// 移动光标到最右边，会按两下右没有反应，再按一下回到左边。
// 如果按两下右后落子，five程序乱。
// period (瞌睡虫): ~问题出在这儿, 应该注释掉.
				}	/*会跳格咧 */
				break;
			case ' ':
				if (banf == 1)
					break;

				if ((px % 2) == 1)
					px = px - 1;	/*解决netterm不合问题 */
				move(py, px);
				hand += 1;
				playboard[x][py] = player;
				if (player == black) {
					outs("●");
					player = white;
				} else {
					outs("○");
					player = black;
				}
				chess[hand][0] = px;
				chess[hand][1] = py;
				if (hand % 2 == 0)
					move(((hand - 1) % 20) / 2 + 4, 48);
				else
					move(((hand - 1) % 19) / 2 + 4, 35);

				prints("第%2d手 %s%c%2d", hand,
				       (hand % 2 == 1) ? "●" : "○",
				       abcd[px / 2], 15 - py);
				idone = 1;
				move(py, px);
				refresh();
				break;
			default:
				break;
			}
			move(py, px);
			x = px / 2;
			if (playboard[x][py] != 0)
				banf = 1;
			else
				banf = 0;

			if (idone == 1) {
				xy_po[0] = px / 10 + '0';
				xy_po[1] = px % 10 + '0';
				xy_po[2] = py / 10 + '0';
				xy_po[3] = py % 10 + '0';
				fdone = 0;
				xy_po[4] = '1';
				if (send(fd, xy_po, sizeof (xy_po), 0) == -1)
					break;

				move(15, 35);
				outs("★等待对方下子★");
				haha(5);

				tdeadf = tlivef = livethree = threefour = 0;
				for (j = 0; j <= 10; j++)
					calvalue(px / 2, j, px / 2, j + 1,
						 px / 2, j + 2, px / 2, j + 3,
						 px / 2, j + 4);
				for (i = 0; i <= 10; i++)	/*横向 */
					calvalue(i, py, i + 1, py, i + 2, py,
						 i + 3, py, i + 4, py);
				for (i = -4; i <= 0; i++)	/*斜右下 */
					calvalue(px / 2 + i, py + i,
						 px / 2 + i + 1, py + i + 1,
						 px / 2 + i + 2, py + i + 2,
						 px / 2 + i + 3, py + i + 3,
						 px / 2 + i + 4, py + i + 4);
				for (i = -4; i <= 0; i++)	/*斜左下 */
					calvalue(px / 2 - i, py + i,
						 px / 2 - i - 1, py + i + 1,
						 px / 2 - i - 2, py + i + 2,
						 px / 2 - i - 3, py + i + 3,
						 px / 2 - i - 4, py + i + 4);

				for (j = 0; j <= 9; j++)
					callfour(px / 2, j, px / 2, j + 1,
						 px / 2, j + 2, px / 2, j + 3,
						 px / 2, j + 4, px / 2, j + 5);
				for (i = 0; i <= 9; i++)	/*四横向 */
					callfour(i, py, i + 1, py, i + 2, py,
						 i + 3, py, i + 4, py, i + 5,
						 py);
				for (i = -5; i <= 0; i++) {	/*四斜右下 */
					callfour(px / 2 + i, py + i,
						 px / 2 + i + 1, py + i + 1,
						 px / 2 + i + 2, py + i + 2,
						 px / 2 + i + 3, py + i + 3,
						 px / 2 + i + 4, py + i + 4,
						 px / 2 + i + 5, py + i + 5);
					/*四斜左下 */
					callfour(px / 2 - i, py + i,
						 px / 2 - i - 1, py + i + 1,
						 px / 2 - i - 2, py + i + 2,
						 px / 2 - i - 3, py + i + 3,
						 px / 2 - i - 4, py + i + 4,
						 px / 2 - i - 5, py + i + 5);
				}

				if (tlivef >= 2 && winner == 0)
					bandhand(4);
				if (livethree >= 2 && tlivef == 0)
					bandhand(3);
				if (threefour == black)
					haha(1);
				else if (threefour == white)
					haha(1);

			}
			idone = 0;
		}
		if (quitf) {
			genbuf1[0] = '\xff';
			send(fd, genbuf1, 1, 0);
			press();
			break;
		}
		if (winner) {
			InitScreen();
			goto begin;
		}
	}

	add_io(0, 0);
	close(fd);
	return;
}

void
five_chat(char *msg, int init)
{
	char prompt[] = "===>";
	char chat[] = "聊天: ";
	static char win[MAX][80];
	static int curr, p, i;

	if (init) {
		for (i = 0; i < MAX; i++)
			win[i][0] = '\0';
		curr = START;
		p = 0;
		move(START - 1, 0);
		for (i = 0; i < 80; i++)
			outc('-');
		move(END + 1, 0);
		for (i = 0; i < 80; i++)
			outc('-');
		move(curr, 0);
		clrtoeol();
		outs(prompt);
		move(PROMPT, 0);
		outs(chat);
		return;
	}

	if (msg) {
		strlcpy(win[p], msg, 80);
		move(curr, 0);
		clrtoeol();
		outs(win[p]);
		p++;
		if (p == MAX)
			p = 0;
		curr++;
		if (curr > END) {
			for (i = START; i < END; i++) {
				move(i, 0);
				clrtoeol();
				outs(win[(p + MAX + (i - START)) % MAX]);
			}
			curr = END;
		}
		move(curr, 0);
		clrtoeol();
		outs(prompt);
		refresh();
	}
}

void
press(void)
{
	int c;
	int tmpansi;

	tmpansi = showansi;
	showansi = 1;
	saveline(t_lines - 1, 0);
	move(t_lines - 1, 0);
	clrtoeol();
	prints
	    ("\033[37;40m\033[0m                               \033[33m按任意键继续 ...\033[37;40m\033[0m");
	refresh();
	read(0, &c, sizeof (int));
	move(t_lines - 1, 0);
	saveline(t_lines - 1, 1);
	showansi = tmpansi;
}

#endif
