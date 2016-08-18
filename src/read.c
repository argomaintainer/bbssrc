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

#define PUTCURS   move(3+locmem->crs_line-locmem->top_line,0);prints(">");
#define RMVCURS   move(3+locmem->crs_line-locmem->top_line,0);prints(" ");

extern int noreply;
extern int local_article;
extern int current_bm;

extern char attach_link[1024];	/* 附件链接 */
extern char attach_info[1024];	/* 附件信息 */

char currdirect[PATH_MAX + 1];
char keyword[STRLEN];		/* for 相关主题 */
int screen_len;
int last_line;
int previewmode = 0;		/* gcc: 预览模式 */

struct keeploc *
getkeep(char *s, int def_topline, int def_cursline)
{
	static struct keeploc *keeplist = NULL;
	struct keeploc *p;

	for (p = keeplist; p != NULL; p = p->next) {
		if (!strcmp(s, p->key)) {
			if (p->crs_line < 1)
				p->crs_line = 1;	/* DAMMIT! - rrr */
			return p;
		}
	}
	p = (struct keeploc *) malloc(sizeof (*p));
	p->key = (char *) malloc(strlen(s) + 1);
	strcpy(p->key, s);
	p->top_line = def_topline;
	p->crs_line = def_cursline;
	p->next = keeplist;
	keeplist = p;

	return p;
}

void
setkeep(char *s, int pos)
{
	int half;
	struct keeploc *k;

	k = getkeep(s, 1, 1);
	if (pos >= k->top_line && pos < k->top_line + t_lines - 4) {
		k->crs_line = pos;
	} else {
		half = (t_lines - 3) / 2;
		k->crs_line = pos;
		k->top_line = (pos <= half ? 1: pos - half);
	}
}

void
fixkeep(char *s, int first, int last)
{
	int half;
	struct keeploc *k;

	k = getkeep(s, 1, 1);
	if (k->crs_line >= first) {
		half = (t_lines - 3) / 2;
		k->crs_line = (first == 1 ? 1 : first - 1);
		k->top_line = (first <= half ? 1 : first - half);
	}
}

void
modify_locmem(struct keeploc *locmem, int total)
{
	if (locmem->top_line > total) {
		locmem->crs_line = total;
		locmem->top_line = total - t_lines / 2;
		if (locmem->top_line < 1)
			locmem->top_line = 1;
	} else if (locmem->crs_line > total) {
		locmem->crs_line = total;
	}
	/* gcc: fix preview mode "文件丢失" */
	if (locmem->crs_line - screen_len >= locmem->top_line) {
		locmem->top_line = locmem->crs_line - screen_len / 2;
		if (locmem->top_line < 1)
			locmem->top_line = 1;
	}
}

/* calc cursor pos and show cursor correctly -cuteyu */
int
cursor_pos(struct keeploc *locmem, int val, int from_top)
{
	if (val > last_line) {
		val = DEFINE(DEF_CIRCLE) ? 1 : last_line;
	}
	if (val <= 0) {
		val = DEFINE(DEF_CIRCLE) ? last_line : 1;
	}
	//if(val>=locmem->top_line&&val<locmem->top_line+screen_len-1){
	if (val >= locmem->top_line && val < locmem->top_line + screen_len) {
		RMVCURS;
		locmem->crs_line = val;
		PUTCURS;
		return NA;
	}
	locmem->top_line = val - from_top;
	if (locmem->top_line <= 0)
		locmem->top_line = 1;
	locmem->crs_line = val;
	return YEA;
}

int
move_cursor_line(struct keeploc *locmem, int mode)
{
	int top, crs;
	int reload = 0;

	top = locmem->top_line;
	crs = locmem->crs_line;
	if (mode == READ_PREV) {
		if (crs <= top) {
			top -= screen_len - 1;
			if (top < 1)
				top = 1;
			reload = 1;
		}
		crs--;
		if (crs < 1) {
			crs = 1;
			reload = -1;
		}
	} else if (mode == READ_NEXT) {
		if (crs + 1 >= top + screen_len) {
			top += screen_len - 1;
			reload = 1;
		}
		crs++;
		if (crs > last_line) {
			crs = last_line;
			reload = -1;
		}
	}
	locmem->top_line = top;
	locmem->crs_line = crs;
	return reload;
}

/* gcc: post preview mode */

void
post_preview(struct keeploc *locmem, char *pnt, int ssize)
{
        struct fileheader *fileinfo =
		(struct fileheader *) &pnt[(locmem->crs_line - locmem->top_line) * ssize];
    
	char *t;
	char buf[512];
	extern int offsetln;
	strcpy(buf, currdirect);
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	snprintf(genbuf, sizeof(genbuf), "%s/%s", buf, fileinfo->filename);
	
	if (!dashf(genbuf)) {
		move(screen_len + 3, 30);
		prints("对不起，本文内容丢失！");
		return;
	}
	offsetln = screen_len + 4;
	ansimore2(genbuf, NA, screen_len + 3, t_lines - screen_len - 4);
	offsetln = 0;
}

void
draw_title(void (*dotitle)())
{
	clear();
	(*dotitle)();
}

void
draw_entry(char *(*doentry)(int, void *), void (*doendline)(), struct keeploc *locmem, char *pnt, int num, int ssize)
{
	char *str;
	int base, i;

	base = locmem->top_line;
	move(3, 0);
	clrtobot();
	for (i = 0; i < num; i++) {
		str = (*doentry) (base + i, &pnt[i * ssize]);
		if (check_stuffmode() == NA) {
			outs(str);
		} else {
			showstuff(str);
		}
		outc('\n');
	}
	
	/* gcc: post preview mode */
	if (previewmode && uinfo.mode == READING)
                post_preview(locmem, pnt, ssize);
	
	doendline();
}

/*
 * freestyler: 返回fhdr在目录文件direct的index(下标0开始) (binary search)
 */ 
int
get_dir_index(char* direct, struct fileheader* fhdr)
{
	int i, fd, result = -1, records, high, low;
	struct stat st;
	struct fileheader *headers;

	if ((fd = open(direct, O_RDONLY, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return -1;
	}

	headers = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	if (headers == MAP_FAILED || st.st_size <= 0) {
		close(fd);
		return -1;
	}

	records = st.st_size / sizeof(struct fileheader);

	TRY
		/* freestyler: binary search variant lower_bound */
		for(low = 0, high = records; low < high; ) {
			i = (high + low) >> 1;
			if (fhdr->filetime <= headers[i].filetime) 
				high = i;
			else 
				low = i + 1;
		}
		// now low is the least i for which p(i) is true,
		// where p(i):= ( headers[i].filetime >= fhdr->filetime )

		for (i = low; i < records; i++) {
			if (headers[i].filetime != fhdr->filetime) 
				break;
			
			if (!strcmp(headers[i].filename+1, fhdr->filename+1)) {
				result = i;
				break;
			}
		}
	CATCH
		result = -1;
	END

	munmap(headers, st.st_size);
	close(fd);
	return result;
}


int
i_read_key(struct one_key *rcmdlist, struct keeploc *locmem, char *pnt, int ch, int ssize)
{
	int i, mode = DONOTHING, savemode;
	char sdirect[PATH_MAX + 1];

	switch (ch) {
	case 'q':
		if (digestmode >0 && digestmode <= 7 && uinfo.mode != RMAIL && uinfo.mode != ADMIN
		    && uinfo.mode != DIGEST && uinfo.mode != DIGESTRACE) {
			struct fileheader *fileinfo =
				(struct fileheader *) &pnt[(locmem->crs_line - locmem->top_line) * ssize];
			acction_mode(locmem->crs_line, fileinfo, currdirect);
			int idx = get_dir_index(currdirect, fileinfo);
			setkeep(currdirect, idx+1);
			return NEWDIRECT;
		} // else falls through
	case KEY_LEFT:
		if (digestmode && uinfo.mode != RMAIL && uinfo.mode != ADMIN
		    && uinfo.mode != DIGEST && uinfo.mode != DIGESTRACE) {
			struct fileheader *fileinfo =
				(struct fileheader *) &pnt[(locmem->crs_line - locmem->top_line) * ssize];
			return acction_mode(locmem->crs_line, fileinfo, currdirect);
		} else {
			return DOQUIT;
		}
	case Ctrl('L'):
		redoscr();
		break;
	case 'w':
	case 'M':
		savemode = uinfo.mode;
		m_new();
		modify_user_mode(savemode);
		return FULLUPDATE;
	case 'u':
		savemode = uinfo.mode;
		modify_user_mode(QUERY);
		t_query();
		modify_user_mode(savemode);
		return FULLUPDATE;
	case 'H':
		show_help("0Announce/bbslist/day");
		if (HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_OBOARDS))
			show_help("0Announce/bbslist/day2");
		return FULLUPDATE;
	case 'k':
	case KEY_UP:
		if (last_line <= 0)
			break;
		
		if (cursor_pos(locmem, locmem->crs_line - 1, screen_len - 1))
			return PARTUPDATE;
		if (previewmode && uinfo.mode == READING)
			return PREUPDATE;
		break;
	case 'j':
	case KEY_DOWN:
		if (last_line <= 0)
			break;

		if (cursor_pos(locmem, locmem->crs_line + 1, 0))
			return PARTUPDATE;
		if (previewmode && uinfo.mode == READING)
			return PREUPDATE;
		break;
/*	case 'L':
		show_allmsgs();
		return FULLUPDATE;
*/
	case 'l':		//chenhao 解决在文章列表时看信的问题
		if (uinfo.mode == RMAIL)
			return DONOTHING;
		savemode = uinfo.mode;
		strlcpy(sdirect, currdirect, sizeof(sdirect));
		m_read();
		strlcpy(currdirect, sdirect, sizeof(currdirect));
		modify_user_mode(savemode);
		return MODECHANGED;
	case 'N':
	case Ctrl('F'):
	case KEY_PGDN:
	case ' ':
		if (last_line <= 0)
			break;

		if (last_line >= locmem->top_line + screen_len) {
			locmem->top_line += screen_len - 1;
			locmem->crs_line = locmem->top_line;
			return PARTUPDATE;
		}
		
		RMVCURS;
		locmem->crs_line = last_line;
		/* gcc: preview mode */
		if (previewmode && uinfo.mode == READING)
			return PREUPDATE;
		PUTCURS;
		break;
	case 'P':
	case Ctrl('B'):
	case KEY_PGUP:
		if (last_line <= 0)
			break;

		if (locmem->top_line > 1) {
			locmem->top_line -= screen_len - 1;
			if (locmem->top_line <= 0)
				locmem->top_line = 1;
			locmem->crs_line = locmem->top_line;
			return PARTUPDATE;
		} else {
			RMVCURS;
			locmem->crs_line = locmem->top_line;
			/* gcc: preview mode */
			if (previewmode && uinfo.mode == READING)
				return PREUPDATE;
			
			PUTCURS;
		}
		break;
	case KEY_HOME:
		if (last_line <= 0)
			break;

		locmem->top_line = 1;
		locmem->crs_line = 1;
		return PARTUPDATE;
	case '$':
	case KEY_END:
		if (last_line <= 0)
			break;

		if (last_line >= locmem->top_line + screen_len) {
			locmem->top_line = last_line - screen_len + 1;
			if (locmem->top_line <= 0)
				locmem->top_line = 1;
			locmem->crs_line = last_line;
			return PARTUPDATE;
		}
		RMVCURS;
		locmem->crs_line = last_line;
		/* gcc: preview mode */
		if (previewmode && uinfo.mode == READING)
			return PREUPDATE;
			
		PUTCURS;
		break;
	case 'S':		/* youzi */
		if (!HAS_PERM(PERM_MESSAGE))
			break;
		s_msg();
		return FULLUPDATE;
		break;
/*
	case 'f':
		if (!HAS_PERM(PERM_BASIC))
			break;
		t_friends();
		return FULLUPDATE;
		break;
*/
	case '!':		/* youzi leave */
		return Q_Goodbye();
		break;
		/* toggle post preview mode */
	case '\n':
	case '\r':
	case KEY_RIGHT:
		ch = 'r';
		/* lookup command table */
	default:
		if (ch == 'r' && last_line <= 0)
			break;

		for (i = 0; rcmdlist[i].fptr != NULL; i++) {
			if (rcmdlist[i].key == ch) {
				mode = (*(rcmdlist[i].fptr)) (locmem->crs_line, &pnt[(locmem->
							      crs_line - locmem->top_line) * ssize], currdirect);
				break;
			}
		}
	}
	return mode;
}

/* 参数说明：
 *
 *     	cmdmode			用户状态
 *     	direct			索引文件
 *     	init			初始化函数 (可选)
 *	cleanup			资源回收函数 (可选)
 *	dotitle			标题绘制函数
 *	doentry			项目绘制函数
 *	doendline		状态条绘制函数
 *	rcmdlist		命令列表
 *	getrecords		获取可显示项目
 *	getrecordnum		获取可显示项目数
 *	ssize			项目结构大小
 */

#define MAX_IREAD_DEPTH		32

void
i_read(int cmdmode, char *direct, void init(), void cleanup(),
       void (*dotitle)(), char *(*doentry)(int, void *), void (*doendline)(), struct one_key *rcmdlist,
       int getrecords(char *, void *, int, int, int), int getrecordnum(char *, int), int ssize)
{
	extern int friendflag;
	struct keeploc *locmem;
	char lbuf[11], desc[5], buf[40];
	char pnt[(t_lines - 4) * ssize];
	int lbc, mode, ch, num;
	int entries = 0, recbase = 0;
	static int cnt;

	/* monster: call i_read recursively may exhaust stack space */
	if (cnt == MAX_IREAD_DEPTH - 1)
		return;

	cnt++;
	if (init != NULL)
		init();

	screen_len = t_lines - 4; 
	memset(pnt, 0, screen_len * ssize);

	modify_user_mode(cmdmode);
	strcpy(currdirect, direct);
	draw_title(dotitle);
	last_line = getrecordnum(currdirect, ssize);

	if (last_line == 0) { /* 0 个项目 */
		switch (cmdmode) {
		case RMAIL:
			outs("没有任何新信件...");
			pressreturn();
			clear();
			break;
		case GMENU:
			switch (friendflag) {
			case 1:
				strcpy(desc, "好友");
				break;
			case 0:
				strcpy(desc, "坏人");
				break;
			case 2:
				strcpy(desc, "拒收");
				break;
			}
			snprintf(buf, sizeof(buf), "没有任何%s (A)新增%s (Q)离开？[Q] ", desc, desc);
			getdata(t_lines - 1, 0, buf, genbuf, 4, DOECHO, YEA);
			if (genbuf[0] == 'a' || genbuf[0] == 'A') {
				switch (friendflag) {
				case 1:
					friend_add(0, NULL, NULL);
					break;
				case 0:
					reject_add(0, NULL, NULL);
					break;
				case 2:
					maildeny_add(0, NULL, NULL);
					break;
				}
			}
			break;
		case READING:
			getdata(t_lines - 1, 0, "看版新成立 (P)发表文章 (Q)离开？[Q] ", genbuf, 4, DOECHO, YEA);
			if (genbuf[0] == 'p' || genbuf[0] == 'P')
				do_post();
			if ((last_line = getrecordnum(currdirect, ssize)) > 0) {
				draw_title(dotitle);
				goto start;
			}
			break;
		case DIGEST:
			move(3, 0);
			clrtobot();
			outs(">     << 目前没有文章 >>\n");
			move(3, 0);
			locmem = getkeep(currdirect, 1, 1);
			doendline();
			goto process;
		};

		if (cleanup != NULL) cleanup();
		cnt--;
		return;
	}

start:

	/* gcc: post previewmode */
	if (cmdmode == READING && previewmode) {
	        screen_len =  (t_lines - 4) / 2;
	}
	
	num = last_line - screen_len + 2;

	/* monster: 阅读精华区是光标定位在第一项 (nilky's suggestion) */
	if (cmdmode == DIGEST) {
		locmem = getkeep(currdirect, 1, last_line);
		modify_locmem(locmem, 1);
	} else {
		locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
		modify_locmem(locmem, last_line);
	}
	
	recbase = locmem->top_line;
	entries = getrecords(currdirect, pnt, ssize, recbase, screen_len);
	draw_entry(doentry, doendline, locmem, pnt, entries, ssize);
	PUTCURS;

process:
	lbc = 0;
	mode = DONOTHING;
	while ((ch = egetch()) != EOF) {
		if (talkrequest) {
			talkreply();
			mode = FULLUPDATE;
		} else if ((ch >= '0' && ch <= '9') ||
			   ((Ctrl('H') == ch || '\177' == ch) && lbc > 0)) {

			/* monster: 版面文章跳转提示 */
			if (Ctrl('H') == ch || '\177' == ch)
				lbuf[lbc--] = 0;
			else if (lbc < 9)
				lbuf[lbc++] = ch;
			lbuf[lbc] = 0;

			if (cmdmode == READING || cmdmode == RMAIL) {
				if (lbc == 0) {
					doendline();
				} else if (DEFINE(DEF_ENDLINE)) {
					int allstay;
					char buf[256];

					allstay = (time(NULL) - login_start_time) / 60;
					move(t_lines - 1, 0);
					clrtoeol();
					snprintf(buf, sizeof(buf), "[\033[36m%.12s\033[33m]", currentuser.userid);
					prints("\033[1;44;33m[\033[36m  跳转到第 %9s %s  \033[33m][\033[36m%4d\033[33m人/\033[1;36m%3d\033[33m友][\033[36m%1s%1s%1s%1s%1s%1s\033[33m]帐号%-24s[\033[36m%3d\033[33m:\033[36m%2d\033[33m]\033[m",
						     lbuf, (cmdmode == RMAIL) ? "封信件" : "篇文章",
						count_users, count_friends,
						     (uinfo.pager & ALL_PAGER) ? "P" : "p",
						     (uinfo.pager & FRIEND_PAGER) ? "O" : "o",
						     (uinfo.pager & ALLMSG_PAGER) ? "M" : "m",
						     (uinfo.pager & FRIENDMSG_PAGER) ? "F" : "f",
						     (DEFINE(DEF_MSGGETKEY)) ? "X" : "x",
						     (uinfo.invisible == 1) ? "C" : "c", buf,
						     (allstay / 60) % 1000, allstay % 60);
				}
			} else if (cmdmode == DIGEST) {		/* Pudding: 精华区按数字跳转时的显示 */
				if (lbc == 0) {
					doendline();
				} else if (DEFINE(DEF_ENDLINE)) {
					move(t_lines - 1, 0);
					clrtoeol();
					prints("跳转到第 %s 项", lbuf);
				}
			}
		} else if (lbc > 0 && (ch == '\n' || ch == '\r')) { /* 跳转 */
			doendline();
			lbuf[lbc] = '\0';
			lbc = atoi(lbuf);
			if (cursor_pos(locmem, lbc, 10))
				mode = PARTUPDATE;
			/* gcc: 同一页跳转需更新预览 */
			else if (previewmode && cmdmode == READING)
				mode = PREUPDATE;
			lbc = 0;
				
		} else {
			if (lbc != 0) {
				doendline();
				lbc = 0;
			}
			mode = i_read_key(rcmdlist, locmem, pnt, ch, ssize);

			while (mode == READ_NEXT || mode == READ_PREV) {
				int reload;

				reload = move_cursor_line(locmem, mode);
				if (reload == -1) {
					mode = FULLUPDATE;
					break;
				} else if (reload) {
					recbase = locmem->top_line;
					entries = getrecords(currdirect, pnt, ssize, recbase, screen_len);
					if (entries <= 0) {
						last_line = -1;
						break;
					}
				}
				num = locmem->crs_line - locmem->top_line;
				mode = i_read_key(rcmdlist, locmem, pnt, ch, ssize);
			}
			modify_user_mode(cmdmode);
		}
		if (mode == DOQUIT)
			break;
		if (mode == GOTO_NEXT) {
			cursor_pos(locmem, locmem->crs_line + 1, 1);
			mode = PARTUPDATE;
		}
		switch (mode) {
		case NEWDIRECT:
		case NEWDIRECT2:
		case DIRCHANGED:
		case MODECHANGED:
			recbase = -1;
			if (mode == MODECHANGED) {
//				pnt = ptr;
				mode = NEWDIRECT;
			}
			last_line = getrecordnum(currdirect, ssize);
			if (last_line == 0 && digestmode > 0) {
				struct fileheader *fileinfo =
					(struct fileheader *) &pnt[(locmem->crs_line - locmem->top_line) * ssize];
				acction_mode(locmem->crs_line, fileinfo, currdirect);
			}
			if (mode == NEWDIRECT) {
				num = last_line - screen_len + 1;
				locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
			}
			if (mode == NEWDIRECT2) {
				sscanf(genbuf, "%d", &num);
				locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
				cursor_pos(locmem, num, 1);
			}
		case FULLUPDATE:
			entries = getrecords(currdirect, pnt, ssize, recbase, screen_len);
			draw_title(dotitle);
		case PARTUPDATE:
			if (last_line < locmem->top_line + screen_len) {
				num = getrecordnum(currdirect, ssize);
				if (last_line != num) {
					last_line = num;
					recbase = -1;
				}
			}
			if (last_line == 0) {
				entries = 0;
				break;
			} else if (recbase != locmem->top_line) {
				recbase = locmem->top_line;
				if (recbase > last_line) {
					recbase = last_line - screen_len / 2;
					if (recbase < 1)
						recbase = 1;
					locmem->top_line = recbase;
				}
				entries = getrecords(currdirect, pnt, ssize, recbase, screen_len);
			}
			if (locmem->crs_line > last_line)
				locmem->crs_line = last_line;
						
			/* gcc: for post preview */
			if (uinfo.mode == READING && previewmode) {
				screen_len =  (t_lines - 4) / 2;
			}
			if (previewmode && locmem->crs_line >= locmem->top_line + screen_len) {
				locmem->top_line = locmem->crs_line;
				recbase = locmem->top_line;
				entries = getrecords(currdirect, pnt, ssize, recbase, screen_len);
				
			}
			
			draw_entry(doentry, doendline, locmem, pnt, entries, ssize);
			PUTCURS;
			break;
		case PREUPDATE:
			post_preview(locmem, pnt, ssize);
			doendline();
			PUTCURS;
			break;
		default:
			break;
		}
		mode = DONOTHING;
		if (entries == 0) {
			if (cmdmode != DIGEST) {
				if (cmdmode == READING && digestmode != NA)
					acction_mode(0, NULL, NULL);
				break;
			} else {
				move(3, 0);
				clrtobot();
				outs(">     << 目前没有文章 >>\n");
				move(3, 0);
				memset(pnt, 0, ssize);
				doendline();
			}
		}
	}
	clear();
	if (cleanup != NULL) cleanup();
	cnt--;
}

int
show_author(int ent, struct fileheader *fileinfo, char *direct)
{
	int oldmode;

	if (strchr(fileinfo->owner, '.'))
		return DONOTHING;

	oldmode = uinfo.mode;
	// modify_user_mode(QUERY);
	t_query(fileinfo->owner);
	modify_user_mode(oldmode);

	return FULLUPDATE;
}

int
SR_read(int ent, struct fileheader *fileinfo, char *direct)
{
	sread(YEA, NA, ent, fileinfo);
	return FULLUPDATE;
}

int
SR_author(int ent, struct fileheader *fileinfo, char *direct)
{
	sread(YEA, YEA, ent, fileinfo);
	return FULLUPDATE;
}

int
locate_article(char *direct, struct fileheader *header, int ent, int flag, void *arg)
{
	int fd, inc, oldent = ent, match = NA, eid = -1, len = 0;
	struct fileheader *start, *current, *end;
	char currpath[PATH_MAX + 1], filename[PATH_MAX + 1], *ptr;
	struct stat st;

	if ((fd = open(direct, O_RDONLY, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0 || st.st_size <= 0) {
		close(fd);
		return -1;
	}

	if ((start = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0)) == MAP_FAILED) {
		close(fd);
		return -1;
	}

	current = start + ent - 1;
	end = start + st.st_size / sizeof(struct fileheader);
	close(fd);

	inc = (flag & LOCATE_FIRST || flag & LOCATE_PREV) ? -1 : 1;

	if (flag & LOCATE_AUTHOR)
		len = strlen((char *)arg);

	TRY
		eid = ent + inc;
		current += inc;

		while (current >= start && current < end) {
			if (flag & LOCATE_NEW && !brc_unread(current->filetime))
				goto moveptr;

			if (flag & LOCATE_THREAD) {
				if (inc == -1) {
					if (current->filetime <= 0) {
						--eid;
						--current;
						continue;
					}
					if (current->filetime < *(int *)arg - MAX_POSTRETRY) {
						BREAK;
						munmap(start, st.st_size);
						return ent;
					}
				}

				match = (current->id == *(int *)arg) ? YEA : NA;
			} else if (flag & LOCATE_AUTHOR) {
				match = (!strncasecmp(current->owner, (char *)arg, len)) ? YEA : NA;
			} else if (flag & LOCATE_TITLE) {
				match = (strstr2(current->title, (char *)arg) != NULL) ? YEA : NA;
			} else if (flag & LOCATE_TEXT) {
				strlcpy(currpath, direct, sizeof(currpath));
				if ((ptr = strrchr(currpath, '/')) != NULL) {
					*ptr = '\0';
				} else {
					munmap(start, st.st_size);
					return -1;
				}
				snprintf(filename, sizeof(filename), "%s/%s", currpath, current->filename);
				match = searchpattern(filename, (char *)arg);
			} else {
				match = NA;
			}

			if (match == YEA) {
				if (flag & LOCATE_PREV || flag & LOCATE_NEXT) {
					if (header != NULL)
						memcpy(header, current, sizeof(struct fileheader));
					BREAK;
					munmap(start, st.st_size);
					return eid;
				}
				ent = eid;
			}

moveptr:
			eid += inc;
			current += inc;
		}
	END

	munmap(start, st.st_size);
	return (ent == oldent) ? -1 : ent;
}

int
SR_first(int ent, struct fileheader *fileinfo, char *direct)
{
	int result;

	if ((result = locate_article(direct, NULL, ent, LOCATE_THREAD | LOCATE_FIRST, &fileinfo->id)) == -1)
		return DONOTHING;

	setkeep(direct, result);
	return PARTUPDATE;
}

int
SR_first_new(int ent, struct fileheader *fileinfo, char *direct)
{
	int result;
	struct fileheader header;

	if (brc_unread(fileinfo->filetime)) {
		sread(YEA, NA, ent, fileinfo);
	} else {
		if ((result = locate_article(direct, &header, ent, LOCATE_THREAD | LOCATE_NEW | LOCATE_NEXT, &fileinfo->id)) == -1)
			return DONOTHING;
		sread(YEA, NA, ent, &header);
	}

	return FULLUPDATE;
}

int
SR_last(int ent, struct fileheader *fileinfo, char *direct)
{
	int result;

	if ((result = locate_article(direct, NULL, ent, LOCATE_THREAD | LOCATE_LAST, &fileinfo->id)) == -1)
		return DONOTHING;

	setkeep(direct, result);
	return PARTUPDATE;
}

/* freestyler: 跳到re文处 */
int
jump_to_reply(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[512], buf2[512];
	char currpath[PATH_MAX + 1], filepath[PATH_MAX + 1];
	char author[IDLEN + 1];
	char *t, *p;
	FILE *inf;
	int fd,	result = ent, eid = -1, len = 0;
	struct fileheader *start, *current, *end;
	struct stat st;
	
	strlcpy(currpath, direct, sizeof(currpath));
	if ((t = strrchr(currpath, '/')) != NULL)
		*t = '\0';
	else
		return DONOTHING;
	
	snprintf(filepath, sizeof(filepath), "%s/%s", currpath, fileinfo->filename);
	inf = fopen(filepath, "r");

	if (inf == NULL)
		return DONOTHING;
	
	/* get the author */
	while (fgets(buf, 256, inf) != NULL) {
		if ((t = strstr(buf, "【 在 ")) == NULL ||
		    strstr(buf, "的大作中提到: 】") == NULL)
			continue;
		if ((p = strstr(t + 6, " (")) == NULL) {
			fclose(inf);
			return DONOTHING;
		}
		*p = '\0';
		strlcpy(author, t + 6, sizeof(author));
		len = strlen(author);
		break;
	}
	/* 大作中第一行用来判断 */
	if (fgets(buf, 256, inf) == NULL) {
		fclose(inf);
		return DONOTHING;
	}
	fclose(inf);
	
	p = buf;
	if (!strncmp(buf, ": ", 2))
		p += 2;
	
	if ((fd = open(direct, O_RDONLY, 0)) == -1)
		return DONOTHING;
	
	if (fstat(fd, &st) < 0 || st.st_size <= 0) {
		close(fd);
		return DONOTHING;
	}

	if ((start = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0)) == MAP_FAILED) {
		close(fd);
		return DONOTHING;
	}

	current = start + ent - 2;
	eid = ent - 1;
	end = start + st.st_size / sizeof(struct fileheader);
	close(fd);

	
	TRY

		for (; current >= start; --eid, --current) {
                        /* LOCATE_THREAD */
			if (current->filetime <= 0) 
				continue;
			
			if (current->filetime < (fileinfo->id - MAX_POSTRETRY)) 
				break;
			
			if (current->id != fileinfo->id)
				continue;
			
			/* LOCATE_AUTHOR */
			if (strncasecmp(current->owner, author, len))
				continue;
			
			result = eid;		/* temp result */
			
			/* LOCATE_TEXT */
			snprintf(filepath, sizeof(filepath), "%s/%s", currpath, current->filename);
			
			if ((inf = fopen(filepath,  "r")) == NULL)
				break;
			
			/* 跳过头4行文章信息 */
			while (fgets(buf2, 256, inf) != NULL)
				if (buf2[0] == '\n')
					break;

			/* 正文部分 */
			if (fgets(buf2, 256, inf) != NULL)  {
				if (!strncmp(p, buf2, strlen(buf) - 2)) {
					result = eid;
					fclose(inf);
					break;
				}
			}
			fclose(inf);
		}		

	END

	munmap(start, st.st_size);
	
	setkeep(direct, result);
	return PARTUPDATE;
}

int
thread_search_up(int ent, struct fileheader *fileinfo, char *direct)
{
	int result;

	if ((result = locate_article(direct, NULL, ent, LOCATE_THREAD | LOCATE_PREV, &fileinfo->id)) == -1)
		return DONOTHING;

	setkeep(direct, result);
	return PARTUPDATE;
}

int
thread_search_down(int ent, struct fileheader *fileinfo, char *direct)
{
	int result;

	if ((result = locate_article(direct, NULL, ent, LOCATE_THREAD | LOCATE_NEXT, &fileinfo->id)) == -1)
		return DONOTHING;

	setkeep(direct, result);
	return PARTUPDATE;
}

int
searchpattern(char *filename, char *query)
{
	int fd;
	struct stat st;
	char *buf;

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return NA;

	if (fstat(fd, &st) == -1) {
		close(fd);
		return NA;
	}

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);
	if (buf == MAP_FAILED || st.st_size <= 0) {
		return NA;
	}

	if (strstr2n(buf, query, st.st_size) != NULL) {
		munmap(buf, st.st_size);
		return YEA;
	}

	munmap(buf, st.st_size);
	return NA;
}

int
search_author(char *direct, int ent, int flag, char *currauthor)
{
	static char author[IDLEN + 2];
	char ans[IDLEN + 2], pmt[STRLEN];
	int result;

	strlcpy(author, currauthor, sizeof(author));
	snprintf(pmt, sizeof(pmt), "%s的文章搜寻作者 [%s]: ", (flag == LOCATE_NEXT) ? "往后来" : "往先前", author);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, pmt, ans, IDLEN + 1, DOECHO, YEA);

	if (ans[0] != '\0')
		strlcpy(author, ans, sizeof(author));

	if ((result = locate_article(direct, NULL, ent, LOCATE_AUTHOR | flag, author)) != -1)
		setkeep(direct, result);

	return PARTUPDATE;
}

int
auth_search_down(int ent, struct fileheader *fileinfo, char *direct)
{
	return search_author(direct, ent, LOCATE_NEXT, fileinfo->owner);
}

int
auth_search_up(int ent, struct fileheader *fileinfo, char *direct)
{
	return search_author(direct, ent, LOCATE_PREV, fileinfo->owner);
}

int
search_title(char *direct, int ent, int flag)
{
	static char title[TITLELEN];
	char ans[TITLELEN], pmt[STRLEN];
	int result;

	snprintf(pmt, sizeof(pmt), "%s搜寻标题 [%.16s]: ", (flag == LOCATE_NEXT) ? "往后" : "往前", title);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, pmt, ans, sizeof(title), DOECHO, YEA);
	if (*ans != '\0')
		strlcpy(title, ans, sizeof(title));

	if ((result = locate_article(direct, NULL, ent, LOCATE_TITLE | flag, title)) != -1)
		setkeep(direct, result);

	return PARTUPDATE;
}

int
title_search_down(int ent, struct fileheader *fileinfo, char *direct)
{
	return search_title(direct, ent, LOCATE_NEXT);
}

int
title_search_up(int ent, struct fileheader *fileinfo, char *direct)
{
	return search_title(direct, ent, LOCATE_PREV);
}

int
search_post(char *direct, int ent, int flag)
{
	static char query[50];
	char ans[50], pmt[STRLEN];
	int result;

	snprintf(pmt, sizeof(pmt), "搜寻%s的文章 [%.16s]: ", (flag == LOCATE_NEXT) ? "往后" : "往前", query);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, pmt, ans, sizeof(ans), DOECHO, YEA);
	if (ans[0] != '\0')
		strlcpy(query, ans, sizeof(query));

	if ((result = locate_article(direct, NULL, ent, LOCATE_TEXT | flag, query)) != -1)
		setkeep(direct, result);

	return PARTUPDATE;
}

int
post_search_down(int ent, struct fileheader *fileinfo, char *direct)
{
	return search_post(direct, ent, LOCATE_NEXT);
}

int
post_search_up(int ent, struct fileheader *fileinfo, char *direct)
{
	return search_post(direct, ent, LOCATE_PREV);
}

int
sread(int readfirst, int auser, int ent, struct fileheader *fileinfo)
{
	int flag = LOCATE_NEXT, movecursor = NA;
	char filename[PATH_MAX + 1];
	struct boardheader *bp;
	struct fileheader header;

	if (fileinfo->filename[0] == '\0')
		return DONOTHING;

	memcpy(&header, fileinfo, sizeof(header));
	if (readfirst == NA) {
		clear_line(t_lines - 1);
		prints("\033[1;44;31m[%8s] \033[33m下一封 <Space>,<Enter>,↓│上一封 ↑,U                              \033[m",
			auser ? "相同作者" : "主题阅读");
		switch (egetch()) {
		case ' ':
		case '\n':
			flag = LOCATE_NEXT;
			movecursor = NA;
			break;
		case KEY_DOWN:
			flag = LOCATE_NEXT;
			movecursor = YEA;
			break;
		case 'n':
		case 'N':
		case Ctrl('N'):
			flag = LOCATE_NEXT | LOCATE_NEW;
			movecursor = YEA;
			break;
		case 'u':
		case 'U':
			flag = LOCATE_PREV;
			movecursor = NA;
			break;
		case KEY_UP:
			flag = LOCATE_PREV;
			movecursor = YEA;
			break;
		default:
			return FULLUPDATE;
		}

		if (auser == YEA) {
			if ((ent = locate_article(currdirect, &header, ent, flag | LOCATE_AUTHOR, header.owner)) == -1)
				return FULLUPDATE;
		} else {
			if ((ent = locate_article(currdirect, &header, ent, flag | LOCATE_THREAD, &header.id)) == -1)
				return FULLUPDATE;
		}
	}

	while (1) {
		if (INMAIL(uinfo.mode)) {
			setmailfile(filename, header.filename);
		} else {
			setboardfile(filename, currboard, header.filename);
		}

		setrid(header.id);
		if (movecursor == YEA) setkeep(currdirect, ent);
		setquotefile(filename);
		
		if (!INMAIL(uinfo.mode)) {
			char article_link[1024];
			snprintf(article_link, sizeof(article_link), 	/* 全文链接 */
				 "http://%s/bbscon?board=%s&file=%s",
				BBSHOST, currboard, fileinfo->filename);
			if (getattachinfo(&header)) 
				ansimore4(filename, attach_info, attach_link, article_link, NA);
			else
				ansimore4(filename, NULL, NULL, article_link, NA);
		} else {
			ansimore(filename, NA);
		}
		
		brc_addlist(header.filetime);

		clear_line(t_lines - 1);
		prints("\033[1;44;31m[%8s] \033[33m回信 R │ 结束 Q,← │下一封 ↓,Enter│上一封 ↑,U │ ^R 回给作者   \033[m",
			auser ? "相同作者" : "主题阅读");

		switch (egetch()) {
		case 'Y':
		case 'R':
		case 'y':
		case 'r':
			bp = getbcache(currboard);
			noreply = (header.flag & FILE_NOREPLY) || (bp->flag & NOREPLY_FLAG);

			if (!noreply || HAS_PERM(PERM_SYSOP) || current_bm || isowner(&currentuser, &header)) {
				local_article = (header.flag & FILE_OUTPOST) ? NA : YEA;
				do_reply(header.title, header.owner, header.id);
			} else {
				clear();
				move(5, 6);
				prints("对不起, 该文章有不可 RE 属性, 你不能回复(RE) 这篇文章.");
				pressreturn();
			}
			break;
		case ' ':
		case '\n':
			flag = LOCATE_NEXT;
			movecursor = NA;
			break;
		case KEY_DOWN:
			flag = LOCATE_NEXT;
			movecursor = YEA;
			break;
		case 'n':
		case 'N':
		case Ctrl('N'):
			flag = LOCATE_NEXT | LOCATE_NEW;
			movecursor = YEA;
			break;
		case 'u':
		case 'U':
			flag = LOCATE_PREV;
			movecursor = NA;
			break;
		case KEY_UP:
			flag = LOCATE_PREV;
			movecursor = YEA;
			break;
		case Ctrl('A'):
			clear();
			show_author(0, &header, currdirect);
			flag = LOCATE_NEXT;
			break;
		case Ctrl('R'):
			post_reply(0, &header, NULL);
			break;
		case Ctrl('X'):
			auser = NA;
			break;
		case Ctrl('U'):
			auser = YEA;
			break;
		default:
			return FULLUPDATE;
		}

		if (auser == YEA) {
			if ((ent = locate_article(currdirect, &header, ent, flag | LOCATE_AUTHOR, header.owner)) == -1)
				break;
		} else {
			if ((ent = locate_article(currdirect, &header, ent, flag | LOCATE_THREAD, &header.id)) == -1)
				break;
		}
	}

	return FULLUPDATE;
}
