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

    Contains codes from NJU distributions of Firebird Bulletin
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
#include <termios.h>

char host1[100][40], host2[100][40], ip[100][40];
int port[100], counts = 0;

char str[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()";
char datafile[PATH_MAX + 1] = "etc/bbsnet.ini";
char userid[STRLEN] = "unknown.";

int current_page = 1;
int t_lines = 22;
int oldn = -1;

#ifndef cfmakeraw
void cfmakeraw(struct termios *t);
#endif

void telnet(char *host, char *server, int port);
void telnetopt(int fd, unsigned char *buf, int max);
int custom_telnet(void);

int
dashf(char *fname)
{
	struct stat st;

	return (stat(fname, &st) == 0 && S_ISREG(st.st_mode));
}

#undef pressanykey
void
pressanykey(void)
{
	printf("\033[m\033[%d;%dH                                \033[5;1;33m按任何键继续...\033[m",
	       t_lines + 1, 0);
	fflush(stdout);
	getchar();
}

int
init_data(void)
{
	FILE *fp;
	char t[256], *t1, *t2, *t3, *t4;

	if ((fp = fopen(datafile, "r")) == NULL)
		return -1;

	counts = 0;
	while (fgets(t, 255, fp) && counts <= 72) {
		t1 = strtok(t, " \t");
		t2 = strtok(NULL, " \t\n");
		t3 = strtok(NULL, " \t\n");
		t4 = strtok(NULL, " \t\n");
		if (t1[0] == '#' || t1 == NULL || t2 == NULL || t3 == NULL)
			continue;
		strncpy(host1[counts], t1, 16);
		strncpy(host2[counts], t2, 36);
		strncpy(ip[counts], t3, 36);
		port[counts] = t4 ? atoi(t4) : 23;
		counts++;
	}
	fclose(fp);
	return counts > 0 ? 0 : -1;
}

void
locate(int n)
{
	int x, y;

	if (n >= counts)
		return;
	y = n / 3 + 3;
	x = n % 3 * 25 + 4;

	printf("\033[%d;%dH", y, x);
}

void
sh(int n)
{
	if (n >= counts)
		return;
	if (oldn >= 0) {
		locate(oldn);
		printf("\033[1;32m %c.\033[m%s", str[oldn], host2[oldn]);
	}
	oldn = n;
	printf("\033[%d;3H\033[1;37m单位: \033[1;33m%s                   \033[%d;32H\033[1;37m 站名: \033[1;33m%s              \r\n",
	     t_lines, host1[n], t_lines, host2[n]);
	printf("\033[1;37m\033[%d;3H连往: \033[1;33m%s                   \033[%d;1H",
	       t_lines + 1, ip[n], t_lines - 1);
	locate(n);
	printf("[%c]\033[1;42m%s\033[m", str[n], host2[n]);
}

void
show_all(void)
{
	int n;

	printf("\033[H\033[2J\033[m");
	printf("┏━━━━━━━━━━━━━━━\033[1;35m 穿  梭  银  河 \033[m━━━━━━━━━━━━━━━┓\r\n");
	for (n = 1; n < t_lines; n++) {
		printf("┃                                                                            ┃\r\n");
	}
	printf("┃                                              \033[1;33m1-9: \033[1;36m换页    \033[1;33mCtrl+H: \033[1;36m获取帮助\033[m ┃\r\n");
	printf("┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
	printf("\033[%d;3H──────────────────────────────────────", t_lines - 1);
	for (n = 0; n < counts; n++) {
		locate(n);
		printf("\033[1;32m %c.\033[m%s", str[n], host2[n]);
	}
}

int
read0(void)
{
	unsigned char c;

	if (read(0, &c, 1) <= 0)
		return -1;
	return c;
}

int
getch(void)
{
	int c;
	static int lastc = 0;

	c = getchar();
	if (c == 10 && lastc == 13)
		c = getchar();
	lastc = c;
	if (c != 27)
		return c;
	c = getchar();
	c = getchar();
	if (c == 'A')
		return KEY_UP;
	if (c == 'B')
		return KEY_DOWN;
	if (c == 'C')
		return KEY_RIGHT;
	if (c == 'D')
		return KEY_LEFT;
	if (c == '5')
		return KEY_PGUP;
	if (c == '6')
		return KEY_PGDN;
	return 0;
}

int p;

#define refresh()               \
	{                       \
		show_all();     \
		sh(p);          \
		fflush(stdout); \
	}

void
bbsnethelp(void)
{
	FILE *fp;
	char buf[128];

	fp = fopen("help/bbsnethelp", "r");
	if (!fp)
		return;
	while (fgets(buf, 128, fp)) {
		buf[strlen(buf) - 1] = ' ';
		printf("%s\r\n", buf);
	}
	fclose(fp);
}

void
goto_page(int num)
{
	if (num == 1) {
		strlcpy(datafile, "etc/bbsnet.ini", sizeof(datafile));
	} else {
		snprintf(datafile, sizeof(datafile), "etc/bbsnet%d.ini", num);
	}
	
	if (init_data() == 0) {
		p = 0;
		oldn = -1;
		refresh();
		current_page = num;
	}
}

static void
input_timeout(int signo)
{
	exit(0);
}

void
main_loop(void)
{
	int c, n;

	while (1) {
		p = 0;
		refresh();
		while (1) {
			c = getch();
			if (c == 3 || c == 4 || c == 27 || c < 0)
				return;
			if (c == 8)	//帮助
			{
				printf("\033[H\033[2J\033[m");
				bbsnethelp();
				pressanykey();
				refresh();
				continue;
			}
			if (c == KEY_UP)	//向上
			{
				if (p < 3) {
					p += 3 * ((counts - 1) / 3 -
						  (p > ((counts - 1) % 3)));
				} else
					p -= 3;
			}
			if (c == KEY_DOWN)	//向下
			{
				if (p + 3 > counts - 1)
					p %= 3;
				else
					p += 3;
			}
			if (c == KEY_RIGHT)	//向右
			{
				p++;
				if (p > counts - 1)
					p = 0;
			}
			if (c == KEY_LEFT)	//向左
			{
				p--;
				if (p < 0)
					p = counts - 1;
			}
			if (c == KEY_PGUP || c == Ctrl('B')) {
				goto_page(current_page - 1);
				continue;
			}
			if (c == KEY_PGDN || c == Ctrl('F')) {
				goto_page(current_page + 1);
				continue;
			}
			if (c == 13 || c == 10) {
				/* monster: 自定义站点 */
				if (!strcmp(ip[p], "自定义站点")) {
					if (custom_telnet() == 0) {
						refresh();
						continue;
					} else {
						return;
					}
				} else {
					telnet(host1[p], ip[p], port[p]);
					return;
				}
			}
			// Added by cancel at 02.01.02, extended to support 9 pages by monster
			if (c >= '1' && c <= '9') {
				goto_page(c - '0');
				continue;
			}
			for (n = 0; n < counts; n++)
				if (str[n] == c)
					p = n;
			sh(p);
			fflush(stdout);
		}
	}
}

int
main(int argc, char **argv)
{
	int n;
	struct termios tty;

	for (n = 1; n < NSIG; n++)
		(void) signal(n, SIG_IGN);

	dup2(0, 1);
	dup2(0, 2);

	cfmakeraw(&tty);
	tcsetattr(0, TCSANOW, &tty);
	chdir(BBSHOME);

	if (argc >= 2)
		strcpy(datafile, argv[1]);
	if (argc >= 3)
		strcpy(userid, argv[2]);
	if (argc >= 4)
		t_lines = atoi(argv[3]);
	if (t_lines > 100 || t_lines < 10)
		t_lines = 22;

	init_data();
	main_loop();
	printf("\033[m");

	return 0;
}

void
bbsnet_log(char *s, char *server, int port)
{
	time_t t = time(NULL);
	FILE *fp;

	if ((fp = fopen("reclog/bbsnet.log", "a")) != NULL) {
		fprintf(fp, "%-12s %24.24s %s [%s:%d]\n", userid, ctime(&t), s,
			server, port);
		fclose(fp);
	}
}

unsigned char buf[2048];
struct timeval tv;
fd_set readfds;
int result, readnum;
int fd;

int
custom_telnet(void)
{
	int port = 23, curpos = 0;
	char server[80];
	char *p;
	int ch;

	signal(SIGALRM, input_timeout);
	alarm(300);

	printf("\033[%d;3H\033[1;37m请输入目标站点的地址：                                                   ",
		t_lines);
	printf("\033[%d;3H\033[1;37m连往：\033[1;33m______________\033[m", t_lines + 1);
	printf("\033[%d;9H\033[1;32m", t_lines + 1);
	fflush(stdout);
	memset(server, 0, sizeof (server));

	while (1) {
		ch = getch();
		if ((isdigit(ch) || isalpha(ch) || ch == '-' || ch == '.' ||
		     ch == ':') && (curpos < 50)) {
			server[curpos++] = ch;
			printf("%c", ch);
			fflush(stdout);
		} else if (ch == 8 && curpos > 0) {
			server[--curpos] = 0;
			printf("\033[%d;%dH%s\033[%d;%dH", t_lines + 1, 9 + curpos,
			       (curpos < 14) ? "\033[1;33m_\033[1;32m" : " ",
			       t_lines + 1, 9 + curpos);
			fflush(stdout);
		} else if (ch == 13) {
			if (curpos == 0)
				return 0;
			p = strstr(server, ":");
			if (p != NULL) {
				port = atoi(p + 1);
				if (port <= 0 || port > 65535)
					port = 23;
				*p = 0;
			}
			telnet("自定义站点", server, port);
			return 1;
		} else if (ch == 3 || ch == 4) {
			return 0;
		}
	}
}

void
telnet(char *host, char *server, int port)
{
	printf("\033[H\033[2J\033[m连往: %s (%s)...", host, server);
	fflush(stdout);
	bbsnet_log(host, server, port);

	if ((fd = async_connect(server, port, BBSNET_CONNECT_TIMEOUT)) < 0) {
		printf("\033[1;31m%s\033[m", (fd == -1) ? "失败" : "超时");
		fflush(stdout);
		sleep(3);
		return;
	}

	printf("\033[1;32m成功\033[m\r\n返回请用 \033[1;33m'Ctrl+]'\033[m 键.\r\n");
	fflush(stdout);

	while (1) {
		tv.tv_sec = BBSNET_NOINPUT_TIMEOUT;
		tv.tv_usec = 0;
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);
		FD_SET(0, &readfds);
		result = select(fd + 1, &readfds, NULL, NULL, &tv);
		if (result <= 0)
			break;
		if (FD_ISSET(0, &readfds)) {
			readnum = read(0, buf, 2048);
			if (readnum <= 0)
				break;
			if (buf[0] == 29)
				return;
			if (buf[0] == 13 && readnum == 1) {
				buf[1] = 10;
				readnum++;
			}
			if (write(fd, buf, readnum) == -1)
				break;
		} else {
			readnum = read(fd, buf, 2048);
			if (readnum <= 0)
				break;
			if (strchr((const char *) buf, 255)) {
				telnetopt(fd, buf, readnum);
			} else {
				if (write(0, buf, readnum) == -1)
					break;
			}
		}
	}
}

void
telnetopt(int fd, unsigned char *buf, int max)
{
	unsigned char c2, c3;
	unsigned char neg_naws[] = { IAC, WILL, TELOPT_NAWS, IAC, SB, TELOPT_NAWS, 0, 80, 0, 49, IAC, SE };
	unsigned char neg_do[] = { IAC, DO, 0 };
	unsigned char neg_dont[] = { IAC, DONT, 0 };
	unsigned char neg_will[] = { IAC, WILL, 0 };
	unsigned char neg_wont[] = { IAC, WONT, 0 };
	unsigned char neg_term[] = { IAC, SB, TELOPT_TTYPE, 0, 'A', 'N', 'S', 'I', IAC, SE };
	int i, j;

	for (i = 0; i < max; i++) {
#ifndef DDDD
		if (buf[i] != IAC) {
			// locate IAC
			for (j = i; j < max; j++) {
				if (buf[j] == IAC)
					break;
			}
			write(0, &buf[i], j - i);
			if (j == max) break;
			i = j;
		}
#else
		if (buf[i] != IAC) {
			write(0, &buf[i], 1);
			continue;
		}
#endif
		c2 = buf[i + 1];
		c3 = buf[i + 2];
		i += 2;

		if (c2 == DO && c3 == TELOPT_NAWS) {
			neg_naws[9] = (unsigned char)(t_lines & 0xff);
			write(fd, neg_naws, sizeof(neg_naws));
			continue;
		}

		if (c2 == DO && (c3 == TELOPT_SGA || c3 == TELOPT_TTYPE)) {
			neg_will[2] = c3;
			write(fd, neg_will, sizeof(neg_will));
			continue;
		}

		if ((c2 == WILL || c2 == WONT) &&
		    (c3 == TELOPT_ECHO || c3 == TELOPT_SGA || c3 == TELOPT_TTYPE)) {
			neg_do[2] = c3;
			write(fd, neg_do, sizeof(neg_do));
			continue;
		}

		if (c2 == WILL || c2 == WONT) {
			neg_dont[2] = c3;
			write(fd, neg_dont, sizeof(neg_dont));
			continue;
		}

		if (c2 == DO || c2 == DONT) {
			neg_wont[2] = c3;
			write(fd, neg_wont, sizeof(neg_wont));
			continue;
		}

		if (c2 == SB) {
			// locate end of sub negotiation
			while (c3 != SE && i < max) {
				c3 = buf[i];
				i++;
			}
			write(fd, neg_term, sizeof(neg_term));
		}
	}
	fflush(stdout);
}
