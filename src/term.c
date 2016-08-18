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

#ifdef HP_UX
#define O_HUPCL 01
#define O_XTABS 02
#endif

#ifdef TERMIOS
#include <termios.h>
#define stty(fd, data) tcsetattr( fd, TCSANOW, data )
#define gtty(fd, data) tcgetattr( fd, data )
struct termios tty_state, tty_new;
#else
struct sgttyb tty_state, tty_new;
#endif

#ifndef TANDEM
#define TANDEM	0x00000001
#endif

#ifndef CBREAK
#define CBREAK  0x00000002
#endif

int
get_tty(void)
{
	if (gtty(1, &tty_state) < 0) {
		prints("gtty failed\n");
		oflush();
		sleep(2);
		exit(-1);
	}
	return 1;
}

#ifdef TERMIOS
void
init_tty()
{
	long vdisable;

	memcpy(&tty_new, &tty_state, sizeof (tty_new));
	tty_new.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ISIG);
	tty_new.c_cflag &= ~CSIZE;
	tty_new.c_cflag |= CS8;
	tty_new.c_cc[VMIN] = 1;
	tty_new.c_cc[VTIME] = 0;
	if ((vdisable = fpathconf(STDIN_FILENO, _PC_VDISABLE)) >= 0) {
		tty_new.c_cc[VSTART] = vdisable;
		tty_new.c_cc[VSTOP] = vdisable;
		tty_new.c_cc[VLNEXT] = vdisable;
	}
	tcsetattr(1, TCSANOW, &tty_new);
}
#else
void
init_tty(void)
{
	memcpy(&tty_new, &tty_state, sizeof (tty_new));
	tty_new.sg_flags |= RAW;

#ifdef HP_UX
	tty_new.sg_flags &= ~(O_HUPCL | O_XTABS | LCASE | ECHO | CRMOD);
#else
	tty_new.sg_flags &= ~(TANDEM | CBREAK | LCASE | ECHO | CRMOD);
#endif

	stty(1, &tty_new);
}
#endif

void
reset_tty(void)
{
	stty(1, &tty_state);
}

void
restore_tty(void)
{
	stty(1, &tty_new);
}

#define TERMCOMSIZE (255)

int dumb_term = YEA;

char clearbuf[TERMCOMSIZE];
int clearbuflen;

char cleolbuf[TERMCOMSIZE];
int cleolbuflen;

char cursorm[TERMCOMSIZE];
char *cm;

char scrollrev[TERMCOMSIZE];
int scrollrevlen;

char strtstandout[TERMCOMSIZE];
int strtstandoutlen;

char endstandout[TERMCOMSIZE];
int endstandoutlen;

int t_lines = 24;
int t_columns = 255;
int t_realcols = 80;

int automargins;

char *outp;
int *outlp;

static int
outcf(int ch)
{
	if (*outlp < TERMCOMSIZE) {
		(*outlp)++;
		*outp++ = ch;
	}
	return 0;
}

int
term_init(char *term)
{
	extern char PC, *UP, *BC;
	extern short ospeed;
	static char UPbuf[TERMCOMSIZE];
	static char BCbuf[TERMCOMSIZE];
	static char buf[5120];
	char sbuf[5120];
	char *sbp, *s;

#ifdef TERMIOS
	ospeed = cfgetospeed(&tty_state);
#else
	ospeed = tty_state.sg_ospeed;
#endif

	if (tgetent(buf, term) != 1)
		return NA;
	sbp = sbuf;
	s = tgetstr("pc", &sbp);	/* get pad character */
	if (s)
		PC = *s;
	t_lines = tgetnum("li");
	t_columns = tgetnum("co");
	automargins = tgetflag("am");
	outp = clearbuf;	/* fill clearbuf with clear screen command */
	outlp = &clearbuflen;
	clearbuflen = 0;
	sbp = sbuf;
	s = tgetstr("cl", &sbp);
	if (s)
		tputs(s, t_lines, outcf);
	outp = cleolbuf;	/* fill cleolbuf with clear to eol command */
	outlp = &cleolbuflen;
	cleolbuflen = 0;
	sbp = sbuf;
	s = tgetstr("ce", &sbp);
	if (s)
		tputs(s, 1, outcf);
	outp = scrollrev;
	outlp = &scrollrevlen;
	scrollrevlen = 0;
	sbp = sbuf;
	s = tgetstr("sr", &sbp);
	if (s)
		tputs(s, 1, outcf);
	outp = strtstandout;
	outlp = &strtstandoutlen;
	strtstandoutlen = 0;
	sbp = sbuf;
	s = tgetstr("so", &sbp);
	if (s)
		tputs(s, 1, outcf);
	outp = endstandout;
	outlp = &endstandoutlen;
	endstandoutlen = 0;
	sbp = sbuf;
	s = tgetstr("se", &sbp);
	if (s)
		tputs(s, 1, outcf);
	sbp = cursorm;
	cm = tgetstr("cm", &sbp);
	if (cm)
		dumb_term = NA;
	else
		dumb_term = YEA;
	sbp = UPbuf;
	UP = tgetstr("up", &sbp);
	sbp = BCbuf;
	BC = tgetstr("bc", &sbp);
	if (dumb_term) {
		t_lines = 24;
		t_columns = 255;
	}
	return YEA;
}

void
do_move(int destcol, int destline, int (*outc)(int))
{
	tputs(tgoto(cm, destcol, destline), 0, outc);
}
