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
    Copyright (C) 2001-2003, Yu Chen, monster@marco.zsu.edu.cn
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

/* modify from: util/poststat.c      ( NTHU CS MapleBBS Ver 2.36 )    */

#include "bbs.h"

char *myfile[] = { "day", "week", "month", "year" };
char *myfile2[] = { "day2", "week2", "month2", "year2" };

int ptype;	/* ÈËÊı¡¡or¡¡ÆªÊı */
int mycount[4] = { 7, 4, 12 };
int mytop[] = { 10, 50, 100, 150 };
char *mytitle[] = { "ÈÕÊ®", "ÖÜÎåÊ®", "ÔÂÒ»°Ù", "Äê¶È°ÙÎå" }; 

#define HASHSIZE 1024
#define TOPCOUNT 200

/* hash_bucket */
struct postrec {
	struct postlog log;
	struct postrec *next;
}*bucket[HASHSIZE];

struct postlog top[TOPCOUNT]; 	/* ±£´ætop 200 postlog */


struct postent {
	struct postlog log;
	struct fileheader fileinfo;
} postinfo[TOPCOUNT];

/* freestyler: ¿ØÖÆÊ®´ó±êÌâ*/
int has_keyword(char *title) {

	char buf[200];
	FILE* fp = fopen("etc/top10keyword", "r");

	if(fp) {
		while(fgets(buf, 100, fp)) {
			if(strlen(buf) <= 4)
				continue;
			buf[strlen(buf) - 1] = 0; /* ?? */
			if(strstr(title, buf)) {
				fclose(fp);
				return 1;
			}
		}
	}
	fclose(fp);
	return 0;
}

int get_fileinfo(int idx, struct fileheader *fh) {
	static char filepath[256];
	int fd;
	size_t size;
	void *buf, *start, *end;
	struct stat st;
	
	size = sizeof(struct fileheader);
	setboardfile(filepath, top[idx].board, DOT_DIR);
	
	if ((fd = open(filepath, O_RDONLY, 0)) == -1)
		return 0;
	if (fstat(fd, &st) < 0) {
		close(fd);
		return 0;
	}
	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);
	if (buf == MAP_FAILED || st.st_size <= 0) {
		return 0;
	}

	start = buf;
	end = buf + st.st_size;

	while (start < end) {
		memcpy(fh, start, size);
		if (fh->id == top[idx].id)
			return 1;
		start += size;
		
	}
	

	munmap(buf, st.st_size);
	return 0;
}

/* hash function */
int
hash(char *key)
{
	int i, value = 0;

	for (i = 0; key[i] && i < 80; i++)
		value += key[i] < 0 ? -key[i] : key[i];

	value = value % HASHSIZE;
	return value;
}


/* ---------------------------------- */
/* hash structure : array + link list */
/* ---------------------------------- */

void
search(struct postlog *t)
{
	struct postrec *p, *q, *s;
	int i, found = 0;

	i = t->id % HASHSIZE;

	q = NULL;
	p = bucket[i];
	while (p && (!found)) {
		if ((p->log).id ==  t->id &&  /* ÎÄÕÂ±êÌâÏàÍ¬ */
		    !strcmp((p->log).board, t->board)) 	       /* Í¬Ò»°æÃæ */
			found = 1;
		else {
			q = p;
			p = p->next;
		}
	}
	if (found) {
		(p->log).number += t->number;
		if ((p->log).date < t->date)	/* È¡½Ï½üÈÕÆÚ */
			(p->log).date = t->date;
	} else {  /* ²åÈëÍ°ÖĞ */
		s = (struct postrec *)malloc(sizeof(struct postrec));
		memcpy(s, t, sizeof(struct postlog));
		s->next = NULL;
		if (q == NULL)
			bucket[i] = s;
		else
			q->next = s;
	}
}

/* ²åÈëÅÅĞò,  pp:´ı²åÈëÊı¾İ, count:topÊı×éÒÑÅÅĞòÊıÄ¿*/
int
sort(struct postrec *pp, int count)
{
	int i, j;

	for (i = 0; i <= count; i++) {
		if ((pp->log).number > top[i].number) {
			if (count < TOPCOUNT - 1)
				count++;
			for (j = count - 1; j >= i; j--)
				memcpy(&top[j + 1], &top[j], sizeof(struct postlog));
			memcpy(&top[i], pp, sizeof(struct postlog));
			break;
		}
	}
	return count;
}

void
load_stat(char *fname)
{
	FILE *fp;

	if ((fp = fopen(fname, "r")) != NULL) {
		int count = fread(top, sizeof(struct postlog), TOPCOUNT, fp);

		fclose(fp);
		while (count)
			search(&top[--count]);
	}
}

void
poststat(int mytype)
{
	FILE *fp;
	char buf[40], currfile[40], logfile[40], oldfile[40], *p;
	struct postrec *pp;
	struct fileheader fileinfo;
	int i, j;

	if (ptype == 1) { /* Í³¼ÆÈËÊı */
		strcpy(logfile, ".post");
		strcpy(oldfile, ".post.old");
		strcpy(currfile, "etc/posts/day.0");
	} else {	/*¡¡Í³¼ÆÆªÊı¡¡*/
		strcpy(logfile, ".post2");
		strcpy(oldfile, ".post2.old");
		strcpy(currfile, "etc/posts/day2.0");
	}

	if (mytype < 0) { 
		/* --------------------------------------- */
		/* load .post and statictic processing     */
		/* --------------------------------------- */
		remove(oldfile); /* É¾µô¾Élogfile¡¡*/
		rename(logfile, oldfile); /* ½«logfile¸ÄÃûÎªoldfile, logfile½«ÖØĞÂÉú³É */
		if ((fp = fopen(oldfile, "r")) == NULL)
			return;
		mytype = 0;
		load_stat(currfile); /* ¼ÌĞøÒÔÇ°*/

		while (fread(top, sizeof(struct postlog), 1, fp)) /*¡¡new post */
			search(top); /* merge */
		fclose(fp);
	} else {
		/* ---------------------------------------------- */
		/* load previous results and statictic processing */
		/* ---------------------------------------------- */

		i = mycount[mytype];
		p = (ptype == 1) ? myfile[mytype] : myfile2[mytype];
		while (i) {
			sprintf(buf, "etc/posts/%s.%d", p, i);
			sprintf(currfile, "etc/posts/%s.%d", p, --i);
			load_stat(currfile);
			rename(currfile, buf);
		}
		mytype++;
	}

	/* ---------------------------------------------- */
	/* sort top 100 issue and save results            */
	/* ---------------------------------------------- */

	memset(top, 0, sizeof(top));
	for (i = j = 0; i < HASHSIZE; i++) { /* for each bucket */
		for (pp = bucket[i]; pp; pp = pp->next) { /* for each postlog */

#ifdef  DEBUG
			printf
				("ID : %s, Board: %s\nPostNo : %d\n",
				 pp->log.id, pp->log.board, pp->log.number);
#endif

			j = sort(pp, j); /* insert into top */
		}
	}

	p = (ptype == 1) ? myfile[mytype] : myfile2[mytype];
	sprintf(currfile, "etc/posts/%s.0", p);  /* ±£´æÆğÀ´ÎÄ¼ş*/
	if ((fp = fopen(currfile, "w")) != NULL) {
		fwrite(top, sizeof(struct postlog), j, fp);  /* ±£´æÆğÀ´ */
		fclose(fp);
	}



	i = mytop[mytype];
	int num = j , k;
	if (num > i)
		num = i;
	int board_hash[HASHSIZE];
	memset(board_hash, 0, sizeof(board_hash));
	/* ËµÃ÷£ºiÎª±éÀútopÊı×éµÄÏÂ±ê£¬jÎªtopÊı×éµÄ´óĞ¡£¬numÎªÒªÊä³öµÄÌõÄ¿Êı£¬kÎªµ±Ç°Êä³öµÄÌõÄ¿Êı */
	for (i = 0, k = 0; k < num && i != j; i++) {
		if(board_hash[ hash(top[i].board) ] != 0)	/* Õâ¸ö°æÒÑ¾­ÓĞÒ»¸öÊ®´óÃû¶îÁË */
			continue;
		
		if (!get_fileinfo(i, &fileinfo))
			continue;
		if (has_keyword(fileinfo.title))
			continue;
		
		board_hash[ hash(top[i].board) ] = 1;

		postinfo[k].log = top[i];
		postinfo[k].fileinfo = fileinfo;
		k++;
	}
	
	/* for telnet */
	sprintf(currfile, "etc/posts/%s", p);
	if ((fp = fopen(currfile, "w")) != NULL) {
		fprintf(fp, "                [1;34m-----[37m=====[41m ±¾%s´óÈÈÃÅ»°Ìâ [40m=====[34m-----[0m\n\n",
			mytitle[mytype]);

		for (i = 0; i < k; i++) {

			strcpy(buf, ctime(&postinfo[i].log.date)); /* ctime: Wed Jun 30 21:49:08 1993 */
			buf[20] = '\0';
			p = buf + 4;
			fprintf(fp,
				"[1;37mµÚ[1;31m%3d[37m Ãû [37mĞÅÇø : [33m%-16s[37m¡¾[32m%s[37m¡¿[36m%4d [37m%s[35m%+16s\n"
				"     [37m±êÌâ : [1;44;37m%-60.60s[40m\n",
				i + 1 , postinfo[i].log.board, p, postinfo[i].log.number, (ptype == 1) ? "ÈË" : "Æª",
				postinfo[i].fileinfo.owner, postinfo[i].fileinfo.title);
		}
		fclose(fp);
	}


	/* for http */
	p = (ptype == 1) ? myfile[mytype] : myfile2[mytype];
	sprintf(currfile, "etc/posts/http.%s", p);
	if ((fp = fopen(currfile, "w")) != NULL) {
		for (i = 0; i < k; i++) {
			/* author	title	boardname	filename	date	number */
			/* for split in python script */
			fprintf(fp,
				"%s\t%s\t%s\t%s\t%d\t%d\n",
				postinfo[i].fileinfo.owner, postinfo[i].fileinfo.title,
				postinfo[i].log.board, postinfo[i].fileinfo.filename,
				postinfo[i].log.date, postinfo[i].log.number);
		}
		fclose(fp);
	}
	

	/* free statistics */

	for (i = 0; i < HASHSIZE; i++) {
		struct postrec *pp0;

		pp = bucket[i];
		while (pp) {
			pp0 = pp;
			pp = pp->next;
			free(pp0);
		}
		bucket[i] = NULL;
	}
}

int
main(int argc, char **argv)
{
	time_t now;
	struct tm *ptime;   	/* defined in time.h */

	chdir(BBSHOME);
	ptype = (argc > 1) ? atoi(argv[1]) : 1; /* Í³¼ÆÈËÊı¡¡or¡¡ÆªÊı */

	if (argc > 2) {
		poststat(atoi(argv[2]));
		return (0);
	}

	time(&now);	/* get the time now */
	ptime = localtime(&now); /* convert to tm struct */
	if (ptime->tm_hour == 0) {  /*  0Ê± */
		if (ptime->tm_mday == 1) /* µ±ÔÂ 1 ºÅ, ¸üĞÂÄê*/
			poststat(2);
		if (ptime->tm_wday == 0) /* ĞÇÆÚÌì , ¸üĞÂÔÂ*/
			poststat(1);
		poststat(0);  /* ¸üĞÂµ±week */
	}
	poststat(-1);  /* ±¾ÈÕÊ®´ó */
	return 0;
}
