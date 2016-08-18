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

struct ACSHM *movieshm = NULL;
int nnline = 0;

void
empty_movie(int x)
{
	report("Empty movie!!! (error = %d)", x);

	strcpy(movieshm->data[1], "\033[K      ** 尚未设定活动看版 ** ");
	strcpy(movieshm->data[2], "\033[K         请详见安装说明书    ");
	strcpy(movieshm->data[3], "\033[K         设定 activeboard 版 ");

	movieshm->movielines = MAXMOVIE;
	movieshm->movieitems = 1;
	movieshm->update = time(NULL);
}

void
activeboard_init(void)
{
	struct fileheader fh;
	FILE *fp;
	char *ptr;
	char buf[1024], buf2[1024];
	struct stat st;
	int max = 0, i = 0, j = 0, x, y = 0;
	int flag;		/* flag = 1 即为过虑掉 "--\n" 以后之任何内容 */

	if (movieshm == NULL) {
		movieshm = attach_shm("ACBOARD_SHMKEY", sizeof(*movieshm));
	}

	if (stat("boards/activeboard/.DIGEST", &st) < 0) {
		empty_movie(1);
		return;
	}
	if (movieshm->update > st.st_mtime)
		return;

	for (i = 0; i < ACBOARD_MAXLINE; i++)
		movieshm->data[i][0] = 0;

	max = get_num_records("boards/activeboard/.DIGEST", sizeof (fh));

	i = 1;
	j = 0;
	for (i = 1, j = 0; i <= max && j < ACBOARD_MAXLINE; i++) {
		get_record("boards/activeboard/.DIGEST", &fh, sizeof(fh), i);
		snprintf(buf, sizeof(buf), "boards/activeboard/%s", fh.filename);

		if ((fp = fopen(buf, "r")) == NULL)
			continue;

		if (fh.title[0] == '$') {
			flag = (int) (fh.title[1] - '0');
		} else {
			flag = 4;
		}

		for (x = 0; x < flag; x++) {		// 跳过头部信息
			if (fgets(buf, 1024, fp) == NULL)
				goto next;
		}

		y++;					// record how many files have been append
		flag = 0;
		for (x = 0; x < MAXMOVIE - 1 && j < ACBOARD_MAXLINE; x++) {
			if (fgets(buf, 1024, fp) != NULL) {
				buf[ACBOARD_BUFSIZE - 4] = '\0';
				if (flag == 1 || strcmp(buf, "--\n") == 0) {
					strcpy(buf2, "\033[K");
					flag = 1;
				}
				ptr = movieshm->data[j];
				if (flag == 0) {
					strcpy(buf2, "\033[K");
					strcat(buf2, buf);
				}
				memcpy(ptr, buf2, ACBOARD_BUFSIZE);
			} else {	/* no data handling */
				strcpy(movieshm->data[j], "\033[K");
			}
			j++;
		}
next:		
		fclose(fp);
	}

	if (j == 0) {
		empty_movie(3);
		return;
	}
	movieshm->movielines = j;
	movieshm->movieitems = y;
	movieshm->update = time(NULL);

	report("活动看版更新, 共 %d 行, %d 部份.", j, y);
	return;
}

/* below added by netty, rewrite by SmallPig */
void
netty_more(void)
{
	char buf[256];
	int ne_row = 1;
	int x, y;

	getyx(&y, &x);
	update_endline();
	if (!DEFINE(DEF_ACBOARD))
		return;
	nnline = (time(NULL) / 10 % movieshm->movieitems) * (MAXMOVIE - 1);

	while ((nnline < movieshm->movielines)) {
#ifdef BIGGER_MOVIE
		clear_line(1 + ne_row);
#else
		clear_line(2 + ne_row);
#endif
		strcpy(buf, movieshm->data[nnline]);
		showstuff(buf /*, 0 */ );
		nnline = nnline + 1;
		ne_row = ne_row + 1;
		if (nnline == movieshm->movielines) {
			nnline = 0;
			break;
		}
		if (ne_row > MAXMOVIE - 1) {
			break;
		}
	}
	outs("\033[m");
	move(y, x);
}

static void
R_monitor_sig(int signo)
{
	R_monitor();
}

void
R_monitor(void)
{
	if (uinfo.mode != MMENU)
		return;
	if (!DEFINE(DEF_ACBOARD) && !DEFINE(DEF_ENDLINE))
		return;

	alarm(0);
	signal(SIGALRM, R_monitor_sig);
	netty_more();
	printacbar();
	alarm(10);		// if (!DEFINE(DEF_ACBOARD)) alarm(55);
}

void
printacbar(void)
{
#ifndef BIGGER_MOVIE
//	struct boardheader *bp;
	int x, y;

	getyx(&y, &x);
	clear_line(2);
	outs("\033[1;31m□——————————————┤\033[37m活  动  看  版\033[31m├——————————————□ \033[m");
	clear_line(8);

//	if ((bp = getbcache(DEFAULTBOARD)) && (bp->flag & VOTE_FLAG))
//	      prints("\033[1;31m□———————————┤\033[37m系统投票中 [ Config->Vote ] \033[31m├——————————□ \033[m\n");
//	else 

	outs("\033[1;31m□—————————————————————————————————————□ \033[m");
	move(y, x);
#endif
	refresh();
}
