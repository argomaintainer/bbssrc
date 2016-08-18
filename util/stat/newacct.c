/* $Id: newacct.c,v 1.1.1.1 2003-02-20 19:54:45 bbs Exp $ */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

#define MAX_LINE        (15)

char datestring[30];

struct {
	int no[24];		/* ´ÎÊý */
	int sum[24];		/* ×ÜºÏ */
} st;

void
getdatestring(time_t now)
{
	struct tm *tm;
	char weeknum[7][3] = { "Ìì", "Ò»", "¶þ", "Èý", "ËÄ", "Îå", "Áù" };

	tm = localtime(&now);
	sprintf(datestring, "%4dÄê%02dÔÂ%02dÈÕ%02d:%02d:%02d ÐÇÆÚ%2s",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, weeknum[tm->tm_wday]);
}

int
main()
{
	FILE *fp;
	char buf[256];
	char date[80];
	time_t now;
	int hour, max = 0, item, total = 0;
	int i, j;
	char *blk[10] = {
		"£ß", "£ß", "¨x", "¨y", "¨z",
		"¨{", "¨|", "¨}", "¨~", "¨€",
	};

	sprintf(buf, "%s/reclog/usies", BBSHOME);

	if ((fp = fopen(buf, "r")) == NULL) {
		printf("cann't open usies\n");
		return 1;
	}

	now = time(0);
	getdatestring(now);
	sprintf(date, "%24.24s", ctime(&now));

	while (fgets(buf, 256, fp)) {
		hour = atoi(buf + 11);
		if (hour < 0 || hour > 23) {
			//printf("%s", buf);
			continue;
		}

		if ((strncmp(buf, date, 10)) ||
		    (atoi(buf + 20) != atoi(date + 20)))
			continue;
		if (!strncmp(buf + 25, "APPLY", 5)) {
			st.no[hour]++;
			continue;
		}

		if (strncmp(buf + 44, "Stay:", 5)) ;
		{
			st.sum[hour] += atoi(buf + 59);
			continue;
		}
	}
	fclose(fp);
	for (i = 0; i < 24; i++) {
		total += st.no[i];
		if (st.no[i] > max)
			max = st.no[i];
	}

	item = max / MAX_LINE + 1;
	sprintf(buf, "%s/0Announce/bbslist/newacct.today", BBSHOME);
	if ((fp = fopen(buf, "w")) == NULL) {
		printf("Cann't open newacct\n");
		return 1;
	}

	fprintf(fp,
		"\n[1;36m   ©°¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª¡ª©´\n");
	for (i = MAX_LINE; i >= 0; i--) {
		fprintf(fp, "[1;37m%3d[36m©¦[31m", (i + 1) * item);
		for (j = 0; j < 24; j++) {
			if ((item * (i) > st.no[j]) &&
			    (item * (i - 1) <= st.no[j]) && st.no[j]) {
				fprintf(fp, "[35m%-3d[31m", (st.no[j]));
				continue;
			}
			if (st.no[j] - item * i < item && item * i < st.no[j])
				fprintf(fp, "%s ",
					blk[((st.no[j] -
					      item * i) * 10) / item]);
			else if (st.no[j] - item * i >= item)
				fprintf(fp, "%s ", blk[9]);
			else
				fprintf(fp, "   ");
		}
		fprintf(fp, "[1;36m©¦\n");
	}
	fprintf(fp,
		"  [37m0[36m©¸¡ª¡ª¡ª [37m%-9.9s ±¾ÈÕÐÂÔöÈË¿ÚÍ³¼Æ[36m¡ª¡ª¡ª¡ª[37m%14s[36m¡ª©¼\n"
		"   [;36m  00 01 02 03 04 05 06 07 08 09 10 11 [1;32m12 13 14 15 16 17 18 19 20 21 22 23\n\n"
		"                     [33m1 [31m¡ö [33m= [37m%-5d [33m±¾ÈÕÉêÇëÐÂÕÊºÅÈËÊý£º[37m%-9d[m\n",
		BBSNAME, datestring, item, total);
	fclose(fp);
	return 0;
}
