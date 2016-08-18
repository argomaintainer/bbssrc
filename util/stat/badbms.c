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

/* Pudding: 重写badbms.s
   功能更改: 统计所有版主的未上站天数, 而非只统计7天未上站的
   	     分版面输出统计数据
   debug: 原程序对隐身登陆的情况仍视为没有登陆, 这并不符合实际
   改进统计程序风格
   有可能根据需要增加其它统计项目
 */


#include "bbs.h"

#define REPORT_FILE	"etc/bmlogin"
#define DAY_SECS	(24 * 3600)

#define MAX_RECS	4096

#undef snprintf

typedef struct {
	char userid[16];
	int nologin_days;
} loginfo_t;

typedef struct {
	char title;
	char filename[BFNAMELEN];
	char BM[BMLEN];
} brdinfo_t;

char* brd_title[] = {
	"0",
	"u",
	"z",
	"c",
	"r",
	"a",
	"s",
	"t",
	"b",
	"p",
	"*",
	"$",
	NULL};

loginfo_t loginfo[MAX_RECS];
brdinfo_t brdinfo[MAXBOARD];
int total_bms = 0;
int total_brds = 0;

char
get_brd_area(char ch)
{
	int i;
	for (i = 0; brd_title[i]; i++) {
		if (strchr(brd_title[i], ch))
			return i + '0';
	}
	return i + '0';
}

int
board_cmp(const void *a, const void *b)
{
	
	if (((brdinfo_t*)a)->title == ((brdinfo_t*)b)->title)
		return strcmp(((brdinfo_t*)a)->filename, ((brdinfo_t*)b)->filename);
	else return ((brdinfo_t*)a)->title - ((brdinfo_t*)b)->title;
}

int
load_board(void)
{
	int board_fd;
	struct boardheader board;
	brdinfo_t *curr;

	board_fd = open(BOARDS, O_RDONLY);
	if (board_fd == -1) return -1;

	while ((total_brds < MAXBOARD) &&
	       read(board_fd, &board, sizeof(board)) == sizeof(board)) {
		if (board.filename[0] == '\0' ||
		    board.flag & BRD_RESTRICT ||
		    board.flag & BRD_GROUP ||
		    board.title[0] == '0' ||	/* 不显示系统区版面 */
		    board.level != 0)
			continue;

		curr = brdinfo + total_brds;
		strcpy(curr->filename, board.filename);
		strcpy(curr->BM, board.BM);
		curr->title = get_brd_area(board.title[0]);
		total_brds++;		
	}
	return 0;
}

int
report_byboard(void)
{
	brdinfo_t *curr;
	FILE *fout;
	char tmp[1024];
	char *ptr;
	int now, i;

	fout = fopen(REPORT_FILE, "w");
	if (!fout) return -1;

	for (now = 0; now < total_brds; now++) {
		curr = brdinfo + now;
		/* Print Header */
		fprintf(fout, "版面: %-18s%s\n", curr->filename, "未上站天数");
		snprintf(tmp, sizeof(tmp), "%s", curr->BM);
		ptr = strtok(tmp, " \t\r\n");
		while (ptr && *ptr != '\0') {
			for (i = 0;
			     ((i < total_bms) && (strcasecmp(ptr, loginfo[i].userid) != 0));
			     i++);
			if (i < total_bms) {
				/* Report */
				if (loginfo[i].nologin_days < 7)
					fprintf(fout, "%-24s%6d\n", loginfo[i].userid, loginfo[i].nologin_days);
				else fprintf(fout, "%-24s\033[1;31m%6d\033[m\n", loginfo[i].userid, loginfo[i].nologin_days);
			}
			ptr = strtok(NULL, " \t\r\n");
		}
		fprintf(fout, "\n");
	}

	fclose(fout);
	return 0;
}

int
report_bmlogin(void)
{
	int passwd_fd;
	FILE *fp;
	time_t now, login_time;
	int tmp;
	struct userec user;
	char fname[1024];

	now = time(NULL);

	passwd_fd = open(PASSFILE, O_RDONLY);
	if (passwd_fd == -1) return -1;

	while ((total_bms < MAX_RECS) &&
	       read(passwd_fd, &user, sizeof(user)) == sizeof(user)) {
		if (user.userid[0] == '\0' ||
		    !(user.userlevel & PERM_BOARDS) ||
		    strcasecmp(user.userid, "SYSOP") == 0)
			continue;
		login_time = user.lastlogin;
		/* 获取隐身登陆时间 */
		snprintf(fname, sizeof(fname), "home/%c/%s/lastlogin",
			 mytoupper(user.userid[0]), user.userid);
		fp = fopen(fname, "r");
		if (fp != NULL) {
			fscanf(fp, "%d", &tmp);
			fclose(fp);
			if (tmp > login_time) login_time = tmp;				
		}
		/* 输出纪录 */
		int days = (now - login_time) / DAY_SECS;
		snprintf(loginfo[total_bms].userid, 16, "%s", user.userid);
		loginfo[total_bms].nologin_days = days;
		total_bms++;
	}
	close(passwd_fd);
	return 0;
}

int
main(int argc, char *argv[])
{

	chdir(BBSHOME);

	if (report_bmlogin() != 0) return -1;
	if (load_board() != 0) return -1;
	qsort(brdinfo, total_brds, sizeof(brdinfo_t), board_cmp);
	report_byboard();
	
	return 0;
}
