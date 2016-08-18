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
#include "edit.h"

#ifdef MARK_X_FLAG
extern int markXflag;
#endif

struct textline *firstline = NULL;
struct textline *lastline = NULL;

void vedit_key(int ch);
struct textline *currline = NULL;
int first_mark_line;
int currpnt = 0;
extern int local_article;
extern unsigned int posts_article_id;

char searchtext[80];
int editansi = 0;
int scrollen = 2;
int marknum;
int moveln = 0;
int shifttmp = 0;
int ismsgline;
int tmpline;

#define	       a_lines	(t_lines - 2)

struct textline *top_of_win = NULL;
int curr_window_line, currln;
int redraw_everything;
int insert_character = 1;

#define CLEAR_MARK()  mark_on = 0; mark_begin = mark_end = NULL;
struct textline *mark_begin, *mark_end;
int mark_on;
int savecursor;

void
msgline(void)
{
	int tmpansi;
	time_t now;

	if (ismsgline <= 0)
		return;
	now = time(NULL);
	tmpansi = showansi;
	showansi = YEA;

	if (talkrequest) {
		talkreply();
		clear();
		showansi = NA;
		display_buffer();
		showansi = YEA;
	}
	clear_line(t_lines - 1);
	prints("\033[1;33;44m%s是否有新信件 \033[31mCtrl-Q\033[33m 求救 "
		"状态 [\033[32m%s\033[33m][\033[32m%4.4d\033[33m,\033[32m%3.3d\033[33m][\033[32m%s\033[33m] 时间"
		"\033[1;33;44m【\033[1;32m%.16s\033[33m】\033[m",
		chkmail(YEA) ? "【\033[5;32m⊙\033[m\033[1;33;44m】" : "【  】",
		insert_character ? "Ins" : "Rep", currln + 1, currpnt + 1,
		enabledbchar ? "X" : " ", ctime(&now));
	showansi = tmpansi;
}

static void
msg(int signo)
{
	int x, y;
	int tmpansi;

	tmpansi = showansi;
	showansi = 1;
	getyx(&y, &x);
	msgline();

	signal(SIGALRM, msg);
	move(y, x);
	refresh();
	alarm(60);
	showansi = tmpansi;
	return;
}

void
indigestion(int i)
{
	report("SERIOUS INTERNAL INDIGESTION CLASS %d\n", i);
}

struct textline *
forward_line(struct textline *pos, int num)
{
	moveln = 0;
	while (num-- > 0)
		if (pos && pos->next) {
			pos = pos->next;
			moveln++;
		}
	return pos;
}

int
forward_line2(struct textline *pos, int num)
{
	struct textline *temp;

	moveln = 0;
	temp = pos;
	while (num-- > 0) {
		if (temp && temp->next) {
			temp = temp->next;
			moveln++;
		}
	}

	if (num > 0) {
		moveln = 0;
		return 0;
	}

	pos = temp;
	return 1;
}

struct textline *
back_line(struct textline *pos, int num)
{
	moveln = 0;
	while (num-- > 0)
		if (pos && pos->prev) {
			pos = pos->prev;
			moveln++;
		}

	return pos;
}

void
countline(void)
{
	struct textline *pos;

	pos = firstline;
	moveln = 0;
	while (pos != lastline)
		if (pos->next) {
			pos = pos->next;
			moveln++;
		}
}

int
getlineno(void)
{
	int cnt = 0;
	struct textline *p = currline;

	while (p != top_of_win) {
		if (p == NULL)
			break;
		cnt++;
		p = p->prev;
	}
	return cnt;
}

char *
killsp(char *s)
{
	while (*s == ' ')
		s++;
	return s;
}

struct textline *
alloc_line(void)
{
	struct textline *p;

	p = (struct textline *) malloc(sizeof (*p));
	if (p == NULL) {
		indigestion(13);
		abort_bbs();
	}
	p->next = NULL;
	p->prev = NULL;
	p->data[0] = '\0';
	p->len = 0;
	p->attr = 0;		/* for copy/paste */
	return p;
}

/*
  Appends p after line in list.  keeps up with last line as well.
 */

void
goline(int n)
{
	struct textline *p = firstline;
	int count;

	if (n < 0)
		n = 1;
	if (n == 0)
		return;
	for (count = 1; count < n; count++) {
		if (p) {
			p = p->next;
			continue;
		} else
			break;
	}
	if (p) {
		currln = n - 1;
		curr_window_line = 0;
		top_of_win = p;
		currline = p;
	} else {
		top_of_win = lastline;
		currln = count - 2;
		curr_window_line = 0;
		currline = lastline;
	}
	if (Origin(currline)) {
		currline = currline->prev;
		top_of_win = currline;
		curr_window_line = 0;
		currln--;
	}
	if (Origin(currline->prev)) {
		currline = currline->prev->prev;
		top_of_win = currline;
		curr_window_line = 0;
		currln -= 2;
	}

}

void
go(void)
{
	char tmp[8];
	int line;

	signal(SIGALRM, SIG_IGN);
	getdata(t_lines - 1, 0, "请问要跳到第几行: ", tmp, 7, DOECHO, YEA);
	msg(SIGALRM);
	if (tmp[0] == '\0')
		return;
	line = atoi(tmp);
	goline(line);
	return;
}

void
searchline(char *text)
{
	int tmpline;
	int addr;
	int tt;

	struct textline *p = currline;
	int count = 0;

	tmpline = currln;
	for (;; p = p->next) {
		count++;
		if (p) {
			if (count == 1)
				tt = currpnt;
			else
				tt = 0;
			if (strstr(p->data + tt, text)) {
				addr =
				    (int) (strstr(p->data + tt, text) -
					   p->data) + strlen(text);
				currpnt = addr;
				break;
			}
		} else
			break;
	}
	if (p) {
		currln = currln + count - 1;
		curr_window_line = 0;
		top_of_win = p;
		currline = p;
	} else {
		goline(currln + 1);
	}
	if (Origin(currline)) {
		currline = currline->prev;
		top_of_win = currline;
		curr_window_line = 0;
		currln--;
	}
	if (Origin(currline->prev)) {
		currline = currline->prev->prev;
		top_of_win = currline;
		curr_window_line = 0;
		currln -= 2;
	}

}

void
search(void)
{
	char tmp[STRLEN];

	signal(SIGALRM, SIG_IGN);
	getdata(t_lines - 1, 0, "搜寻字串: ", tmp, 65, DOECHO, YEA);
	msg(SIGALRM);
	if (tmp[0] == '\0')
		return;
	else
		strcpy(searchtext, tmp);

	searchline(searchtext);
	return;
}

void
append(struct textline *p, struct textline *line)
{
	p->next = line->next;
	if (line->next)
		line->next->prev = p;
	else
		lastline = p;
	line->next = p;
	p->prev = line;
}

/*
  delete_line deletes 'line' from the list and maintains the lastline, and
  firstline pointers.
 */

#define ADJUST_MARK(p, q) if(p == q) p = (q->next) ? q->next : q->prev

void
delete_line(struct textline *line)
{
	/* if single line */
	if (!line->next && !line->prev) {
		line->data[0] = '\0';
		line->len = 0;
		CLEAR_MARK();
		return;
	}

	ADJUST_MARK(mark_begin, line);
	ADJUST_MARK(mark_end, line);

	if (line->next)
		line->next->prev = line->prev;
	else
		lastline = line->prev;	/* if on last line */

	if (line->prev)
		line->prev->next = line->next;
	else
		firstline = line->next;	/* if on first line */

	if (line)
		free(line);
}

/*
  split splits 'line' right before the character pos
 */

void
split(struct textline *line, int pos)
{
	struct textline *p = alloc_line();

	if (pos > line->len) {
		free(p);
		return;
	}

	p->len = line->len - pos;
	line->len = pos;
	strcpy(p->data, (line->data + pos));
	p->attr = line->attr;	/* for copy/paste */
	*(line->data + pos) = '\0';
	append(p, line);

	if (line == currline && pos <= currpnt) {
		currline = p;
		currpnt -= pos;
		curr_window_line++;
		currln++;
	}

	redraw_everything = YEA;
}

/*
  join connects 'line' and the next line.  It returns true if:

  1) lines were joined and one was deleted
  2) lines could not be joined
  3) next line is empty

  returns false if:

  1) Some of the joined line wrapped
 */

int
join(struct textline *line)
{
	int ovfl;

	if (!line->next)
		return YEA;
	/*if(*killsp(line->next->data) == '\0')
	   return YEA ; */
	ovfl = line->len + line->next->len - WRAPMARGIN;
	if (ovfl < 0) {
		strcat(line->data, line->next->data);
		line->len += line->next->len;
		delete_line(line->next);
		return YEA;
	} else {
		char *s;
		struct textline *p = line->next;

		s = p->data + p->len - ovfl - 1;
		while (s != p->data && *s == ' ')
			s--;
		while (s != p->data && *s != ' ')
			s--;
		if (s == p->data)
			return YEA;
		split(p, (s - p->data) + 1);
		if (line->len + p->len >= WRAPMARGIN) {
			indigestion(0);
			return YEA;
		}
		join(line);
		p = line->next;
		if (p->len >= 1 && p->len + 1 < WRAPMARGIN) {
			if (p->data[p->len - 1] != ' ') {
				strcat(p->data, " ");
				p->len++;
			}
		}
		return NA;
	}
}

void
insert_char(int ch)
{
	char *s;
	struct textline *p = currline;
	int i, wordwrap = YEA;
	int ich, lln;

	if (currpnt > p->len) {
		indigestion(1);
		return;
	}
	if (currpnt < p->len && !insert_character) {
		p->data[currpnt++] = ch;
	} else {
		for (i = p->len; i >= currpnt; i--)
			p->data[i + 1] = p->data[i];
		p->data[currpnt] = ch;
		p->len++;
		currpnt++;
	}
	if (p->len < WRAPMARGIN)
		return;
	s = p->data + (p->len - 1);
	while (s != p->data && *s == ' ')
		s--;
	while (s != p->data && *s != ' ')
		s--;
	if (s == p->data) {
		wordwrap = NA;
		s = p->data + (p->len - 2);
	}

	/* Leeward 98.07.28 */
	if ((*s) & 0x80) {       /* 避免在汉字中间折行 */
		for (ich = 0, lln = s - p->data + 1; lln > 0; lln--) {
			if (!(p->data[lln - 1] & 0x80)) {
				    break;
			} else {
				    ich++;
			}
		}
		if (ich % 2) s--;
	}

	split(p, (s - p->data) + 1);
	p = p->next;
	if (wordwrap && p->len >= 1) {
		i = p->len;
		if (p->data[i - 1] != ' ') {
			p->data[i] = ' ';
			p->data[i + 1] = '\0';
			p->len++;
		}
		{
		}
	}

/*	while (!join(p)) {
 *		p = p->next;
 *		if (p == NULL) {
 *			indigestion(2);
 *			break;
 *		}
 *	}
*/

	if (Origin(currline)) {
		currline = p->prev;
		curr_window_line--;
		currln--;
	}
}

void
ve_insert_str(char *str)
{
	while (*str)
		insert_char(*(str++));
}

void
delete_char(void)
{
	int i, dbchar = 0;

	if (currline->len == 0)
		return;
	if (currpnt >= currline->len) {
		indigestion(1);
		return;
	}
	if (enabledbchar && (currline->data[currpnt] & 0x80)) {
		for (i = 0; i < currpnt; i++)
			if (dbchar)
				dbchar = 0;
			else if (currline->data[i] & 0x80)
				dbchar = 1;
		/* 这里是吴如宏于1999年元月8日修改过 */
		if (currpnt < currline->len - 1) {
			if (currline->data[currpnt + 1] & 0x80) {
				/*  这样可以确保删除的是一个整字 */
				if (dbchar) {
					for (i = currpnt - 1; i != currline->len - 1; i++)
						currline->data[i] = currline->data[i + 2];
					currpnt--;
				} else {
					for (i = currpnt; i != currline->len - 1; i++)
						currline->data[i] = currline->data[i + 2];
				}
				currline->len -= 2;
			} else {	/* 当前位为高位，后续位为低位 */
				if (dbchar) {	/* 当前是整字 */
					for (i = currpnt - 1; i != currline->len - 1; i++)
						currline->data[i] = currline->data[i + 2];
					currpnt--;
					currline->len -= 2;
				} else {	/* 当前为非整字，只删单字节 */
					for (i = currpnt; i != currline->len; i++)
						currline->data[i] = currline->data[i + 1];
					currline->len--;
				}
			}
			return;
		}
		if (dbchar && currpnt == currline->len - 1) {
			for (i = currpnt - 1; i != currline->len - 1; i++)
				currline->data[i] = currline->data[i + 2];
			currline->len -= 2;
			currpnt--;
		} else {
			for (i = currpnt; i != currline->len; i++)
				currline->data[i] = currline->data[i + 1];
			currline->len--;
		}
	} else {
		for (i = currpnt; i != currline->len; i++)
			currline->data[i] = currline->data[i + 1];
		currline->len--;
	}
}

void
vedit_init(void)
{
	struct textline *p = alloc_line();

	first_mark_line = 0;
	firstline = p;
	lastline = p;
	currline = p;
	currpnt = 0;
	marknum = 0;

	process_ESC_action('M', '0');
	top_of_win = p;
	curr_window_line = 0;
	currln = 0;
	redraw_everything = NA;

	CLEAR_MARK();
}

void
insert_to_fp(FILE *fp)
{
	int ansi = 0;
	struct textline *p;

	for (p = firstline; p; p = p->next)
		if (p->data[0]) {
			fprintf(fp, "%s\n", p->data);
			if (strchr(p->data, '\033'))
				ansi++;
		}
	if (ansi)
		fprintf(fp, "%s\n", ANSI_RESET);
}

int
read_file(char *filename)
{
	int fd;
	char ch, *buf, *buf1;
	struct stat st;

	if (currline == NULL)
		vedit_init();

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0 || st.st_size < 0) {
		close(fd);
		return -1;
	}

	if (st.st_size == 0) {
		close(fd);
		return 0;
	}

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);
	if (buf == MAP_FAILED) {
		return -1;
	}

	TRY
		buf1 = buf + st.st_size;
		while (buf < buf1) {
			ch = *buf;

			if (isprint2((int)ch) || ch == 27) {
				if (currpnt < 254)
					insert_char(ch);
				else if (currpnt < 255)
					insert_char('.');
				} else if (ch == Ctrl('I')) {
					do {
						insert_char(' ');
				} while (currpnt & 0x7);
			} else if (ch == '\n') {
				split(currline, currpnt);
			}
			++buf;
		}
	END

	munmap(buf, st.st_size);
	return 0;
}

int
if_exist_id(unsigned int id)
{
	static struct mypostlog my_posts;
	char buf1[256];
	int n;
	FILE *fp;

	sethomefile(buf1, currentuser.userid, "my_posts");
	if ((fp = fopen(buf1, "r+")) == NULL) {
		if ((fp = fopen(buf1, "w+")) == NULL)
			return 0;
	}
	fread(&my_posts, sizeof(my_posts), 1, fp);
	for (n = 0; n < 64; n++) {
		if (my_posts.id[n] == id) {
			fclose(fp);
			return 1;
		}
	}
	my_posts.hash_ip = (my_posts.hash_ip + 1) & 63;
	my_posts.id[my_posts.hash_ip] = id;
	fseek(fp, 0, SEEK_SET);
	fwrite(&my_posts, sizeof (my_posts), 1, fp);
	fclose(fp);
	return 0;
}

void
write_posts(void)
{
	struct postlog log;

	if (normalboard(currboard) == NA)
		return;

	strcpy(log.board, currboard);
	log.id = posts_article_id;
	log.date = time(NULL);
	log.number = 1;

	
	if (!if_exist_id(log.id))
		append_record(".post", &log, sizeof(log));
	append_record(".post2", &log, sizeof(log));
}

void
write_header(FILE *fp, int mode, int inmail)
{
	int noname;
	struct boardheader *bp;
	char uid[IDLEN + 2];
	char uname[NICKNAMELEN + 1];
	time_t now;

	now = time(NULL);
	strlcpy(uid, currentuser.userid, sizeof(uid));
	if (inmail)
#if defined(MAIL_REALNAMES)
		strlcpy(uname, currentuser.realname, sizeof(uname));
#else
		strlcpy(uname, currentuser.username, sizeof(uname));
#endif
	else
#if defined(POSTS_REALNAMES)
		strlcpy(uname, currentuser.realname, sizeof(uname));
#else
		strlcpy(uname, currentuser.username, sizeof(uname));
#endif
	my_ansi_filter(save_title);
	bp = getbcache(currboard);
	noname = (bp != NULL && (bp->flag & ANONY_FLAG) && header.chk_anony);
	if (inmail) {
		fprintf(fp, "寄信人: %s (%s)\n", uid, uname);
	} else {
		if (mode == 0 && !noname)
			write_posts();

		fprintf(fp, "发信人: %s (%s), 信区: %s\n",
			noname ? currboard : uid,
			noname ? "我是匿名天使" : uname, currboard);
	}
	fprintf(fp, "标  题: %s\n", save_title);
	fprintf(fp, "发信站: %s (%24.24s)", BoardName, ctime(&now));
	if (inmail) {
		fprintf(fp, "\n来  源: %s \n\n", fromhost);
	} else {
		fprintf(fp, ", %s\n\n", (local_article) ? "站内信件" : "转信");
	}
}

void
addsignature(FILE *fp, int blank)
{
	FILE *sigfile;
	int i, valid_ln = 0;
	char tmpsig[MAXSIGLINES][256];
	char inbuf[256];
	char fname[STRLEN];

	if (fp == NULL)
		return;

	if (blank) {
		fputs("\n--\n", fp);
	} else {
		fputs("--\n", fp);
	}

	setuserfile(fname, "signatures");
	if (currentuser.signature == 0 || (sigfile = fopen(fname, "r")) == NULL)
		return;

	for (i = 0; i < (currentuser.signature - 1) * MAXSIGLINES && currentuser.signature != 1; i++) {
		if (fgets(inbuf, sizeof(inbuf), sigfile) == NULL) {
			fclose(sigfile);
			return;
		}
	}

	for (i = 1; i <= MAXSIGLINES; i++) {
		if (fgets(inbuf, sizeof (inbuf), sigfile) == NULL)
			break;

		if (inbuf[0] != '\n')
			valid_ln = i;
		strcpy(tmpsig[i - 1], inbuf);
	}
	fclose(sigfile);

	for (i = 0; i < valid_ln; i++)
		fputs(tmpsig[i], fp);
}

#define KEEP_EDITING -2

void
valid_article(char *pmt, char *abort, char *choice, int sure)
{
	struct textline *p = firstline;
	char ch;
	int total, lines, len, sig, y, w = NA;

	if (uinfo.mode == POSTING) {
		total = lines = len = sig = 0;
		while (p != NULL) {
			if (!sig) {
				ch = p->data[0];
				if (strcmp(p->data, "--") == 0)
					sig = 1;
				else if (ch != ':' && ch != '>' && ch != '=' &&
					 !strstr(p->data, "的大作中提到: 】")) {
					lines++;
					len += strlen(p->data);
				}
			}
			total++;
			p = p->next;
		}
		y = 2;

		if (YEA == sure) {
#ifdef MARK_X_FLAG
			if (len <= 40 || lines <= 1)
				markXflag = 1;
			else
				markXflag = 0;
#endif
			return;
		}

		if (lines < total * 0.35 - MAXSIGLINES) {
			move(y, 0);
			prints("注意：本篇文章的引言过长, 建议您删掉一些不必要的引言.\n");
			y += 3;
		}

		if (len <= 40 || lines <= 1) {
			move(y, 0);
			prints("注意：本篇文章过于简短, 系统认为是灌水文章.\n");
			y += 3;
#ifdef MARK_X_FLAG
			markXflag = 1;
#endif
		}
#ifdef MARK_X_FLAG
		else
			markXflag = 0;
#endif

		if (w) {
			strcpy(pmt, "(E)再编辑, (F)换行发出，(S)转信, (L)不转信, (A)取消 or (T)更改标题? [E]: ");
			choice = "efslatEFSLAT";
		}
	}

	if (YEA == sure) {
		abort[0] = 's';
		return;
	}

	do {
		getdata(0, 0, pmt, abort, 2, DOECHO, YEA);
	} while (abort[0] != '\0' && strchr(choice, abort[0]) == NULL);

	if (w && abort[0] == '\0')
		abort[0] = 'E';
	switch (abort[0]) {
	case 'A':
	case 'a':		/* abort */
	case 'E':
	case 'e':		/* keep editing */
		return;
	}

}

/* monster: Add By KCN for auto crlf, enhanced by me */
void
autolfline(char *line, FILE *fp)
{
	char *p1, *p2;
	int dbchar = 0;
	int n = 0;
	int curlinelen = 0;
	int inESC = 0;

	p2 = p1 = line;
	while (*p2 != 0) {
		switch (inESC) {
		case 0:
/*data mode*/
			if (*p2 == KEY_ESC)
				inESC = 1;
			else {
				curlinelen++;
				if (curlinelen > 73) {
					if (dbchar) {
						fwrite(p1, 1, p2 - p1 - 1, fp);
						p1 = p2 - 1;
					} else {
						/* monster: a better word split algorithm for tailing english word */
						if (isalpha(*p2)) {
							if (*(p2 + 1) == ' ') {
								fwrite(p1, 1, p2 - p1 + 1, fp);
								p1 = p2 + 2;
							} else {
								int i = 1, j;

								j = (curlinelen > 10) ? 10 : curlinelen -  1;
								while (i < j) {
									if (*(p2 - i) == ' ') {
										fwrite(p1, 1,  p2 -  p1 - i, fp);
										p1 = p2 - i + 1;
										break;
									}
									i++;
								}
								if (i == j) {
									fwrite(p1, 1, p2 - p1, fp);
									p1 = p2;
								}
							}
						} else {
							fwrite(p1, 1, p2 - p1, fp);
							if (*p2 == ' ')
								p1 = p2 + 1;
							else
								p1 = p2;
						}
					}
					fprintf(fp, "\n");
					curlinelen = 0;
				}
				if (dbchar)
					dbchar = 0;
				else if (*p2 & 0x80)
					dbchar = 1;
			}
			break;
		case 1:
/*ESC mode            */
			if (*p2 == '[')
				inESC = 2;
			else {
				inESC = 0;
/*Error ESC format.Need to correct curlinelen;    				*/
			}
			n = 0;
			dbchar = 0;
			break;
		case 2:
/* '*[' begin */
			if (*p2 == ';') {
				n = 0;
				break;
			}
			if (isdigit(*p2)) {
				n = n * 10 + (int) *p2 - '0';
				break;
			}
			switch (*p2) {
			case 'C':
				curlinelen += n;
				break;
			case 'D':
				curlinelen -= n;
				break;
			case 'H':
			case 'f':
				if (n == 0)
					n = 1;
				curlinelen = n - 1;
				break;
			case 's':
				savecursor = curlinelen;
				break;
			case 'u':
				if (savecursor != -1)
				curlinelen = savecursor;
				break;
			};
			inESC = 0;
		}
		p2++;
	}
	fprintf(fp, "%s\n", p1);
}

int
write_file(char *filename, int flag, int sure)
{
	struct textline *p = firstline;
	FILE *fp;
	char abort[6];
	int autolf = 0;

	signal(SIGALRM, SIG_IGN);
	clear();

	if (sure) {
		autolf = 0;
		abort[0] = 'S';
		valid_article("", abort, "", sure);
	} else {
		if (uinfo.mode == POSTING) {
			if (local_article == YEA) {
				valid_article("(L)不转信, (F)换行发出，(S)转信, (A)取消, (T)更改标题 or (E)再编辑? [L]: ",
					      abort, "lfsateLFSATE", sure);
			} else {
				valid_article("(S)转信, (F)换行发出，(L)不转信, (A)取消, (T)更改标题 or (E)再编辑? [S]: ",
					      abort, "sflateSFLATE", sure);
			}
		} else if (uinfo.mode == SMAIL) {
			valid_article("(S)寄出, (F)换行发出，(A)取消, or (E)再编辑? [S]: ", abort,
				      "sfaeSFAE", sure);
		} else {
			valid_article("(S)储存档案, (F)换行发出，(A)放弃编辑, (E)继续编辑? [S]: ", abort,
				      "sfaeSFAE", sure);
		}
	}

	if (abort[0] == 'F' || abort[0] == 'f') {
		abort[0] = 's';
		autolf = 1;
	}

	if (abort[0] == 'a' || abort[0] == 'A') {
		struct stat stbuf;

		clear();
		prints("取消...\n");
		refresh();
		sleep(1);
		if (stat(filename, &stbuf) || stbuf.st_size == 0)
			unlink(filename);

		/* free up memory */
		while (p != NULL) {
			struct textline *v = p->next;

			free(p);
			p = v;
		}
		currline = NULL;
		lastline = NULL;
			firstline = NULL;

		return -1;
	} else if (abort[0] == 'e' || abort[0] == 'E') {
		msg(SIGALRM);
		return KEEP_EDITING;
	} else if (abort[0] == 't' || abort[0] == 'T') {
		char buf[STRLEN]; 

		move(1, 0);
		prints("旧标题: %s", save_title);
		strlcpy(buf, save_title, sizeof(buf));
		int temp_showansi = showansi;
		showansi = YEA;
		getdata(2, 0, "新标题: ", buf, 50, DOECHO, NA);
		showansi = temp_showansi;
		my_ansi_filter(buf);
		if (strcmp(save_title, buf) && strlen(buf) != 0) {
			local_article = NA;
			strlcpy(save_title, buf, sizeof(save_title));
		}
	} else if (abort[0] == 's' || abort[0] == 'S') {
		local_article = NA;
	} else if (abort[0] == 'l' || abort[0] == 'L')
		local_article = YEA;

	#ifdef FILTER
	if ((uinfo.mode == POSTING || uinfo.mode == EDIT) && check_text() == 0) {
		/* todo:记录发贴人 */
		char filter_buf[STRLEN];
		char save_buf[STRLEN];
		struct textline *line = firstline;
		char	report_filename[80]; /* freestyler: 应该写在另外一个文件 */

		strlcpy(save_buf,save_title,sizeof(save_buf));
		snprintf(filter_buf, sizeof(filter_buf), "[filter][%s]%s", currboard, save_title);
		snprintf(report_filename, sizeof(report_filename), "%s.filter", filename);
		if ((fp = fopen(report_filename, "w")) != NULL) {
			write_header(fp, 0, INMAIL(uinfo.mode));
			while (line != NULL) {
				if (line->next != NULL || line->data[0] != '\0') {
					if (autolf)
						autolfline(line->data, fp);
					else
						fprintf(fp, "%s\n", line->data);
				}
				line = line->next;
			}
			
			fprintf(fp,  "\n\033[m\033[1;%2dm※ 来源:．%s %s．[FROM: %s]\033[m\n",
				(currentuser.numlogins % 7) + 31, BoardName, BBSHOST,
				fromhost);

			fclose(fp);
		}
		postfile(report_filename, "Arbitrate", filter_buf, 2);
		unlink(report_filename);
		strlcpy(save_title,save_buf,sizeof(save_title));
		clear();
		move(12, 20);
		prints("很抱歉，您的文章里含有不适合的内容，请重新编辑.");
		refresh();
		sleep(1);
		msg(SIGALRM);
		return KEEP_EDITING;
	}
	#endif

	if ((fp = fopen(filename, "w")) == NULL) {
		clear();
		move(12, 20);
		prints("很抱歉，系统暂时无法保存文章内容，请重新编辑.");
		pressanykey();
		msg(SIGALRM);
		return KEEP_EDITING;
	}

	if (flag & EDIT_SAVEHEADER) {
		write_header(fp, 0, INMAIL(uinfo.mode));
	}

	savecursor = -1;
	while (p != NULL) {
		struct textline *v = p->next;

		if (p->next != NULL || p->data[0] != '\0') {
			if (autolf)
				autolfline(p->data, fp);
			else
				fprintf(fp, "%s\n", p->data);
		}
		free(p);
		p = v;
	}

	if (flag & EDIT_ADDLOGINFO) {		// 加入文章来源
		struct boardheader *bp = getbcache(currboard);
                int anony = (bp != NULL && (bp->flag & ANONY_FLAG) && (header.chk_anony));

		fprintf(fp,  "\n\033[m\033[1;%2dm※ 来源:．%s %s．[FROM: %s]\033[m\n",
		        anony ? (time(NULL) + random()) % 7 + 31 : currentuser.numlogins % 7 + 31,
                        BoardName, BBSHOST,
			anony ? "匿名天使的家" : fromhost);
	}

	fclose(fp);
	currline = NULL;
	lastline = NULL;
	firstline = NULL;

	if (abort[0] == 'l' || abort[0] == 'L' || local_article == YEA) {
		local_article = NA;
	}

	return 1;
}

void
keep_fail_post(void)
{
	char filename[PATH_MAX + 1];
	struct textline *p = firstline;
	FILE *fp;

	snprintf(filename, sizeof(filename), "home/%c/%s/%s.deadve",
		mytoupper(currentuser.userid[0]), currentuser.userid,
		currentuser.userid);
	if ((fp = fopen(filename, "w")) == NULL) {
		indigestion(5);
		return;
	}

	while (p != NULL) {
		struct textline *v = p->next;

		if (p->next != NULL || p->data[0] != '\0')
			fprintf(fp, "%s\n", p->data);
		free(p);
		p = v;
	}
	return;
}

void
strnput(char *str)
{
	int count = 0;

	while ((*str != '\0') && (++count < STRLEN)) {
		if (*str == KEY_ESC) {
			outc('*');
			str++;
			continue;
		}
		outc(*str++);
	}
}

void
cstrnput(char *str)
{
	int count = 0;

	outs(ANSI_REVERSE);
	while ((*str != '\0') && (++count < STRLEN)) {
		if (*str == KEY_ESC) {
			outc('*');
			str++;
			continue;
		}
		outc(*str++);
	}
	while (++count < STRLEN)
		outc(' ');
	clrtoeol();
	outs(ANSI_RESET);
}

/*Function Add by SmallPig*/
int
Origin(struct textline *text)
{
	char tmp[STRLEN];

	if (uinfo.mode != EDIT)
		return 0;
	if (!text)
		return 0;
	snprintf(tmp, sizeof(tmp), ":．%s %s．[FROM:", BoardName, BBSHOST);
	if (strstr(text->data, tmp) && *text->data != ':')
		return 1;
	else
		return 0;
}

void
display_buffer(void)
{
	struct textline *p;
	int i, shift, temp_showansi;

	temp_showansi = showansi;

	for (p = top_of_win, i = 0; i < t_lines - 1; i++) {
		move(i, 0);
		if (p) {
			shift = (currpnt + 2 > STRLEN) ? (currpnt / (STRLEN - scrollen)) * (STRLEN - scrollen) : 0;
			if (editansi) {
				showansi = 1;
				outs(p->data);
			} else if ((p->attr & M_MARK)) {
				showansi = 1;
				clear_line(i);
				cstrnput(p->data + shift);
			} else {
				if (p->len >= shift) {
					showansi = 0;
					strnput(p->data + shift);
				} else
					clrtoeol();
			}
			p = p->next;
		} else
			prints("%s~", editansi ? ANSI_RESET : "");
		clrtoeol();
	}

	showansi = temp_showansi;
	msgline();
	return;
}

#define WHICH_ACTION_COLOR  "(M)区块处理 (I/E)读取/写入剪贴簿 (C)使用彩色 (F/B/R)前景/背景/还原色彩"
#define WHICH_ACTION_MONO   "(M)区块处理 (I/E)读取/写入剪贴簿 (C)使用单色 (F/B/R)前景/背景/还原色彩"

#define CHOOSE_MARK     "(0)取消标记 (1)设定开头 (2)设定结尾 (3)复制标记内容 "
#define FROM_WHICH_PAGE "读取剪贴簿第几页? (0-7) [预设为 0]"
#define SAVE_ALL_TO     "把整篇文章写入剪贴簿第几页? (0-7) [预设为 0]"
#define SAVE_PART_TO    "把标记块写入剪贴簿第几页? (0-7) [预设为 0]"
#define FROM_WHICH_SIG  "取出签名簿第几页? (0-7) [预设为 0]"
#define CHOOSE_FG       "前景颜色? 0)黑 1)红 2)绿 3)黄 4)深蓝 5)粉红 6)浅蓝 7)白 "
#define CHOOSE_BG       "背景颜色? 0)黑 1)红 2)绿 3)黄 4)深蓝 5)粉红 6)浅蓝 7)白 "
#define CHOOSE_BIG5     "内码表? 0)标点 1)符号 2)表格 3)注音 4)平假 5)片假 6)希腊 7)数字"
#define CHOOSE_ERROR    "选项错误"

int
vedit_process_ESC(int arg)	/* ESC + x */
{
	int ch2, action, old_showansi;

	switch (arg) {
	case 'M':
	case 'm':
		ch2 = ask(CHOOSE_MARK);
		action = 'M';
		break;
	case 'I':
	case 'i':		/* import */
		ch2 = ask(FROM_WHICH_PAGE);
		action = 'I';
		break;
	case 'E':
	case 'e':		/* export */
		ch2 = ask(mark_on ? SAVE_PART_TO : SAVE_ALL_TO);
		action = 'E';
		break;
	case 'S':
	case 's':		/* signature */
		ch2 = '0';
		action = 'S';
		break;
	case 'F':
	case 'f':
		ch2 = ask(CHOOSE_FG);
		action = 'F';
		break;
	case 'B':
	case 'b':
		ch2 = ask(CHOOSE_BG);
		action = 'B';
		break;
	case 'R':
	case 'r':
		ch2 = '0';	/* not used */
		action = 'R';
		break;
	case 'D':
	case 'd':
		ch2 = '4';
		action = 'M';
		break;
	case 'N':
	case 'n':
		ch2 = '0';
		action = 'N';
		break;
	case 'G':
	case 'g':
		ch2 = '1';
		action = 'G';
		break;
	case 'L':
	case 'l':
		ch2 = '0';	/* not used */
		action = 'L';
		break;
	case 'C':
	case 'c':
		ch2 = '0';	/* not used */
		action = 'C';
		break;
	case 'Q':
	case 'q':
		ch2 = '0';	/* not used */
		action = 'M';
		break;
		marknum = 0;
		break;
	case 'V':		/* monster: 快速锁屏 */
	case 'v':
		old_showansi = showansi;
		showansi = 1;
		x_lockscreen_silent();
		showansi = old_showansi;
	default:
		return 0;	//yy add 0
	}

	if (strchr("IES", action) && (ch2 == '\n' || ch2 == '\r'))
		ch2 = '0';

	if (ch2 >= '0' && ch2 <= '7')
		return process_ESC_action(action, ch2);
	else {
		return ask(CHOOSE_ERROR);
	}
}

int
mark_block(void)
{
	struct textline *p;
	int pass_mark = 0;

	first_mark_line = 0;
	if (mark_begin == NULL && mark_end == NULL)
		return 0;
	if (mark_begin == mark_end) {
		mark_begin->attr |= M_MARK;
		return 1;
	}
	if (mark_begin == NULL || mark_end == NULL) {
		if (mark_begin != NULL)
			mark_begin->attr |= M_MARK;
		else
			mark_end->attr |= M_MARK;
		return 1;
	} else {
		for (p = firstline; p != NULL; p = p->next) {
			if (p == mark_begin || p == mark_end) {
				pass_mark++;
				p->attr |= M_MARK;
				continue;
			}
			if (pass_mark == 1)
				p->attr |= M_MARK;
			else {
				first_mark_line++;
				p->attr &= ~(M_MARK);
			}
			if (pass_mark == 2)
				first_mark_line--;
		}
		return 1;
	}
}

void
process_MARK_action(int arg, char *msg)
				/* operation of MARK */
				  /* message to return */
{
	struct textline *p;
	int dele_1line;

	switch (arg) {
	case '0':		/* cancel */
		for (p = firstline; p != NULL; p = p->next)
			p->attr &= ~(M_MARK);
		CLEAR_MARK();
		break;
	case '1':		/* mark begin */
		mark_begin = currline;
		mark_on = mark_block();
		if (mark_on)
			strcpy(msg, "标记已设定完成");
		else
			strcpy(msg, "已设定开头标记, 尚无结尾标记");
		break;
	case '2':		/* mark end */
		mark_end = currline;
		mark_on = mark_block();
		if (mark_on)
			strcpy(msg, "标记已设定完成");
		else
			strcpy(msg, "已设定结尾标记, 尚无开头标记");
		break;
	case '3':		/* copy mark */
		if (mark_on && !(currline->attr & M_MARK)) {
			for (p = firstline; p != NULL; p = p->next) {
				if (p->attr & M_MARK) {
					ve_insert_str(p->data);
					split(currline, currpnt);
				}
			}
		} else
			bell();
		strcpy(msg, "标记复制完成");
		break;
	case '4':		/* delete mark */
		dele_1line = 0;
		if (mark_on) {	/*&&(currline->attr & M_MARK)) { */
			if (currline == firstline)
				dele_1line = 1;
			else
				dele_1line = 2;
			for (p = firstline; p != NULL;) {	//这里开始修改
				if (p->attr & M_MARK) {
					currline = p;
					vedit_key(Ctrl('Y'));
					p = currline;
					if ((p == firstline) &&
					    (!(p->next) ||
					     (Origin(currline->next)))) {
						if (p->attr & M_MARK)
							vedit_key(Ctrl('Y'));
						p = p->next;
					}
				} else
					p = p->next;
			}	//断线bug,gd fixed.

			process_ESC_action('M', '0');
			marknum = 0;
			if (dele_1line == 0 || dele_1line == 2) {
				if (first_mark_line == 0)
					first_mark_line = 1;
				goline(first_mark_line);
			} else
				goline(1);
		}
		break;
	default:
		strcpy(msg, CHOOSE_ERROR);
	}
	strcpy(msg, "\0");
}

int
process_ESC_action(int action, int arg)
/* valid action are L/M/I/G/E/N/S/F/B/R/C */
/* valid arg are    '0' - '7' */
{
	int newch = 0;
	char msg[80] = { '\0' }, buf[80];
	char filename[PATH_MAX + 1];
	FILE *fp;

	switch (action) {
	case 'L':
		if (ismsgline >= 1) {
			ismsgline = 0;
			clear_line(t_lines - 1);
			refresh();
		} else {
			ismsgline = 1;
		}
		break;
	case 'M':
		process_MARK_action(arg, msg);
		break;
	case 'I':
		snprintf(filename, sizeof(filename), "home/%c/%s/clip_%c",
			mytoupper(currentuser.userid[0]), currentuser.userid, arg);
		if (read_file(filename) == -1) {
			snprintf(msg, sizeof(msg), "无法取出剪贴簿第 %c 页", arg);
		} else {
			snprintf(msg, sizeof(msg), "已取出剪贴簿第 %c 页", arg);
		}
		break;
	case 'G':
		go();
		redraw_everything = YEA;
		break;
	case 'E':
		snprintf(filename, sizeof(filename), "home/%c/%s/clip_%c",
			mytoupper(currentuser.userid[0]), currentuser.userid, arg);
		if ((fp = fopen(filename, "w")) != NULL) {
			if (mark_on) {
				struct textline *p;

				for (p = firstline; p != NULL; p = p->next)
					if (p->attr & M_MARK)
						fprintf(fp, "%s\n", p->data);
			} else {
				insert_to_fp(fp);
			}
			fclose(fp);
			snprintf(msg, sizeof(msg), "已贴至剪贴簿第 %c 页", arg);
		} else {
			snprintf(msg, sizeof(msg), "无法贴至剪贴簿第 %c 页", arg);
		}
		break;
	case 'N':
		searchline(searchtext);
		redraw_everything = YEA;
		break;
	case 'S':
		search();
		redraw_everything = YEA;
		break;
	case 'F':
		snprintf(buf, sizeof(buf), "%c[3%cm", 27, arg);
		ve_insert_str(buf);
		break;
	case 'B':
		snprintf(buf, sizeof(buf), "%c[4%cm", 27, arg);
		ve_insert_str(buf);
		break;
	case 'R':
		ve_insert_str(ANSI_RESET);
		break;
	case 'C':
		editansi = showansi = YEA;
		redraw_everything = YEA;
		clear();
		display_buffer();
		redoscr();
		strlcpy(msg, "已显示彩色编辑成果，即将切回单色模式", sizeof(msg));
		break;
	}

	if (strchr("FBRCM", action))
		redraw_everything = YEA;

	if (msg[0] != '\0') {
		if (action == 'C') {	/* need redraw */
			clear_line(t_lines - 2);
			prints("\033[1m%s%s%s\033[m", msg, ", 请按任意键返回编辑画面...", ANSI_RESET);
			igetkey();
			newch = '\0';
			editansi = showansi = 0;
			clear();
			display_buffer();
		} else {
			newch = ask(strcat(msg, "，请继续编辑。"));
		}
		return newch;
	}

	return '\0';
}

#define NO_ANSI_MODIFY  if(no_touch) { warn++; break; }

void
vedit_key(int ch)
{
	static int lastindent = -1;
	int i, no_touch, warn, shift;

	if (ch == Ctrl('P') || ch == KEY_UP || ch == Ctrl('N') ||
	    ch == KEY_DOWN) {
		if (lastindent == -1)
			lastindent = currpnt;
	} else
		lastindent = -1;

	no_touch = (editansi && strchr(currline->data, '\033')) ? 1 : 0;
	warn = 0;

	if (ch < 0x100 && isprint2(ch)) {
		if (no_touch)
			warn++;
		else
			insert_char(ch);
	} else
		switch (ch) {
		case Ctrl('I'):
			NO_ANSI_MODIFY;
			do {
				insert_char(' ');
			}
			while (currpnt & 0x7);
			break;
		case '\r':
		case '\n':
			NO_ANSI_MODIFY;
			split(currline, currpnt);
			break;
		case Ctrl('G'):	/* redraw screen */
			clear();
			redraw_everything = YEA;
			break;
		case Ctrl('R'):	/* toggle double char */
			enabledbchar = ~enabledbchar & 1;
			break;
		case Ctrl('Q'):	/* call help screen */
			show_help("help/edithelp");
			redraw_everything = YEA;
			break;
		case KEY_LEFT:	/* backward character */
			if (currpnt > 0) {
				currpnt--;
			} else if (currline->prev) {
				curr_window_line--;
				currln--;
				currline = currline->prev;
				currpnt = currline->len;
			}
			break;
		case Ctrl('C'):
			process_ESC_action('M', '3');
			break;
		case Ctrl('U'):
			if (marknum == 0) {
				marknum = 1;
				process_ESC_action('M', '1');
			} else
				process_ESC_action('M', '2');
			clear();
			break;
		case Ctrl('V'):
		case KEY_RIGHT:	/* forward character */
			if (currline->len != currpnt) {
				currpnt++;
			} else if (currline->next) {
				currpnt = 0;
				curr_window_line++;
				currln++;
				currline = currline->next;
				if (Origin(currline)) {
					curr_window_line--;
					currln--;
					currline = currline->prev;
				}
			}
			break;
		case Ctrl('P'):
		case KEY_UP:	/* Previous line */
			if (currline->prev) {
				currln--;
				curr_window_line--;
				currline = currline->prev;
				currpnt = (currline->len > lastindent) ? lastindent : currline->len;
			}
			break;
		case Ctrl('N'):
		case KEY_DOWN:	/* Next line */
			if (currline->next) {
				currline = currline->next;
				curr_window_line++;
				currln++;
				if (Origin(currline)) {
					currln--;
					curr_window_line--;
					currline = currline->prev;
				}
				currpnt = (currline->len > lastindent) ? lastindent : currline->len;
#ifdef OSF
				/* monster: a hack to solve scroll bug */
				if (curr_window_line > t_lines - 3)
					redoscr();
#endif
			}
			break;
		case Ctrl('B'):
		case KEY_PGUP:	/* previous page */
			top_of_win = back_line(top_of_win, a_lines);
			currline = back_line(currline, a_lines);
			currln -= moveln;
			curr_window_line = getlineno();
			if (currpnt > currline->len)
				currpnt = currline->len;
			redraw_everything = YEA;
			break;
		case Ctrl('F'):
		case KEY_PGDN:	/* next page */
			// top_of_win = forward_line(top_of_win, a_lines);
			if (forward_line2(top_of_win, a_lines) == 0)
				break;
			currline = forward_line(currline, a_lines);
			currln += moveln;
			curr_window_line = getlineno();
			if (currpnt > currline->len)
				currpnt = currline->len;
			if (Origin(currline->prev)) {
				currln -= 2;
				curr_window_line = 0;
				currline = currline->prev->prev;
				top_of_win = lastline->prev->prev;
			}
			if (Origin(currline)) {
				currln--;
				curr_window_line--;
				currline = currline->prev;
			}
			redraw_everything = YEA;
			break;
		case Ctrl('A'):
		case KEY_HOME:	/* begin of line */
			currpnt = 0;
			break;
		case Ctrl('E'):
		case KEY_END:	/* end of line */
			currpnt = currline->len;
			break;
		case Ctrl('S'):	/* start of file */
			top_of_win = firstline;
			currline = top_of_win;
			currpnt = 0;
			curr_window_line = 0;
			currln = 0;
			redraw_everything = YEA;
			break;
		case Ctrl('T'):	/* tail of file */
			top_of_win = back_line(lastline, a_lines);
			countline();
			currln = moveln;
			currline = lastline;
			curr_window_line = getlineno();
			currpnt = 0;
			if (Origin(currline->prev)) {
				currline = currline->prev->prev;
				currln -= 2;
				curr_window_line -= 2;
			}
			redraw_everything = YEA;
			break;
		case Ctrl('O'):
		case KEY_INS:	/* Toggle insert/overwrite */
			insert_character = !insert_character;
			/*move(0,73);
			   prints( " [%s] ", insert_character ? "Ins" : "Rep" ); */
			break;
		case Ctrl('H'):
		case '\177':	/* backspace */
			NO_ANSI_MODIFY;
			if (currpnt == 0) {
				struct textline *p;

				if (!currline->prev) {
					break;
				}
				currln--;
				curr_window_line--;
				currline = currline->prev;
				currpnt = currline->len;

				/* according to cityhunter */
				if (curr_window_line < 0) {
					top_of_win = currline;
					curr_window_line = 0;
				}

				if (*killsp(currline->next->data) == '\0') {
					delete_line(currline->next);
					redraw_everything = YEA;
					break;
				}
				p = currline;
				while (!join(p)) {
					p = p->next;
					if (p == NULL) {
						indigestion(2);
						abort_bbs();
					}
				}
				redraw_everything = YEA;
				break;
			}
			currpnt--;
			delete_char();
			break;
		case Ctrl('D'):
		case KEY_DEL:	/* delete current character */
			NO_ANSI_MODIFY;
			if (currline->len == currpnt) {
				struct textline *p = currline;

				if (!Origin(currline->next)) {
					while (!join(p)) {
						p = p->next;
						if (p == NULL) {
							indigestion(2);
							abort_bbs();
						}
					}
				} else if (currpnt == 0)
					vedit_key(Ctrl('K'));
				redraw_everything = YEA;
				break;
			}
			delete_char();
			break;
		case Ctrl('Y'):	/* delete current line */
			/* STONGLY coupled with Ctrl-K */
			no_touch = 0;	/* ANSI_MODIFY hack */
			currpnt = 0;
			if (currline->next) {
				if (Origin(currline->next) && !currline->prev) {
					currline->data[0] = '\0';
					currline->len = 0;
					break;
				}
			} else if (currline->prev != NULL) {
				currline->len = 0;
			} else {
				currline->len = 0;
				currline->data[0] = '\0';
				break;
			}
			currline->len = 0;
			vedit_key(Ctrl('K'));
			break;
		case Ctrl('K'):	/* delete to end of line */
			NO_ANSI_MODIFY;
			if (currline->prev == NULL && currline->next == NULL) {
				currline->data[0] = '\0';
				currpnt = 0;
				break;
			}
			if (currline->next) {
				if (Origin(currline->next) &&
				    currpnt == currline->len && currpnt != 0)
					break;
				if (Origin(currline->next) &&
				    currline->prev == NULL) {
					vedit_key(Ctrl('Y'));
					break;
				}
			}
			if (currline->len == 0) {
				struct textline *p = currline->next;

				if (!p) {
					p = currline->prev;
					if (!p) {
						break;
					}
					if (curr_window_line > 0)
						curr_window_line--;
					currln--;
				}
				if (currline == top_of_win)
					top_of_win = p;
				delete_line(currline);
				currline = p;
				if (Origin(currline)) {
					currline = currline->prev;
					curr_window_line--;
					currln--;
				}
				redraw_everything = YEA;
				break;
			}
			if (currline->len == currpnt) {
				struct textline *p = currline;

				while (!join(p)) {
					p = p->next;
					if (p == NULL) {
						indigestion(2);
						abort_bbs();
					}
				}
				redraw_everything = YEA;
				break;
			}
			currline->len = currpnt;
			currline->data[currpnt] = '\0';
			break;
		default:
			break;
		}

	if (curr_window_line < 0) {
		curr_window_line = 0;
		if (!top_of_win->prev) {
			indigestion(6);
		} else {
			top_of_win = top_of_win->prev;
/*            		redraw_everything = YEA ;
			    move(t_lines-2,0);
			    clrtoeol();
			    refresh(); */
			rscroll();
		}
	}
	if (curr_window_line >= t_lines - 1) {
		for (i = curr_window_line - t_lines + 1; i >= 0; i--) {
			curr_window_line--;
			if (!top_of_win->next) {
				indigestion(7);
			} else {
				top_of_win = top_of_win->next;
				/*          redraw_everything = YEA ;
				   move(t_lines-1,0);
				   clrtoeol();
				   refresh(); */
				scroll();
			}
		}
	}

	if (editansi /*|| mark_on */ )
		redraw_everything = YEA;
	shift = (currpnt + 2 > STRLEN) ?
	    (currpnt / (STRLEN - scrollen)) * (STRLEN - scrollen) : 0;
	msgline();
	if (shifttmp != shift || redraw_everything == YEA) {
		redraw_everything = YEA;
		shifttmp = shift;
	} else
		redraw_everything = NA;

	move(curr_window_line, 0);
	if (currline->attr & M_MARK) {
		showansi = 1;
		cstrnput(currline->data + shift);
		showansi = 0;
	} else
		strnput(currline->data + shift);
	clrtoeol();
}

int
raw_vedit(char *filename, int flag, int headlines)
{
	int i, newch, ch = 0, foo, shift;
	struct textline *st_tmp, *st_tmp2;

	if (read_file(filename) == -1 && headlines > 0) {
		flag |= EDIT_SAVEHEADER;
		headlines = 0;
	}

	/* 跳过头部信息 */
	for (i = 0, top_of_win = firstline; i < headlines; i++) {
		if (top_of_win->next == NULL || top_of_win->data[0] == '\n')
			break;
		top_of_win = top_of_win->next;
	}

	currline = top_of_win;
	st_tmp2 = firstline;
	st_tmp = currline->prev;	/* 保存链表指针，并修改编辑第一行的的指针 */
	currline->prev = NULL;
	firstline = currline;
	curr_window_line = 0;
	currln = 0;
	currpnt = 0;
	clear();
	display_buffer();
	msgline();
	while (ch != EOF) {
		newch = '\0';
		switch (ch) {
		case Ctrl('X'):
		case Ctrl('W'):
			if (headlines) {
				st_tmp->next = firstline;	/* 退出时候恢复原链表 */
				firstline->prev = st_tmp;
				firstline = st_tmp2;
			}
			foo = write_file(filename, flag, (ch == Ctrl('X')) ? YEA : NA);
			if (foo != KEEP_EDITING)
				return foo;
			if (headlines) {
				firstline = st_tmp->next;
				firstline->prev = NULL;
			}
			redraw_everything = YEA;
			break;
		case KEY_ESC:
			if (KEY_ESC_arg == KEY_ESC) {
				insert_char(KEY_ESC);
			} else {
				newch = vedit_process_ESC(KEY_ESC_arg);
				clear();
			}
			redraw_everything = YEA;
			break;
		default:
			vedit_key(ch);
		}
		if (redraw_everything) {
			display_buffer();
		}
		redraw_everything = NA;
		shift = (currpnt + 2 > STRLEN) ?
		    (currpnt / (STRLEN - scrollen)) * (STRLEN - scrollen) : 0;
		move(curr_window_line, currpnt - shift);

		ch = (newch != '\0') ? newch : igetkey();
	}
	return 1;
}

int
vedit(char *filename, int flag)
{
	int ans = 0, t;

	if (filename) {
		t = showansi;
		showansi = NA;
		init_alarm();
		ismsgline = (DEFINE(DEF_EDITMSG)) ? 1 : 0;
		msg(SIGALRM);
		if (flag & EDIT_MODIFYHEADER) {
			ans = raw_vedit(filename, flag, 0);	// 允许修改标题行
		} else {
			ans = raw_vedit(filename, flag, (INMAIL(uinfo.mode)) ? 5 : 4);
		}
		signal(SIGALRM, SIG_IGN);
		showansi = t;
	}
	return ans;
}
