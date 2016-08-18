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

char brc_buf[BRC_MAXSIZE];
int brc_cur, brc_size, brc_changed = 0;
char brc_name[BRC_STRLEN];
int brc_list[BRC_MAXNUM], brc_num;	/* brc_list[0] ~ brc_list[brc_num-1] 从大到小 */
int boardlevel = 0;

struct newpostdata {
	char *name, *title, *BM;
	unsigned flag;
	int pos, total;	  /* pos为本版在bcache数组的位置, total版面文章数 */
	char unread, zap; /* unread=true此版有未读, zap=true表示zap掉此讨论区 */
	char status;	  /* 版面列表显示的 'r', 'p' 或 'P'等字符 */
#ifdef INBOARDCOUNT
	unsigned short inboard; /* 版面在线人数*/
#endif
};

struct newpostdata nbrd[MAXBOARD];

#define USER_GOOD_BRD 1
#define SYS_GOOD_BRD 2

struct goodboard {
	char ID[GOOD_BRC_NUM][BRC_STRLEN + 1];
	int num;
	int type;/* Added by betterman at 05/11/23 */
} GoodBrd;

int *zapbuf;
int brdnum = 0, yank_flag = 0;	/* yank_flag=true: zap掉的讨论区仍然显示 */
char *boardprefix;	/* board title prefix */

char *restrict_boards = NULL;	/* 限制版名单列表: 用户出现在这些限制版名单里 */

/* return the index of the board 'bname' in BoodBrd 
 * or 0 if the 'bname' is not found */
int
inGoodBrds(char *bname)
{
	int i;

	for (i = 0; i < GoodBrd.num && i < GOOD_BRC_NUM; i++)
		if (!strcmp(bname, GoodBrd.ID[i]))
			return i + 1;
	return 0;
}

/* 将版面阅读控制列表load进全局变量 restrict_boards */
void
load_restrict_boards(void)
{
	FILE *fp;
	char boardctl[STRLEN];
	struct stat st;

	sethomefile(boardctl, currentuser.userid, "board.ctl");
	if (stat(boardctl, &st) == -1 || st.st_size <= 0)
		return;

	if ((fp = fopen(boardctl, "r")) == NULL)
		return;

	restrict_boards = (char *)calloc(1, 2048);
	fread(restrict_boards, 1, 2048, fp);

	fclose(fp);
}

void
load_zapbuf(void)
{
	char fname[STRLEN];
	int fd, size, n;

	size = MAXBOARD * sizeof (int);
	zapbuf = (int *) malloc(size);
	for (n = 0; n < MAXBOARD; n++)
		zapbuf[n] = 1;
	setuserfile(fname, ".lastread");
	if ((fd = open(fname, O_RDONLY, 0600)) != -1) {
		size = numboards * sizeof (int);
		read(fd, zapbuf, size);
		close(fd);
	}
}

void
load_GoodBrd(void)
{
	int i;
	char fname[PATH_MAX + 1], bname[BRC_STRLEN + 1], *ptr;
	FILE *fp;

	GoodBrd.num = 0;
	if (GoodBrd.type == SYS_GOOD_BRD)
		sprintf(fname,"etc/sysgoodbrd");
	else 
		setuserfile(fname, ".goodbrd");
	if ((fp = fopen(fname, "r")) != NULL) {
		for (i = 0; i < GOOD_BRC_NUM; i++) {
			if (fgets(bname, sizeof(bname), fp) == NULL)
				break;
			if ((ptr = strrchr(bname, '\n')) != NULL)
				*ptr = '\0';
			if (getbnum(bname)) {
				strlcpy(GoodBrd.ID[i], bname, sizeof(GoodBrd.ID[i]));
				++GoodBrd.num;
			} else {
				--i;
			}
		}
		fclose(fp);
	}

	if (GoodBrd.num == 0) {
		GoodBrd.num = 1;
		if (getbcache(DEFAULTFAVBOARD)) {
			strcpy(GoodBrd.ID[0], DEFAULTFAVBOARD);
		} else {
			strcpy(GoodBrd.ID[0], currboard);
		}
	}
}

void
save_GoodBrd(void)
{
	int i;
	FILE *fp;
	char fname[STRLEN];
	
	if (GoodBrd.type == SYS_GOOD_BRD)
		return ;
	if (GoodBrd.num <= 0) {
		GoodBrd.num = 1;
		if (getbcache(DEFAULTFAVBOARD)) {
			strcpy(GoodBrd.ID[0], DEFAULTFAVBOARD);
		} else {
			strcpy(GoodBrd.ID[0], currboard);
		}
	}
	setuserfile(fname, ".goodbrd");
	if ((fp = fopen(fname, "w")) != NULL) {
		for (i = 0; i < GoodBrd.num && i < GOOD_BRC_NUM; i++)
			fprintf(fp, "%s\n", GoodBrd.ID[i]);
		fclose(fp);
	}
}

void
save_zapbuf(void)
{
	char fname[STRLEN];
	int fd, size;

	setuserfile(fname, ".lastread");
	if ((fd = open(fname, O_WRONLY | O_CREAT, 0600)) != -1) {
		size = numboards * sizeof (int);
		write(fd, zapbuf, size);
		close(fd);
	}
}

char
get_brdstatus(int level, int flag)
{
	if (flag & BRD_READONLY)
		return 'P';

	if (level & PERM_POSTMASK)
		return 'p';

	if (level & PERM_NOZAP)
		return 'z';

	if (flag & NOREPLY_FLAG)
		return 'x';

	if ((level & ~PERM_POSTMASK) != 0)
		return 'r';

	return ' ';
}

int
load_boards(int complete)
{
	struct boardheader *bptr;
	struct newpostdata *ptr;
	int n, addto = 0, goodbrd = 0;

	resolve_boards();
	if (zapbuf == NULL)
		load_zapbuf();

	brdnum = 0;
	if (GoodBrd.num == 9999) {
		load_GoodBrd();
		goodbrd = 1;
	}
	for (n = 0; n < numboards; n++) {
		bptr = &bcache[n];
		if (!(bptr->filename[0]))
			continue;	/* 隐藏被删除的版面 */

		if (goodbrd == 0) {
			/* 隐藏非直接从属于当前列表的版面 */
			if (bptr->parent != boardlevel && !complete)
				continue;

			if (!(bptr->level & PERM_POSTMASK) &&
			    !HAS_PERM(bptr->level) &&
			    !(bptr->level & PERM_NOZAP))
				continue;
			if ( boardprefix != NULL &&
			     boardlevel == 0 &&	 /* freestyler: 不是load分组内版面 */
			    strchr(boardprefix, bptr->title[0]) == NULL &&
			    boardprefix[0] != '*')
				continue;
			if (boardprefix != NULL && boardprefix[0] == '*') {
				if (!strstr(bptr->title, "●") &&
				    !strstr(bptr->title, "⊙") &&
				    bptr->title[0] != '*')
					continue;
			}
			if (boardprefix == NULL && bptr->title[0] == '*')
				continue;
			addto = yank_flag || zapbuf[n] != 0 ||
			    (bptr->level & PERM_NOZAP);
		} else {
			addto = inGoodBrds(bptr->filename) && !(bptr->flag & BRD_GROUP);
		}

#ifdef AUTHHOST
/* --add by betterman */
		if(!valid_host_mask) { /* 校外 */
			if(addto && bptr->flag & BRD_INTERN) {
				addto = HAS_PERM(PERM_SYSOP) ||
					HAS_PERM(PERM_BOARDS) ||
					HAS_PERM(PERM_BLEVELS) ||
					HAS_PERM(PERM_INTERNAL);
			}

			/* freestyler: 半开放版面校外激活用户可访问 */
			if(addto && bptr->flag & BRD_HALFOPEN) {
				addto = HAS_PERM(PERM_SYSOP) ||
					HAS_PERM(PERM_WELCOME);
			}
		}
#endif

		if (bptr->flag & BRD_RESTRICT) {
			if (restrict_boards == NULL || strsect(restrict_boards, bptr->filename, "\n\t ") == NULL)
				addto = 0;
		}

		if (addto) {
			ptr = &nbrd[brdnum++];
			ptr->name = bptr->filename;
			ptr->title = bptr->title;
			ptr->BM = bptr->BM;
			ptr->flag = bptr->flag | ((bptr->level & PERM_NOZAP) ? NOZAP_FLAG : 0);
			ptr->pos = n;
			ptr->total = -1;
			ptr->zap = (zapbuf[n] == 0);
			ptr->status = get_brdstatus(bptr->level, bptr->flag);
#ifdef INBOARDCOUNT
			ptr->inboard = board_setcurrentuser(n, 0);
#endif
		}

	}
	if (brdnum == 0 && !yank_flag && boardprefix == NULL) {
		if (goodbrd) {
			GoodBrd.num = 0;
			save_GoodBrd();
			GoodBrd.num = 9999;
		}
		brdnum = -1;
		yank_flag = 1;
		return -1;
	}
	return 0;
}

int
search_board(int *num)
{
	static int i = 0, find = YEA;
	static char bname[STRLEN];
	int n, ch, tmpn = NA;

	if (find == YEA) {
		memset(bname, 0, sizeof (bname));
		find = NA;
		i = 0;
	}
	while (1) {
		clear_line(t_lines - 1);
		prints("请输入要找寻的 board 名称：%s", bname);
		ch = egetch();

		if (isprint2(ch)) {
			bname[i++] = ch;
			for (n = 0; n < brdnum; n++) {
				if (!strncasecmp(nbrd[n].name, bname, i)) {
					tmpn = YEA;
					*num = n;
					if (!strcmp(nbrd[n].name, bname))
						return 1;	// 找到类似的版，画面重画
				}
			}
			if (tmpn)
				return 1;
			if (find == NA) {
				bname[--i] = '\0';
			}
			continue;
		} else if (ch == Ctrl('H') || ch == KEY_LEFT || ch == KEY_DEL || ch == '\177') {
			i--;
			if (i < 0) {
				find = YEA;
				break;
			} else {
				bname[i] = '\0';
				continue;
			}
		} else if (ch == '\t') {
			find = YEA;
			break;
		} else if (ch == '\n' || ch == '\r' || ch == KEY_RIGHT) {
			find = YEA;
			break;
		}
		bell();
	}
	if (find) {
		clear_line(t_lines - 1);
		return 2;	/* 结束了 */
	}
	return 1;
}

/* Pudding: search board by title */
int
search_brdtitle(char *title, int start, int direction)
{
	int i;
	if (!title || title[0] == '\0')
		return -1;
	for (i = start + direction; i >= 0 && i < brdnum; i += direction) {
		if (strstr2(nbrd[i].title + 1, title))
			return i;
	}
	return -1;
}

/* monster: adopted from ytht */
int
check_newpost(struct newpostdata *ptr)
{
	struct boardheader *bptr;

	ptr->total = ptr->unread = 0;

	bptr = getbcache(ptr->name);
	if (bptr == NULL)
		return 0;
	ptr->total = bptr->total;

	if (!brc_initial(ptr->name)) { /* return 0, 没找到记录 */
		ptr->unread = 1;
	} else {
		if (brc_unread(bptr->lastpost)) {
			ptr->unread = 1;
		}
	}
	return 1;
}

int
unread_position(char *dirfile, struct boardheader *ptr)
{
	struct fileheader fh;
	int fd, offset, step, num;
	time_t filetime;

	num = ptr->total + 1;
	if ((fd = open(dirfile, O_RDWR)) > 0) {
		if (!brc_initial(ptr->filename)) {
			num = 1;
		} else {
			offset = (int)((char *) &(fh.filetime) - (char *) &(fh));
			num = ptr->total - 1;
			step = 4;
			while (num > 0) {
				lseek(fd, (off_t) (offset + num * sizeof (fh)), SEEK_SET);
				if (read(fd, &filetime, sizeof(filetime)) <= 0 || !brc_unread(filetime))
					break;
				num -= step;
				if (step < 32)
					step += step / 2;
			}
			if (num < 0)
				num = 0;
			while (num < ptr->total) {
				lseek(fd, (off_t) (offset + num * sizeof(fh)), SEEK_SET);
				if (read(fd, &filetime, sizeof(filetime)) <= 0 || brc_unread(filetime))
					break;
				num++;
			}
		}
		close(fd);
	}

	return (num < 0) ? 0 : num;
}

void
show_brdlist(int page, int clsflag, int newflag)
{
	struct newpostdata *ptr;
	int n;
	char tmpBM[BMLEN];

	if (clsflag) {
		clear();
		docmdtitle("[讨论区列表]",
			   " \033[m主选单[\033[1;32m←\033[m,\033[1;32mq\033[m] 阅读[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 列出[\033[1;32my\033[m] 排序[\033[1;32ms\033[m] 搜寻[\033[1;32m/\033[m] 切换[\033[1;32mc\033[m] 求助[\033[1;32mh\033[m]\n");
#ifdef INBOARDCOUNT
		  prints("\033[1;44;37m %s 讨论区名称       V  类别  转 %-20s 在线 S 版  主   %s   \033[m\n",
                        newflag ? "全部  未" : "编号  ", "中  文  叙  述",
                        newflag ? "" : "   ");
#else
		prints("\033[1;44;37m %s 讨论区名称       V  类别  转 %-25s S 版  主   %s   \033[m\n",
		     	newflag ? "全部  未" : "编号  ", "中  文  叙  述",
		     	newflag ? "" : "   ");
#endif
	}
	move(3, 0);
	for (n = page; n < page + BBS_PAGESIZE; n++) {
		if (n >= brdnum) {
			outc('\n');
			continue;
		}
		ptr = &nbrd[n];

		if (ptr->flag & BRD_GROUP) {
			if (!newflag) {
				prints(" %4d＋%c%-16s   %-35s   [目录]\n", n + 1,
					(ptr->zap && !(ptr->flag & NOZAP_FLAG)) ? '*' : ' ',
					ptr->name, ptr->title + 1);
			} else {
				prints("       ＋%c%-16s   %-35s   [目录]\n",
					(ptr->zap && !(ptr->flag & NOZAP_FLAG)) ? '*' : ' ',
					ptr->name, ptr->title + 1);
			}
			continue;
                }

		if (!newflag) {
			prints(" %4d  ", n + 1);
		} else {
			if (ptr->total == -1) {
				refresh();
				check_newpost(ptr);
			}
			prints(" %4d%s%s", ptr->total, ptr->total > 9999 ? " " :"  ", ptr->unread ? "◆" : "◇");
		}
		strlcpy(tmpBM, ptr->BM, sizeof(tmpBM));

#ifdef INBOARDCOUNT
		/* inboard user count  by freestyler */
		int idx = getbnum(ptr->name);
		int inboard = board_setcurrentuser(idx-1, 0);
		if ( /* YEA == check_readonly(ptr->name) */ ptr->status == 'P') {
			prints("%c%-16s %s [\033[1;31m只读\033[m] %-23s%4d  %c %-12s\n",
				(ptr->zap && !(ptr->flag & NOZAP_FLAG)) ? '*' : ' ',
				ptr->name,
				(ptr->flag & VOTE_FLAG) ? "\033[1;31mV\033[m" : " ",
				ptr->title + 8,
				inboard,  /* added by freestyler */
				HAS_PERM(PERM_POST) ? ptr->status : ' ',
				(ptr->BM[0] == ' ' || ptr->BM[0] == 0) ? "诚征版主中" :
				strtok(tmpBM, " "));
                } else {
                        prints("%c%-16s %s %-30s%4d  %c %-12s\n",
				(ptr->zap && !(ptr->flag & NOZAP_FLAG)) ? '*' : ' ',
				ptr->name,
				(ptr->flag & VOTE_FLAG) ? "\033[1;31mV\033[m" : " ",
				ptr->title + 1,
				inboard,  /* added by freestyler */
				HAS_PERM(PERM_POST) ? ptr->status : ' ',
				(ptr->BM[0] == ' ' || ptr->BM[0] == 0) ? "诚征版主中" :
				strtok(tmpBM, " "));
                }
#else

		if ( /* YEA == check_readonly(ptr->name) */ ptr->status == 'P') {
			prints("%c%-16s %s [\033[1;31m只读\033[m] %-28s %c %-12s\n",
			       (ptr->zap && !(ptr->flag & NOZAP_FLAG)) ? '*' : ' ',
			       ptr->name, 
				(ptr->flag & VOTE_FLAG) ? "\033[1;31mV\033[m" : " ",
			       ptr->title + 8,
			       HAS_PERM(PERM_POST) ? ptr->status : ' ',
			       (ptr->BM[0] == ' ' || ptr->BM[0] == 0) ? "诚征版主中" : strtok(tmpBM, " "));
		} else {
			prints("%c%-16s %s %-35s %c %-12s\n",
			       (ptr->zap && !(ptr->flag & NOZAP_FLAG)) ? '*' : ' ',
			       ptr->name,
			       (ptr->flag & VOTE_FLAG) ? "\033[1;31mV\033[m" : " ",
			       ptr->title + 1,
			       HAS_PERM(PERM_POST) ? ptr->status : ' ',
			       (ptr->BM[0] == ' ' || ptr->BM[0] == 0) ? "诚征版主中" : strtok(tmpBM, " "));
		}
#endif
	}
	refresh();
}

int
cmpboard(const void *brd_ptr, const void *tmp_ptr)
{
	int type = 0;
	struct newpostdata *brd = (struct newpostdata *)brd_ptr;
	struct newpostdata *tmp = (struct newpostdata *)tmp_ptr;
	if (!(currentuser.flags[0] & BRDSORT_FLAG)) {
		type = brd->title[0] - tmp->title[0];
		if (type == 0)
			type = strncasecmp(brd->title + 1, tmp->title + 1, 6);
	}
#ifdef INBOARDCOUNT
	else if( currentuser.flags[0] & BRDSORT_FLAG2 ) 
		type = tmp->inboard - brd->inboard ;
#endif
	return (type == 0) ? strcasecmp(brd->name, tmp->name) : type;
}


int
gettheboardname(int x, char *title, int *pos, struct boardheader *fh, char *bname)
{
	move(x, 0);
	make_blist();
	namecomplete(title, bname);
	if (*bname == '\0') {
		return 0;
	}
	*pos = search_record(BOARDS, fh, sizeof (struct boardheader), cmpbnames,
			     bname);
	if (!(*pos)) {
		move(x + 3, 0);
		prints("不正确的讨论区名称");
		pressreturn();
		clear();
		return 0;
	}
	return 1;
}

int
setboardlevel(char *bname)
{
	int i, oldlevel = boardlevel;
             
	char* old_boardprefix  = boardprefix;
	boardprefix = NULL;
	if (load_boards(YEA) == -1 || brdnum <= 0)
		return -1;

	boardprefix = old_boardprefix;
	qsort(nbrd, brdnum, sizeof(nbrd[0]), cmpboard);

	/* monster: locate boardlevel for specific filename */
	for (i = 0; i < brdnum; i++) {
		if (!strcmp(bname, nbrd[i].name)) {
			boardlevel = nbrd[i].pos + 1;
			return oldlevel;
		}
	}

	return -1;
}

int
choose_board(int newflag)
{
	static int num;
	static char search_title[BTITLELEN] = "";
	struct newpostdata *ptr;
	int page = 0, ch = 0, tmp, number, tmpnum;
	int loop_mode = 0, old_mode, pos;
	struct boardheader fh;

start:
	if (guestuser)
		yank_flag = 1;
	modify_user_mode(newflag ? READNEW : READBRD);
	brdnum = number = 0;
	clear();

	while (1) {
		if (brdnum <= 0) {
			if (load_boards(NA) == -1 || brdnum <= 0)
				break;
			qsort(nbrd, brdnum, sizeof(nbrd[0]), cmpboard);
			page = -1;
			num  = 0;
		}
		if (num < 0)
			num = 0;
		if (num >= brdnum)
			num = brdnum - 1;
		if (page < 0) {
			if (newflag) {
				tmp = num;
				while (num < brdnum) {
					ptr = &nbrd[num];
					if (!(ptr->flag & BRD_GROUP)) {
						/*if (ptr->total == -1) */
							check_newpost(ptr);
						if (ptr->unread)
							break;
					}
					num++;
				}
				if (num >= brdnum) 
					num = tmp;
			}
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE; /* 第page项 == 显示第一项 */
			show_brdlist(page, 1, newflag);
			update_endline();
		}
		if (num < page || num >= page + BBS_PAGESIZE) {
			page = (num / BBS_PAGESIZE) * BBS_PAGESIZE;
			show_brdlist(page, 0, newflag);
			update_endline();
		}
		move(3 + num - page, 0);
		prints(">", number);
		if (loop_mode == 0) {
			ch = egetch();
		}
		move(3 + num - page, 0);
		outs(" ");
		if (ch == 'q' || ch == KEY_LEFT || ch == EOF)
			break;
		switch (ch) {
		case 'P':
		case 'b':
		case Ctrl('B'):
		case KEY_PGUP:
			if (num == 0)
				num = brdnum - 1;
			else
				num -= BBS_PAGESIZE;
			break;
		case 'C':
		case 'c':
			if (newflag == 1)
				newflag = 0;
			else
				newflag = 1;
			show_brdlist(page, 1, newflag);
			break;
		case 'L':
			show_allmsgs();
			page = -1;
			break;
		case 'l':
			m_read();
			page = -1;
			break;
		case 'w':
		case 'M':
			m_new();
			page = -1;
			break;
		case 'u':
			modify_user_mode(QUERY);
			t_query();
			page = -1;
			break;
		case 'H':
			{
				show_help("0Announce/bbslist/day");
				if (HAS_PERM(PERM_SYSOP | PERM_OBOARDS))
					show_help("0Announce/bbslist/day2");	/* 篇数统计*/
				page = -1;
				break;
			}
		case Ctrl('V'):	/* monster: 快速锁屏 */
			x_lockscreen_silent();
			break;
		case 'X':	/* monster: 快速封版 */
			if (HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_OBOARDS)) {
				int pos;
				struct boardheader fh;

				if ((pos = search_record(BOARDS, &fh, sizeof(struct boardheader), cmpbnames, nbrd[num].name)) > 0) {
					if (fh.flag & BRD_READONLY) {
						if (!strcmp(nbrd[num].name, "syssecurity"))
							break;
						fh.flag &= ~(BRD_READONLY);
						snprintf(genbuf, sizeof(genbuf), "解开只读讨论区 %s ", nbrd[num].name);
					} else {
						fh.flag |= BRD_READONLY;
						snprintf(genbuf, sizeof(genbuf), "只读讨论区 %s ", nbrd[num].name);
					}

					if (substitute_record(BOARDS, &fh, sizeof (struct boardheader), pos) == 0) {
						nbrd[num].status = get_brdstatus(fh.level, fh.flag);
						securityreport(genbuf);
						show_brdlist(page, 0, newflag);
					}
					refresh_bcache();
				}
			}
			break;
		case Ctrl('A'): /* monster: 显示版面属性 */
			if ((pos = search_record(BOARDS, &fh, sizeof(struct boardheader), cmpbnames, nbrd[num].name)) > 0) {
				m_editboard(&fh, pos, YEA);
				show_brdlist(page, 1, newflag);
			}
			break;
		case Ctrl('E'):	/* monster: 修改版面属性 */
			if (HAS_PERM(PERM_SYSOP) && check_systempasswd() && checkgroupinfo()) {
				if ((pos = search_record(BOARDS, &fh, sizeof(struct boardheader), cmpbnames, nbrd[num].name)) > 0) {
					old_mode = uinfo.mode;
					modify_user_mode(ADMIN);
					m_editboard(&fh, pos, NA);
					modify_user_mode(old_mode);
					show_brdlist(page, 1, newflag);
				}
			}
			break;
		case 'N':
		case ' ':
		case Ctrl('F'):
		case KEY_PGDN:
			if (num == brdnum - 1)
				num = 0;
			else
				num += BBS_PAGESIZE;
			break;
		case 'p':
		case 'k':
		case KEY_UP:
			if (num-- <= 0)
				num = brdnum - 1;
			break;
		case 'n':
		case 'j':
		case KEY_DOWN:
			if (++num >= brdnum)
				num = 0;
			break;
		case '$':
			num = brdnum - 1;
			break;
		case '!':		/* youzi leave */
			return Q_Goodbye();
			break;		// should never returns
		case 'h':
			show_help("help/boardreadhelp");
			page = -1;
			break;
		case '/':
			move(3 + num - page, 0);
			prints(">", number);
			tmpnum = num;
			tmp = search_board(&num);
			move(3 + tmpnum - page, 0);
			prints(" ", number);
			if (tmp == 1)
				loop_mode = 1;
			else {
				loop_mode = 0;
				update_endline();
			}
			break;
		case '?':			/* Pudding: search board by title */
			getdata(t_lines - 1, 0,
				"搜索中文描述: ",
				search_title, sizeof(search_title) - 1,
				YEA, NA);
			update_endline();
			if (search_title[0] == '\0') break;
			
			tmp = search_brdtitle(search_title, num, 1);
			if (tmp >= 0) num = tmp;
			break;
		case 's':
#ifdef INBOARDCOUNT
			/* sort by name, sort by inboard users, sort by title --freestyler */
			/* 三个状态,用到了flags[0]的两位,  11 --> 00 --> 10 --> 11 */
			if( currentuser.flags[0] & BRDSORT_FLAG )  { 
				if( currentuser.flags[0] & BRDSORT_FLAG2 )  /* 11--> 00 */ 
					currentuser.flags[0] &= ~(BRDSORT_FLAG | BRDSORT_FLAG2);
				else  /* 10--> 11 */
					currentuser.flags[0] |= BRDSORT_FLAG2 ;
			}  else      /* 00 -> 10 */
				currentuser.flags[0] |= BRDSORT_FLAG;
			
#else
			/* sort/unsort -mfchen */
			currentuser.flags[0] ^= BRDSORT_FLAG;
#endif
			qsort(nbrd, brdnum, sizeof (nbrd[0]), cmpboard);
			page = 999;
			break;
		case 'y':
			if (GoodBrd.num)
				break;
			yank_flag = !yank_flag;
			brdnum = -1;	/* to load_boards */
			break;
		case 'z':
			if (GoodBrd.num)
				break;
			if (HAS_PERM(PERM_BASIC) &&
			    !(nbrd[num].flag & NOZAP_FLAG)) {
				ptr = &nbrd[num];
				ptr->zap = !ptr->zap;
				ptr->total = -1;
				zapbuf[ptr->pos] =
				    (ptr->zap ? 0 : login_start_time);
				page = 999;
			}
			break;
		case 'a':
			if (GoodBrd.num > 0 && GoodBrd.type == USER_GOOD_BRD) {
				int pos;
				char bname[STRLEN];
				struct boardheader fh;

				if (GoodBrd.num >= GOOD_BRC_NUM) {
					presskeyfor("个人热门版数已经达上限");
					break;
				}

				if (gettheboardname(1, "输入讨论区名 (按空白键自动搜寻): ", &pos, &fh, bname)) {
					if (!inGoodBrds(bname)) {
						strcpy(GoodBrd.ID[GoodBrd.num++], bname);
						save_GoodBrd();
						GoodBrd.num = 9999;
						brdnum = -1;
						break;
					}
				}
				page = -1;
				/* Added by cancel at 01/10/24, modified by monster */
			} else if (GoodBrd.num > 0 && GoodBrd.type == SYS_GOOD_BRD){
				GoodBrd.type = USER_GOOD_BRD;
				load_GoodBrd();

				if (GoodBrd.num >= GOOD_BRC_NUM) {
					presskeyfor("个人热门版数已经达上限");
					break;
				}

				if (!inGoodBrds(nbrd[num].name)) {
					strlcpy(GoodBrd.ID[GoodBrd.num++], nbrd[num].name, sizeof(GoodBrd.ID[0]));
					save_GoodBrd();
					snprintf(genbuf, sizeof(genbuf), "已经将 %s 加到您的个人收藏夹里, 请按任意键继续...", nbrd[num].name);
					presskeyfor(genbuf);
					GoodBrd.num = 9999;
					brdnum = -1;
				}
				GoodBrd.type = SYS_GOOD_BRD;
				/* Added by betterman at 05/11/23 */
			} else {
				load_GoodBrd();

				if (GoodBrd.num >= GOOD_BRC_NUM) {
					presskeyfor("个人热门版数已经达上限");
					break;
				}

				if (!inGoodBrds(nbrd[num].name)) {
					strlcpy(GoodBrd.ID[GoodBrd.num++], nbrd[num].name, sizeof(GoodBrd.ID[0]));
					save_GoodBrd();
					snprintf(genbuf, sizeof(genbuf), "已经将 %s 加到您的个人收藏夹里, 请按任意键继续...", nbrd[num].name);
					presskeyfor(genbuf);
					brdnum = -1;
				}
				GoodBrd.num = 0;
			}
			/* Added End. */
			break;
		case 'd':
			if (guestuser)
				break;
			if (GoodBrd.num && GoodBrd.type == USER_GOOD_BRD) {
				int i, pos;
				char ans[5];

				snprintf(genbuf, sizeof(genbuf), "要把 %s 从收藏夹中去掉？[y/N]", nbrd[num].name);
				getdata(t_lines - 1, 0, genbuf, ans, 2, DOECHO, YEA);
				if (ans[0] == 'y' || ans[0] == 'Y') {
					pos = inGoodBrds(nbrd[num].name);
					for (i = pos - 1; i < GoodBrd.num - 1; i++)
						strcpy(GoodBrd.ID[i],
						       GoodBrd.ID[i + 1]);
					GoodBrd.num--;
					save_GoodBrd();
					GoodBrd.num = 9999;
					brdnum = -1;
				} else {
					page = -1;
				}
				
			}
			break;
		case KEY_HOME:
			num = 0;
			break;
		case KEY_END:
			num = brdnum - 1;
			break;
		case '\n':
		case '\r':
			if (number > 0) {
				num = number - 1;
				break;
			}
			/* fall through */
		case KEY_RIGHT:
			{
				char buf[STRLEN];
				struct boardheader *bp;
				int oldnum = num;

				ptr = &nbrd[num];
				if (ptr->flag & BRD_GROUP) {
					if ((bp = getbcache(ptr->name)) != NULL) {
						GoodBrd.num = 0;
						boardlevel = ptr->pos + 1;
						brdnum = -1;
						newflag = choose_board(newflag);
						boardlevel = bp->parent;
						num = oldnum;
						goto start;
					}
				} else {
					brc_initial(ptr->name); /* 初始化未读标记 */
					current_bm = (HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
						      check_bm(currentuser.userid, ptr->BM)) ||
						      HAS_PERM(PERM_BLEVELS);
					if (DEFINE(DEF_FIRSTNEW)) {
						if ((bp = getbcache(currboard)) != NULL) {
							setbdir(buf, currboard);
							tmp = unread_position(buf, bp);
							page = tmp - t_lines / 2;
							getkeep(buf, page > 1 ? page : 1, tmp + 1);
						}
					}
					Read();

					if (zapbuf[ptr->pos] > 0 && brc_num > 0) {
						zapbuf[ptr->pos] = brc_list[0];
					}
				}
				page = -1;
				break;
			}
		case 'S':	/* sendmsg ... youzi */
			if (!HAS_PERM(PERM_MESSAGE))
				break;
			s_msg();
			page = -1;
			break;
		case 'f':	/* show friends ... youzi */
			if (!HAS_PERM(PERM_BASIC))
				break;
			t_friends();
			page = -1;
			break;
		default:
			;
		}
		modify_user_mode(newflag ? READNEW : READBRD);
		if (ch >= '0' && ch <= '9') {
			number = number * 10 + (ch - '0');
			ch = '\0';
		} else {
			number = 0;
		}
	}
	clear();
	save_zapbuf();
	return newflag;
}


/*将record从ptr复制到name, pnum, list */
char *
brc_getrecord(char *ptr, char *name, int *pnum, int *list)
{
	int num;
	char *tmp;

	strlcpy(name, ptr, BRC_STRLEN);
	ptr += BRC_STRLEN;
	num = (unsigned char)(*ptr++);
	tmp = ptr + num * sizeof (int);
	if (num > BRC_MAXNUM) {
		num = BRC_MAXNUM;
	}
	*pnum = num;
	memcpy(list, ptr, num * sizeof (int));
	return tmp;
}

/* 将记录(name, num, list) put到ptr所指内存, 返回新内存地址 */
char *
brc_putrecord(char *ptr, char *name, int num, int *list)
{
	if (num > 0) {
		if (num > BRC_MAXNUM) {
			num = BRC_MAXNUM;
		}
		strlcpy(ptr, name, BRC_STRLEN);
		ptr += BRC_STRLEN;
		*ptr++ = num;
		memcpy(ptr, list, num * sizeof (int));
		ptr += num * sizeof (int);
	}
	return ptr;
}

/*
 *  將目前的 brc data 寫入 .boardrc 中。
    額外效果：如果 brc data 未被更改或使用者權限不足則不會有動作。
 */
void
brc_update(void)
{
	char dirfile[STRLEN], *ptr;
	char tmp_buf[BRC_MAXSIZE - BRC_ITEMSIZE], *tmp;
	char tmp_name[BRC_STRLEN];
	int tmp_list[BRC_MAXNUM], tmp_num;
	int fd, tmp_size;

	if (brc_changed == 0 || guestuser)
		return;
	ptr = brc_buf;
	if (brc_num > 0) {
		/* 先写改动的数据(currboard's) */
		ptr = brc_putrecord(ptr, brc_name, brc_num, brc_list);
	}
	if (1) {
		setuserfile(dirfile, ".boardrc");
		if ((fd = open(dirfile, O_RDONLY)) != -1) {
			tmp_size = read(fd, tmp_buf, sizeof (tmp_buf));
			close(fd);
		} else {
			tmp_size = 0;
		}
	}
	tmp = tmp_buf;
	while (tmp < &tmp_buf[tmp_size] && (*tmp >= ' ' && *tmp <= 'z')) {
		tmp = brc_getrecord(tmp, tmp_name, &tmp_num, tmp_list);
		if (strncmp(tmp_name, currboard, BRC_STRLEN) != 0) { /* not currboard */
			ptr = brc_putrecord(ptr, tmp_name, tmp_num, tmp_list);
		}
	}
	brc_size = (int) (ptr - brc_buf);

	if ((fd = open(dirfile, O_WRONLY | O_CREAT, 0644)) != -1) {
		ftruncate(fd, 0);
		write(fd, brc_buf, brc_size);
		close(fd);
	}
	brc_changed = 0;
}

int
brc_initial(char *boardname)		//deardragon0912
{
	char dirfile[STRLEN], *ptr;
	int fd;

	if (strcmp(currboard, boardname) == 0) {
		return brc_num;
	}
	brc_update();
	strcpy(currboard, boardname);
	brc_changed = 0;
	if (brc_buf[0] == '\0') {
		setuserfile(dirfile, ".boardrc");
		if ((fd = open(dirfile, O_RDONLY)) != -1) {
			brc_size = read(fd, brc_buf, sizeof (brc_buf));
			close(fd);
		} else {
			brc_size = 0;
		}
	}
	brc_cur = 0;
	ptr = brc_buf;
	while (ptr < &brc_buf[brc_size] && (*ptr >= ' ' && *ptr <= 'z')) {
		ptr = brc_getrecord(ptr, brc_name, &brc_num, brc_list);
		if (strncmp(brc_name, currboard, BRC_STRLEN) == 0) { /* 找到记录 */
			return brc_num;
		}
	}
	strlcpy(brc_name, boardname, BRC_STRLEN);
	brc_list[0] = 1;
	brc_num = 1;
	return 0;
}

/* 如果找到, num==brc_list[brc_cur]
 * 否则, brc_cur指向num应该插入的位置
 *                              ecnegrevid 2001.6.18
 */
int
brc_locate(int num)
{
	if (brc_num == 0) {
		brc_cur = 0;
		return 0;
	}
	if (brc_cur >= brc_num)
		brc_cur = brc_num - 1;
	if (num <= brc_list[brc_cur]) {
		while (brc_cur < brc_num) {
			if (num == brc_list[brc_cur])
				return 1;
			if (num > brc_list[brc_cur])
				return 0;
			brc_cur++;
		}
		return 0;
	}
	while (brc_cur > 0) {
		if (num < brc_list[brc_cur - 1])
			return 0;
		brc_cur--;
		if (num == brc_list[brc_cur])
			return 1;
	}
	return 0;
}

/* 将num插入到brc_cur指示的位置 */
void
brc_insert(int num)
{
	if (brc_num < BRC_MAXNUM)
		brc_num++;
	if (brc_cur < brc_num) {
		memmove(&brc_list[brc_cur + 1], &brc_list[brc_cur],
			sizeof (brc_list[0]) * (brc_num - brc_cur - 1));
		brc_list[brc_cur] = num;
		brc_changed = 1;
	}
}

void
brc_addlist(time_t filetime)
{
	if (filetime > 0 && brc_unread(filetime))
		brc_insert(filetime);
}

int
brc_unread(time_t filetime)
{
	if (brc_locate((int)filetime))  /* 已存在 */
		return 0;
	if (brc_num <= 0)		/* 已读为0*/
		return 1;
	if (brc_cur < brc_num)		/* brc_cur所指位置在brc_list范围内, so未读 */
		return 1;
	return 0;			/* brc_cur >= brc_num, the file is too old */
}

/* monster: add for chat.c, written by yy, modified by me */
int
c_mygrp_unread(struct boardheader *bhp)
{
	static int num = 0;
	static int found = 0;
	struct newpostdata npd;
	static char pline[256] = "";

	if (bhp == NULL) {
		if (!found) {
			printchatline("还没有新文章喔");
		} else {
			printchatline(pline);
		}
		pline[0] = '\0';
		return (num = found = 0);	//init the number
	}

	if (!inGoodBrds(bhp->filename))
		return 0;

	npd.name = bhp->filename;
	npd.title = bhp->title;
	npd.BM = bhp->BM;
	npd.flag = bhp->flag | ((bhp->level & PERM_NOZAP) ? NOZAP_FLAG : 0);
	npd.pos = num - 1;
	npd.total = -1;
	npd.zap = 0;
	npd.unread = 0;

	if (check_newpost(&npd) && npd.unread) {
		found++;
		snprintf(genbuf, sizeof(genbuf), "%-16s", npd.name);
		strlcat(pline, genbuf, sizeof(pline));
		if (found % 4 == 0) {
			printchatline(pline);
			pline[0] = '\0';
		}
	}

	return 1;
}

int
new_flag_clearall(struct boardheader *bhp)
{
	if (bhp->filename[0] == 0)
		return 1;
	brc_initial(bhp->filename);
	brc_num = 1;
	brc_cur = 0;
	brc_list[0] = time(NULL);
	brc_changed = 1;
	brc_update();

	return 1;
}

/* monster: 清除全站未读标记 */
int
flag_clear_allboards(void)
{
	clear();
	move(t_lines - 1, 0);

	if (askyn("确定要清除所有版面的未读标记？", NA, YEA) == NA) {
		return PARTUPDATE;
	} else {
		clear_line(t_lines - 1);
		prints("清除中。。。");
		refresh();
	}

	resolve_boards();
	apply_boards(new_flag_clearall);
	return FULLUPDATE;
}

int
EGroup(char *cmd)
{
	char buf[STRLEN] = "EGROUP1";

	buf[6] = *cmd;
	GoodBrd.num = 0;
	boardprefix = sysconf_str(buf); 
	choose_board(DEFINE(DEF_NEWPOST) ? 1 : 0);
	return 0;
}

int
Boards(void)
{
	boardprefix = NULL;
	GoodBrd.num = 0;
	choose_board(0);
	return 0;
}

int
GoodBrds(void)
{
	GoodBrd.num = 9999;
	if (guestuser) 
		GoodBrd.type = SYS_GOOD_BRD;
	else
		GoodBrd.type = USER_GOOD_BRD;
	boardprefix = NULL;
	choose_board(1);
	return 0;
}

int
SysGoodBrds(void)
{
	GoodBrd.num = 9999;
	GoodBrd.type = SYS_GOOD_BRD;
	boardprefix = NULL;
	choose_board(1);
	return 0;
}

int
New(void)
{
/*	monster: 现行代码应该能在高负荷下正常工作，故取消负荷检查

	if (heavyload()) {
		clear();
		prints
		    ("抱歉，目前系统负荷过重，请改用 Boards 指令阅览讨论区...");
		pressanykey();
		return;
	}
*/
	boardprefix = NULL;
	GoodBrd.num = 0;
	choose_board(1);
	return 0;
}


