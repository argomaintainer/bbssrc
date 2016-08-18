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

extern char clearbuf[];
extern char cleolbuf[];
extern char scrollrev[];
extern char strtstandout[];
extern char endstandout[];
extern int clearbuflen;
extern int cleolbuflen;
extern int scrollrevlen;
extern int strtstandoutlen;
extern int endstandoutlen;

extern int automargins;
extern int dumb_term;
extern int ibufsize;
extern int icurrchar;

#define o_clear()     output(clearbuf,clearbuflen)
#define o_cleol()     output(cleolbuf,cleolbuflen)
#define o_scrollrev() output(scrollrev,scrollrevlen)
#define o_standup()   output(strtstandout,strtstandoutlen)
#define o_standdown() output(endstandout,endstandoutlen)

unsigned char scr_lns, scr_cols;
unsigned char cur_ln = 0, cur_col = 0;
int roll, scrollcnt;
unsigned char docls;
unsigned char downfrom;
int standing = NA;
int inansi = NA;
int disable_move = NA;
int offsetln = 0;
struct screenline *big_picture = NULL;

int
num_ans_chr(char *str)
{
	int ansinum = 0, ansi = NA;
	char *ptr;

	for (ptr = str; *ptr; ptr++) {
		if (*ptr == KEY_ESC) {
			ansi = YEA;
			ansinum++;
			continue;
		}

		if (ansi) {
			if ((*ptr < '0' || *ptr > '9') && *ptr != '[' && *ptr != ';' && *ptr != ' ')
				ansi = NA;
			ansinum++;
		}
	}
	return ansinum;
}

void
init_screen(int slns, int scols)
{
	struct screenline *oslp = big_picture;
	int lns;
	
	lns = (scr_lns < slns) ? scr_lns : slns;
	scr_lns = slns;
	scr_cols = (scols > LINELEN) ? LINELEN : scols;
	big_picture = (struct screenline *)calloc(scr_lns, sizeof(struct screenline));

	// retain old screen content
	if (oslp != NULL) {
		memcpy(big_picture, oslp, lns * sizeof(struct screenline));
		free(oslp);
		redoscr();
	}

	docls = YEA;
	downfrom = 0;
	roll = 0;
}

void
initscr(void)
{
	if (!dumb_term && !big_picture)
		t_columns = WRAPMARGIN;
	init_screen(t_lines, WRAPMARGIN);
}

int tc_col, tc_line;

void
rel_move(int was_col, int was_ln, int new_col, int new_ln)
{
	extern char *BC;

	if (new_ln >= t_lines || new_col >= t_columns)
		return;
	tc_col = new_col;
	tc_line = new_ln;
	if ((new_col == 0) && (new_ln == was_ln + 1)) {
		ochar('\n');
		if (was_col != 0)
			ochar('\r');
		return;
	}
	if ((new_col == 0) && (new_ln == was_ln)) {
		if (was_col != 0)
			ochar('\r');
		return;
	}
	if (was_col == new_col && was_ln == new_ln)
		return;
	if (new_col == was_col - 1 && new_ln == was_ln) {
		if (BC)
			tputs(BC, 1, ochar);
		else
			ochar(Ctrl('H'));
		return;
	}
	do_move(new_col, new_ln, ochar);
}

void
standoutput(char *buf, int ds, int de, int sso, int eso)
{
	int st_start, st_end;

	if (eso <= ds || sso >= de) {
		output(buf + ds, de - ds);
		return;
	}
	st_start = (sso > ds) ? sso : ds;
	st_end = (eso > de) ? de : eso;
	if (sso > ds)
		output(buf + ds, sso - ds);
	o_standup();
	output(buf + st_start, st_end - st_start);
	o_standdown();
	if (de > eso)
		output(buf + eso, de - eso);
}

void
redoscr(void)
{
	int i, j;
	struct screenline *bp = big_picture;

	if (dumb_term)
		return;
	o_clear();
	tc_col = 0;
	tc_line = 0;
	for (i = 0; i < scr_lns; i++) {
		j = i + roll;
		while (j >= scr_lns)
			j -= scr_lns;
		if (bp[j].len == 0)
			continue;
		rel_move(tc_col, tc_line, 0, i);
		if (bp[j].mode & STANDOUT)
			standoutput((char*)bp[j].data, 0, (int)bp[j].len, (int)bp[j].sso,
				    (int)bp[j].eso);
		else
			output((char*)bp[j].data, (int)bp[j].len);
		tc_col += bp[j].len;
		if (tc_col >= t_columns) {
			if (!automargins) {
				tc_col -= t_columns;
				tc_line++;
				if (tc_line >= t_lines)
					tc_line = t_lines - 1;
			} else
				tc_col = t_columns - 1;
		}
		bp[j].mode &= ~(MODIFIED);
		bp[j].oldlen = bp[j].len;
	}
	rel_move(tc_col, tc_line, cur_col, cur_ln);
	docls = NA;
	scrollcnt = 0;
	oflush();
}

void
refresh(void)
{
	int i, j;
	struct screenline *bp = big_picture;
	extern int automargins;

//      extern int scrollrevlen;

	if (icurrchar > ibufsize)
		return;
	if ((docls) || (abs(scrollcnt) >= (scr_lns - 3))) {
		redoscr();
		return;
	}
#ifdef OSF
	if (scrollcnt > 0) {
		rel_move(tc_col, tc_line, 0, t_lines - 1);
		while (scrollcnt > 0) {
			ochar('\n');
			scrollcnt--;
		}
	}
#else
	if (scrollcnt > 0) {
		do_move(0, 1024, ochar);
		while (scrollcnt > 0) {
			ochar('\n');
			scrollcnt--;
		}
		do_move(0, t_lines - 2, ochar);
	}
#endif

	if (scrollcnt < 0) {
		char buf[10];

		rel_move(tc_col, tc_line, 0, 0);
		sprintf(buf, "\033[%dL", -scrollcnt);
		output(buf, strlen(buf));
		scrollcnt = 0;
	}

/*
	if (scrollcnt < 0) {
		if (!scrollrevlen) {
			redoscr();
			return;
		}
		rel_move(tc_col, tc_line, 0, 0);
		while (scrollcnt < 0) {
			o_scrollrev();
			scrollcnt++;
		}
	}
*/

	for (i = 0; i < scr_lns; i++) {
		j = i + roll;
		while (j >= scr_lns)
			j -= scr_lns;
		if (bp[j].mode & MODIFIED && bp[j].smod < bp[j].len) {
			bp[j].mode &= ~(MODIFIED);
			if (bp[j].emod >= bp[j].len)
				bp[j].emod = bp[j].len - 1;
			rel_move(tc_col, tc_line, bp[j].smod, i);
			if (bp[j].mode & STANDOUT)
				standoutput((char*)bp[j].data, (int)bp[j].smod,
					    (int)bp[j].emod + 1, (int)bp[j].sso,
					    (int)bp[j].eso);
			else
				output((char*)&bp[j].data[bp[j].smod],
				       (int)(bp[j].emod - bp[j].smod + 1));
			tc_col = bp[j].emod + 1;
			if (tc_col >= t_columns) {
				if (automargins) {
					tc_col -= t_columns;
					tc_line++;
					if (tc_line >= t_lines)
						tc_line = t_lines - 1;
				} else
					tc_col = t_columns - 1;
			}
		}
		if (bp[j].oldlen > bp[j].len) {
			rel_move(tc_col, tc_line, bp[j].len, i);
			o_cleol();
		}
		bp[j].oldlen = bp[j].len;
	}
	rel_move(tc_col, tc_line, cur_col, cur_ln);
	oflush();
}

void
move(int y, int x)
{
	cur_col = x /* +c_shift(y,x) */ ;
	cur_ln = y;
}

void
getyx(int *y, int *x)
{
	*y = cur_ln;
	*x = cur_col /*-c_shift(y,x)*/ ;
}

void
clear(void)
{
	int i;
	struct screenline *slp;

	if (dumb_term)
		return;
	roll = 0;
	docls = YEA;
	downfrom = 0;
	for (i = offsetln; i < scr_lns; i++) {
		slp = &big_picture[i];
		slp->mode = 0;
		slp->len = 0;
		slp->oldlen = 0;
	}
	move(offsetln, 0);
}

void
clrtoeol(void)
{
	struct screenline *slp;
	int ln;

	if (dumb_term)
		return;
	standing = NA;
	ln = cur_ln + roll;
	while (ln >= scr_lns)
		ln -= scr_lns;
	slp = &big_picture[ln];
	if (cur_col <= slp->sso)
		slp->mode &= ~STANDOUT;
	if (cur_col > slp->oldlen && cur_col - slp->len > 0)
		memset(&slp->data[slp->len], ' ', cur_col - slp->len + 1);
	slp->len = cur_col;
}

void
clrtobot(void)
{
	struct screenline *slp;
	int i, j;

	if (dumb_term)
		return;
	for (i = cur_ln; i < scr_lns; i++) {
		j = i + roll;
		while (j >= scr_lns)
			j -= scr_lns;
		slp = &big_picture[j];
		slp->mode = 0;
		slp->len = 0;
		if (slp->oldlen > 0)
			slp->oldlen = 255;
	}
}

void
outc(unsigned char c)
{
	struct screenline *slp;
	unsigned char reg_col;

#ifndef BIT8
	c &= 0x7f;
#endif
	if (inansi == 1) {
		if (c == 'm') {
			inansi = 0;
			return;
		}
		return;
	}
	if (c == KEY_ESC && iscolor == NA) {
		inansi = 1;
		return;
	}
	if (dumb_term) {
		if (!isprint2(c)) {
			if (c == '\n') {
				ochar('\r');
			} else if (c != KEY_ESC || !showansi) {
				c = '*';
			}
		}
		ochar(c);
		return;
	}

	slp = &big_picture[(cur_ln + roll) % scr_lns];
	reg_col = cur_col;
	/* deal with non-printables */
	if (!isprint2(c)) {
		if (c == '\n' || c == '\r') {	/* do the newline thing */
			if (standing) {
				if (reg_col > slp->eso)
					slp->eso = reg_col;
				standing = NA;
			}
			if (reg_col > slp->len) {
				int i;

				for (i = slp->len; i <= reg_col; i++)
					slp->data[i] = ' ';
			}
			slp->len = reg_col;
			cur_col = 0;	/* reset cur_col */
			if (cur_ln < scr_lns)
				cur_ln++;
			return;
		} else if (c != KEY_ESC || !showansi) {
			c = '*';	/* else substitute a '*' for non-printable */
		}
	}
	if (reg_col >= slp->len) {
		int i;

		for (i = slp->len; i < reg_col; i++)
			slp->data[i] = ' ';
		slp->data[reg_col] = '\0';
		slp->len = reg_col + 1;
	}
	if (slp->data[reg_col] != c) {
		if ((slp->mode & MODIFIED) != MODIFIED)
			slp->smod = (slp->emod = reg_col);
		else {
			if (reg_col > slp->emod)
				slp->emod = reg_col;
			if (reg_col < slp->smod)
				slp->smod = reg_col;
		}
		slp->mode |= MODIFIED;
	}
	slp->data[reg_col] = c;
	reg_col++;
	if (reg_col >= scr_cols) {
		if (standing && slp->mode & STANDOUT) {
			standing = NA;
			if (reg_col > slp->eso)
				slp->eso = reg_col;
		}
		reg_col = 0;
		if (cur_ln < scr_lns)
			cur_ln++;
	}
	cur_col = reg_col;	/* store cur_col back */
}


/* void */
/* outns(char *str, int n) */
/* {  */
/*         int i; */

/*         for (i = 0; i < n; i++) { */
/* 		if (*str == '\r') */
/* 			continue; */
/*                 if (*str == '\0') */
/*                         break; */
/*                 outc(*str++); */
/*         } */
/* } */


/* gcc: 重写的outns */
void
outns(char *str, int n)
{
        int i, j, k;
	struct screenline *slp;
	int savex = -1, savey = -1;

	if (dumb_term || !big_picture) {
		for (i = 0; i < n && str[i]; i++) {
			if (!isprint2(str[i])) {
				if (str[i] == '\n') {
					ochar('\r');
				} else if (str[i] != KEY_ESC || !showansi) {
					str[i] = '*';
				}
			}
			ochar(str[i]);
		}
		return;
	}

	for (i = 0; i < n && str[i]; i++) {


		/* if (cur_ln >= scr_lns) */
		/* 	break; */

	        /* 获取当前行 */
		slp = &big_picture[(cur_ln + roll) % scr_lns];

		/* 如果光标当前列大于当前行长度，增加行空白字符至当前列 */
		if (cur_col >= slp->len) {
			for (j = slp->len; j <= cur_col; j++)
				slp->data[j] = ' ';
			slp->len = cur_col;
		}

		/* 处理连续的可打印字符 */
		if (isprint2(str[i])) {
			if (!(slp->mode & MODIFIED))
				slp->smod = slp->emod = cur_col;
			else if (cur_col < slp->smod)
				slp->smod = cur_col;

			do {
				if (cur_col < scr_cols) {
					slp->data[cur_col] = str[i];
					cur_col++;
				}
				i++;
			} while (i < n && isprint2(str[i]));
			i--;

			if (cur_col > slp->len)
				slp->len = cur_col;
			if (cur_col > slp->emod + 1)
				slp->emod = cur_col - 1;
			slp->mode |= MODIFIED;
			continue;
		}

		if (str[i] == '\r') continue;

		/* 处理换行符 */
		if (str[i] == '\n') {
			if (standing && slp->mode & STANDOUT) {
				if (cur_col > slp->eso)
					slp->eso = cur_col;
				standing = NA;
			}
			slp->len = cur_col;
			cur_col = 0;
			if (cur_ln < scr_lns)
				cur_ln++;
			continue;
		}

		/* 处理控制符 */
		if (str[i] == KEY_ESC && showansi) {
			if (str[i + 1] != '[')
				continue;

			/* 禁用时跳过 */
			if (iscolor == NA) {
				while (!isalpha(str[i]) && str[i])
					i++;
				continue;
			}

			/* 处理阶段 */
			/* 此处可利用esc控制符来拓展功能  */
			j = 1;
			while (!isalpha(str[i + j]) && str[i + j] != KEY_ESC
			       && str[i + j])
				j++;

			if (str[i + j] == 's' && j == 2) {
				i += j;
				savey = cur_ln;
				savex = cur_col;
				continue;
			} else if (str[i + j] == 'u' && j == 2) {
				i += j;
				if (savey != -1 && savex != -1 && !disable_move) {
					cur_ln = savey;
					cur_col = savex;
				}
				continue;
			} else if (str[i + j] == 'J') {
				i += j;
				if (!disable_move)
					clear();
				continue;
			} else if (str[i + j] == 'M') {
			    int t = 1;
			    for (k = 2; k < j; k++)
				t = t && (str[i + j] >= '0'
					  && str[i + j] <= '9');
			    if (t) {
			    	refresh();
			    	output(&str[i], j + 1);
			    	oflush();
			    }
			    i += j;
			    continue;
			} else if (str[i + j] == 'H' || str[i + j] == 'f') {
				k = 0;
				while (k < j && str[i + k] != ';')
					k++;
				if (str[i + k] == ';' && k <= 4 && k >= 3
				    && j - k >= 2 && j - k <= 4) {
					char s1[5], s2[5], x, y;
					memcpy(s1, &str[i + 2], k - 2);
					s1[k - 2] = '\0';
					memcpy(s2, &str[i + k + 1], j - k - 1);
					s2[j - k - 1] = '\0';
					y = atoi(s1) - 1 + offsetln;
					x = atoi(s2) - 1;
					if (y >= 0 && y < scr_lns && x >= 0
					    && x < scr_cols) {
						int t = 0;
						k = 0;
						/* 跳过颜色控制 */
						slp = &big_picture[(y + roll) % scr_lns];
						while (k < x) {
							if (k + t >= slp->len) break;
							if (slp->data[k + t] == KEY_ESC) {
								while (!isalpha(slp->data[k + t]) && slp->data[k + t]) t++;
								t++;
							}
							k++;
						}
						cur_col = x + t;
						cur_ln = y;
					}
				} else if (str[i + k] != ';') {
					if (offsetln == 0)
						clear();
				}
				i += j;
				continue;
			} else if ((str[i + j] == 'A' || str[i + j] == 'B'
			     || str[i + j] == 'C' || str[i + j] == 'D')
			    && j <= 5) {
				char s1[5];
				int t;
				s1[j - 2] = '\0';
				memcpy(s1, &str[i + 2], j - 2);
				
				if (s1[0])
					t = atoi(s1);
				else
					t = 1;
				if (!disable_move) {
					if (str[i + j] == 'A') {
						cur_ln -= t;
						if (cur_ln < offsetln)
							cur_ln = offsetln;
					} else if (str[i + j] == 'B') {
						cur_ln += t;
						if (cur_ln >= scr_lns)
							cur_ln = scr_lns - 1;
					} else if (str[i + j] == 'C') {
						cur_col += t;
						if (cur_col >= scr_cols)
							cur_col = scr_cols;
					} else if (str[i + j] == 'D') {
						/* 左移需要注意跳过颜色控制符 */
						unsigned char lastm = -1;
						for (k = cur_col - 1; k >= 0; k--) {
							if (slp->data[k] == 'm') {
								lastm = k;
								while (slp->data[k] != KEY_ESC && k >= 0)
									k--;
								if (slp->data[k] == KEY_ESC && cur_col - lastm < t)
									t += lastm - k + 1;
							}
						}
						cur_col = cur_col > t ? cur_col - t : 0;
					}
				}
				i += j;
				continue;
			} else if (str[i + j] == 'm') {
				if (!(slp->mode & MODIFIED))
					slp->smod = slp->emod = cur_col;
				else if (cur_col < slp->smod)
					slp->smod = cur_col;
				do {
					if (cur_col < scr_cols) {
						slp->data[cur_col] = str[i];
						cur_col++;
					}
					i++;
				} while (i < n && !isalpha(str[i]));
				
				slp->data[cur_col] = str[i];
				cur_col++;
				if (cur_col > slp->len)
					slp->len = cur_col;
				if (cur_col > slp->emod + 1)
					slp->emod = cur_col - 1;
				slp->mode |= MODIFIED;
				continue;
			}
			/* 屏蔽其它控制符 */
			else if (isalpha(str[i + j])) {
				i += j;
				continue;
			}
		}
		
		
		/* 以下处理其它非可见字符，用*代替显示 */
		char tmp = str[i];
		if (tmp != KEY_ESC || !showansi)
		    tmp = '*';
		if (slp->data[cur_col] != tmp) {
			slp->data[cur_col] = tmp;
			if ((slp->mode & MODIFIED) != MODIFIED)
				slp->smod = (slp->emod = cur_col);
			else {
				if (cur_col > slp->emod)
					slp->emod = cur_col;
				if (cur_col < slp->smod)
					slp->smod = cur_col;
			}
			slp->mode |= MODIFIED;
		}
		

		cur_col++;

	
	}
	
	
	
	
}

/* monster: here is a simplified implementation of prints */
void
prints(char *fmt, ...)
{
	va_list ap;
	char buf[1024];

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	buf[1023] = '\0';
	outns(buf, 1024);
	
}

void
scroll(void)
{
	if (dumb_term) {
		prints("\n");
		return;
	}
	scrollcnt++;
	roll++;
	if (roll >= scr_lns)
		roll -= scr_lns;
	move(scr_lns - 1, 0);
	clrtoeol();
}

void
rscroll(void)
{
	if (dumb_term) {
		prints("\n\n");
		return;
	}
	scrollcnt--;
	if (roll > 0)
		roll--;
	else
		roll = scr_lns - 1;
	move(0, 0);
	clrtoeol();
}

void
standout(void)
{
	struct screenline *slp;
	int ln;

	if (dumb_term || !strtstandoutlen)
		return;
	if (!standing) {
		ln = cur_ln + roll;
		ln %= scr_lns;
		slp = &big_picture[ln];
		standing = YEA;
		slp->sso = cur_col;
		slp->eso = cur_col;
		slp->mode |= STANDOUT;
	}
}

void
standend(void)
{
	struct screenline *slp;
	int ln;

	if (dumb_term || !strtstandoutlen)
		return;
	if (standing) {
		ln = cur_ln + roll;
		ln %= scr_lns;
		slp = &big_picture[ln];
		standing = NA;
		if (cur_col > slp->eso)
			slp->eso = cur_col;
	}
}

void
outs(char *str)
{
	outns(str, 4096);
}

void
saveline(int line, int mode)		/* 0, 2, 4, 6 : save, 1, 3, 5, 7 : restore */
{
	struct screenline *bp = big_picture;
	static char tmp[4][256];
	int x, y;

	if (mode % 2 == 0) {
		strlcpy(tmp[mode / 2], (char*)bp[line].data, LINELEN);
		tmp[mode / 2][bp[line].len] = '\0';
	} else {
		getyx(&x, &y);
		move(line, 0);
		clrtoeol();
		refresh();
		outs(tmp[(mode - 1) / 2]);
		move(x, y);
		refresh();
	}
}

/* Pudding: 运用外部buffer的saveline */
void
saveline_buf(int line, int mode, struct screenline *buf)
{
	struct screenline *bp = big_picture;

	if (mode % 2 == 0) {
		memcpy(buf, bp + line, sizeof(struct screenline));
	} else {
		memcpy(bp + line, buf, sizeof(struct screenline));
	}
}
