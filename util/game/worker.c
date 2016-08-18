/*
	   program name: worker.c
	   Chinese name: ÍÆÏä×Ó
	   this program is a GNU free software
	   first written by period.bbs@smth.org
			and cityhunter.bbs@smth.org, Nov 11, 1998(?)
	   rewitten by zhch.bbs@bbs.nju.edu.cn, Nov 27, 2000
	   last modified by soff bitbbs.org Dec 2, 2000 Ôö¼Óµ¥¹Ø×îºÃ³É¼¨

	   Ê¹ÓÃ·½·¨, °Ñworker·ÅÔÚbinÄ¿Â¼, map.dat·ÅÔÚbbs homeÄ¿Â¼,
	È»ºóÔÚbbsÖĞÓÃsystemµ÷ÓÃ:
	(Èç¹ûmkcfraw³ö´í, ÆÁ±Îµôtelnet_init, µ«ÕâÑù¾ÍÖ»ÄÜÔÚbbsÖĞµ÷ÓÃÁË)

	cc worker.c -o worker
	cp worker ~/bin
	cp map.dat ~
*/

#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#define MAX_STAGE 250
#define TIMEOUT   300

char genbuf[256];
char topID[MAX_STAGE][20];
int topStep[MAX_STAGE];
char userid[20] = "unknown.";

struct termios oldtty, newtty;

#define KEY_UP 		'A'
#define KEY_DOWN	'B'
#define KEY_RIGHT	'C'
#define KEY_LEFT	'D'

#define move(y, x) printf("[%d;%dH", y+1, x*2+1)
#define clear() printf("[H[J")
#define pressanykey() inkey()
#define fatal(s) printf("Fatal error: %s!\r\n", s); quit()
#define refresh() fflush(stdout)

int map_data[MAX_STAGE][20][40];
int map_now[20][40];
int map_total = 1;

int stage;
int now_y, now_x;
int step;

struct history {
	int max;
  char manmove[10000];
	char y0[10000];
	char x0[10000];
	char y1[10000];
	char x1[10000];
} my_history;

struct termios __t, __t0;
int ttymode = 0;

static void sig_terminate(int signo)
{
	static int sig;

	if (sig == 0) {
		sig = 1;
		quit();                
	}
}

int
telnet_init()
{
	if (tcgetattr(0, &__t0) >= 0) {
		ttymode = 1;
		cfmakeraw(&__t);
		tcsetattr(0, TCSANOW, &__t);
	}
}

int
quit()
{
	if (ttymode)
		tcsetattr(0, TCSANOW, &__t0);
	exit(0);
}

int
_inkey()
{
	if (read(0, genbuf, 1) <= 0)
		quit();
	return genbuf[0];
}

int
inkey()
{
	int c;

	c = _inkey();
	if (c == 3 || c == 4)
		quit();
	alarm(TIMEOUT);
	if (c == 127)
		c = 8;
	if (c != 27)
		return c;
	c = _inkey();
	c = _inkey();
	if (c >= 'A' && c <= 'D')
		return c;
	return 0;
}

char *
map_char(int n)
{
	if (n & 8)
		return "¡ö";
	if (n & 4)
		return "¡õ";
	if (n & 2)
		return "¡Ñ";
	if (n & 1)
		return "¡¤";
	return "  ";
}

int
map_init()
{
	FILE *fp;
	int map_y = 0, map_x = 0;

	fp = fopen("etc/map.dat", "r");
	if (fp == NULL)
		return 0;
	map_x = 0;
	while (1) {
		memset(genbuf, 0, 80);
		if (fgets(genbuf, 80, fp) <= 0)
			break;
		if (genbuf[0] == '-') {
			map_y = 0;
			map_total++;
		}
		for (map_x = 0; map_x < 40; map_x++) {
			if (!strncmp(genbuf + map_x * 2, map_char(1), 2))
				map_data[map_total - 1][map_y][map_x] = 1;
			if (!strncmp(genbuf + map_x * 2, map_char(2), 2))
				map_data[map_total - 1][map_y][map_x] = 2;
			if (!strncmp(genbuf + map_x * 2, map_char(4), 2))
				map_data[map_total - 1][map_y][map_x] = 4;
			if (!strncmp(genbuf + map_x * 2, map_char(8), 2))
				map_data[map_total - 1][map_y][map_x] = 8;
		}
		if (map_y < 20)
			map_y++;
	}
	fclose(fp);
	return 1;
}

int
map_show()
{
	int m, n, c;
	char *s;

	clear();
	for (n = 0; n < 20; n++) {
		for (m = 0; m < 40; m++)
			printf("%2.2s", map_char(map_now[n][m]));
		printf("\r\n");
	}
/*
	move(23, 0);
	printf("[44m                                                                        [m");
*/
	move(21, 0);
	worker_showrec();
	move(22, 0);
	clearln();
	move(22, 0);
	printf
	    ("¹¦ÄÜ¼ü  [1;32m¡ü[m [1;32m¡ı [m[1;32m¡û[m [1;32m¡ú[mÒÆ¶¯  ' ' [1;32m^C[m [1;32m^D[mÍË³ö  [1;32mBackSpace[m»ÚÆå  [1;32m^L[mË¢ĞÂ  [1;32mTAB[mÖØ¿ª\r\n");
	move(now_y, now_x);
	refresh();

}

int
map_show_pos(int y, int x)
{
	int c = map_now[y][x];

	move(y, x);
	if (c == 5)
		printf("[1;32m%2.2s[m", map_char(c));
	else
		printf("%2.2s", map_char(c));
}

int
check_if_win()
{
	int m, n;

	for (n = 0; n < 20; n++)
		for (m = 0; m < 40; m++)
			if (map_now[n][m] == 1 || map_now[n][m] == 3)
				return 0;
	return 1;
}

int
find_y_x(int *y, int *x)
{
	int m, n;

	for (n = 0; n < 20; n++)
		for (m = 0; m < 40; m++)
			if (map_now[n][m] & 2) {
				*x = m;
				*y = n;
				return 1;
			};
	return 0;
}

map_move(int y0, int x0, int y1, int x1)
{
	int b0, f0, b1, f1;
	char buf[78];

	b0 = map_now[y0][x0] & 1;
	f0 = map_now[y0][x0] & 6;
	b1 = map_now[y1][x1] & 1;
	map_now[y1][x1] = f0 | b1;
	map_now[y0][x0] = b0;
	map_show_pos(y0, x0);
	map_show_pos(y1, x1);
	move(22, 0);
	clearln();
	move(22, 0);
	printf
	    ("[0;44m[[1;33mÍÆÏä×Ó v1127[0;44m]    [µÚ [1;33m%d[0;44m ¹Ø] [µÚ [1;33m%d[0;44m ²½] [From (%d,%d) to (%d,%d)] [m",
	     stage + 1, step, y0, x0, y1, x1);
}

int
main(int argc, char **argv)
{
	int c, m, n, i;
	int dx, dy;

	signal(SIGALRM, sig_terminate);
	alarm(TIMEOUT);

	tcgetattr(0, &oldtty);
	cfmakeraw(&newtty);
	tcsetattr(0, TCSANOW, &newtty);
	
	if (argc >= 2) {
		strcpy(userid, argv[1]);
	}

	telnet_init();
	if (map_init() == 0)
		printf("map.dat error\n");
	stage = 0;
	clear();
	printf("»¶Ó­¹âÁÙ[1;32mÍÆÏä×Ó[mÓÎÏ·¡£\r\n");
	printf
	    ("¹æÔòºÜ¼òµ¥£¬Ö»Ğè°ÑËùÓĞµÄ'¡õ'¶¼ÍÆµ½'¡¤'ÉÏÃæÈ¥(»á±ä³ÉÂÌÉ«)¾Í¹ı¹ØÁË¡£\r\n");
	printf("µ«ÍæÆğÀ´ÄÑ¶È¿ÉÊÇÏàµ±´óµÄ£¬²»ÒªÇáÊÓà¸¡£\r\n");
	refresh();
	pressanykey();
	clear();
	printf("ÇëÓÃ·½Ïò¼üÑ¡¹Ø, »Ø³µ¼üÈ·ÈÏ: 1 ");
	move(0, 14);
	refresh();
	stage = 0;
	while (1) {
		c = inkey();
		// below modified by soff
		if (c == KEY_LEFT || c == KEY_UP) {
			if (stage > 0)
				stage--;
			else
				stage = map_total - 1;
		}
		if (c == KEY_RIGHT || c == KEY_DOWN) {
			if (stage < map_total - 1)
				stage++;
			else
				stage = 0;
		}
		if (c == 10 || c == 13)
			break;
		if (c == 3 || c == 4 || c == 32)
			quit();
		move(0, 14);
		printf("%d  ", stage + 1);
		move(0, 14);
		refresh();
	}
      start:
	if (stage < 0 || stage > map_total - 1)
		stage = 0;
	clear();
	printf("ÍÆÏä×Ó: µÚ [1;32m%d[m ¹Ø\n\n", stage + 1);
	move(10, 0);
	worker_showrec();
	pressanykey();
	refresh();
	sleep(2);
      start2:
	step = 0;
	for (n = 0; n < 20; n++)
		for (m = 0; m < 40; m++)
			map_now[n][m] = map_data[stage][n][m];
	if (!find_y_x(&now_y, &now_x))
		printf("stage error\n");
	map_show();
	memset(&my_history, 0, sizeof (my_history));
	while (1) {
		c = inkey();
		if (step >= 1999) {
			move(22, 0);
			clearln();
			move(22, 0);
			printf("ÄãÓÃÁË2000²½»¹Ã»ÓĞ³É¹¦! GAME OVER.");
			quit();
		}
		dx = 0;
		dy = 0;
		if (c == 8 && my_history.max > 0) {
      do
      {
  			my_history.max--;
	  		i = my_history.max;
        if (my_history.manmove[i]) step--;
		  	map_move(my_history.y1[i], my_history.x1[i],
			  	 my_history.y0[i], my_history.x0[i]);
  			find_y_x(&now_y, &now_x);
	  		move(now_y, now_x);
		  	refresh();
      } while (my_history.max > 1 && !my_history.manmove[my_history.max-1]);
			continue;
		}

		if (c == ' ')
			quit();
		if (c == '')
			map_show();
		if (c == 9)
			goto start2;

		if (c == KEY_UP)
			dy = -1;
		if (c == KEY_DOWN)
			dy = 1;
		if (c == KEY_LEFT)
			dx = -1;
		if (c == KEY_RIGHT)
			dx = 1;

		if (dx == 0 && dy == 0)
			continue;

		if (map_now[now_y + dy][now_x + dx] & 4)
			if (map_now[now_y + dy * 2][now_x + dx * 2] < 2) {
				map_move(now_y + dy, now_x + dx,
					 now_y + dy * 2, now_x + dx * 2);
				i = my_history.max;
        my_history.manmove[i] = 0;
				my_history.y0[i] = now_y + dy;
				my_history.x0[i] = now_x + dx;
				my_history.y1[i] = now_y + dy * 2;
				my_history.x1[i] = now_x + dx * 2;
				my_history.max++;
			}
		if (map_now[now_y + dy][now_x + dx] < 2) {
			i = my_history.max;
      my_history.manmove[i] = 1;
			my_history.y0[i] = now_y;
			my_history.x0[i] = now_x;
			my_history.y1[i] = now_y + dy;
			my_history.x1[i] = now_x + dx;
			my_history.max++;
      step++;
			map_move(now_y, now_x, now_y + dy, now_x + dx);
		}
		if (check_if_win())
			break;
		find_y_x(&now_y, &now_x);
		move(now_y, now_x);
		refresh();
	}
	move(21, 0);
	clearln();
	move(21, 0);
	printf("×£ºØÄã, Äã³É¹¦ÁË£¡Ò»¹²ÓÃÁË%d²½Íê³ÉÈÎÎñ! ", step);
	worker_checkrec(step);
	refresh();
	sleep(3);
	stage++;
	goto start;
}

int
worker_showrec()
{
	worker_loadrec();
	move(22, 0);
	printf
	    ("ÄãºÃ£¬[1;36m%s[m£¬±¾¹Ø×îºÃ³É¼¨±£³ÖÕß£º[1;36m%s[m£¬³É¼¨Îª£º[1;31m%d[m²½£¬ÕùÈ¡´òÆÆ¼ÇÂ¼Å¶£¡",
	     userid, topID[stage], topStep[stage]);

	return;
}

int
worker_loadrec()
{
	FILE *fp;
	int n;

	for (n = 0; n < MAX_STAGE; n++) {
		strcpy(topID[n], "ÎŞÃûÊÏ");
		topStep[n] = 2000;
	}
	fp = fopen("reclog/worker.rec", "r");
	if (fp == NULL) {
		worker_saverec();
		return;
	}
	for (n = 0; n <= map_total - 1; n++)
		fscanf(fp, "%s %d\n", topID[n], &topStep[n]);
	fclose(fp);
}

int
worker_saverec()
{
	FILE *fp;
	int n;

	fp = fopen("reclog/worker.rec", "w");
	for (n = 0; n <= MAX_STAGE; n++)
		fprintf(fp, "%s %d\n", topID[n], topStep[n]);
	fclose(fp);
}

int
worker_checkrec(int steps)
{
	worker_loadrec();
	if (steps < topStep[stage]) {
		topStep[stage] = steps;
		strcpy(topID[stage], userid);
		worker_saverec();

		clear();
		printf("×£ºØ! Äú»ñµÃÁË×îºÃ³É¼¨£º\033[1;31m%d\033[m²½!\r\n",
		       steps);
		pressanykey();
	}
}

clearln()
{
	char buf[78];

	memset(buf, 32, 77);
	buf[77] = 0;
	printf("\033[m%s", buf);
}
