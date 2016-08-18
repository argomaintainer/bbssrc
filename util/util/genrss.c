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
    Copyright (C) 2001-2005, Yu Chen, yuchen@cs.jhu.edu

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
#undef snprintf
#undef vsnprintf
#include <iconv.h>

/* 
	The rss template used by the program is presented as follows,

	<?xml version="1.0" ?>
	<rss version="2.0">
	   <channel>
	      <title> Board Name </title>
	      <link> http://bbs.sysu.edu.cn/bbsdoc?board=<Board Name> </link>
	      <description> Chinese Board Name </description>
	      <language> zh-CN </language>
	      <pubDate> <Week Day>, DD MMM YYYY HH:MM:SS <time zone> </pubDate>
	      <item>
	          <title> Article Title </title>
	          <link> http://bbs.sysu.edu.cn/bbscon?board=<Board Name>&file=<Filename> </link>
	          <description> Auto-generated sample description from file </description>
	          <pubDate> <Week Day>, DD MMM YYYY HH:MM:SS <time zone> </pubDate>
	      </item>  	
	   </channel>
	</rss>

	Refer to http://feedvalidator.org/docs/rss2.html for additional information.
*/

/*
	TODO:

	1. Support more rss tags;
	2. Use UTF-8 encoding instead of GB2312;
	3. Better sampling function.
*/

/* Pudding: 2005-11-9
   1. fix bugs
   2. Use UTF-8 encoding instead of GB2312
   3. support more rss tags
 */

#define BUFSIZE		1024
#define LANG		"zh-CN"
#define RSSDIR		BBSHOME"/htdocs/rss"
#define TIMEFACTOR	3600 * 24 * 3
#define CHANNELPREFIX	"逸仙时空："
#define MAX_NDIGITEMS	65536
#define SAMPLE_LINES	1000

typedef struct {
	char bname[BFNAMELEN];
	char btitle[BTITLELEN];
	char filename[FNAMELEN];
	char title[TITLELEN];
	char owner[IDLEN + 2];
	time_t filetime;
	int itempos;
} digitem_t;

digitem_t digitem[MAX_NDIGITEMS];
int ndigitems = 0;

/* Code Conversion */
iconv_t gb2utf8;

int
ufprintf(FILE *fp, char *fmt, ...)
{
	static char buf[BUFSIZE * 8];
	static char bufout[BUFSIZE * 8];
	char *bufsrc, *bufdst;	

	/* print string */
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	/* convert */
	int in_top = sizeof(buf) - 1;
	int out_top = sizeof(bufout) - 1;
	bufsrc = buf;
	bufdst = bufout;
	while (*bufsrc != '\0') {
		iconv(gb2utf8, &bufsrc, &in_top, &bufdst, &out_top);
		if (*bufsrc != '\0') {
			bufsrc++;	/* skip invalid token */
			in_top--;
		}
	}
	/* output */
	fprintf(fp, "%s", bufout);
	return 0;
}


/* general purposed functions */

const char *weekdays[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

const char *months[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun",
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

char*
time_tostring(const time_t time_)
{
	struct tm *t = localtime(&time_);
	static char buf[BUFSIZE];

	snprintf(buf, sizeof(buf), "%s, %02d %s %04d %02d:%02d:%02d %s",
		weekdays[t->tm_wday],
		t->tm_mday,
		months[t->tm_mon],
		t->tm_year + 1900,
		t->tm_hour,
		t->tm_min,
		t->tm_sec,
		t->tm_zone
	);

	return buf;
}
#if 0
char*
fix_xmlstr(char *s)
{
	char *segptr, *bufptr, *ptr;
	static char buf[BUFSIZE * 6 + 1];

	for (segptr = ptr = s, buf[0] = 0, bufptr = &buf[0]; *ptr != 0; ptr++) {
		switch (*ptr) {
			case '<':
				memmove(bufptr, segptr, ptr - segptr);
				memmove(bufptr + (ptr - segptr), "&lt;", 5);
				bufptr = bufptr + (ptr - segptr) + 4;
				segptr = ptr + 1;
				break;
			case '>':
				memmove(bufptr, segptr, ptr - segptr);
				memmove(bufptr + (ptr - segptr), "&gt;", 5);
				bufptr = bufptr + (ptr - segptr) + 4;
				segptr = ptr + 1;
				break;
			case '&':
				memmove(bufptr, segptr, ptr - segptr);
				memmove(bufptr + (ptr - segptr), "&amp;", 6);
				bufptr = bufptr + (ptr - segptr) + 5;
				segptr = ptr + 1;
				break;
			case '\'':
				memmove(bufptr, segptr, ptr - segptr);
				memmove(bufptr + (ptr - segptr), "&apos;", 7);
				bufptr = bufptr + (ptr - segptr) + 6;
				segptr = ptr + 1;
				break;
			case '\"':
				memmove(bufptr, segptr, ptr - segptr);
				memmove(bufptr + (ptr - segptr), "&quot;", 7);
				bufptr = bufptr + (ptr - segptr) + 6;
				segptr = ptr + 1;
				break;
			default:
				break;
		}		
	}
	strcat(bufptr, segptr);
	return buf;
}
#endif

char*
fix_xmlstr(char *s)
{
	static char buf[BUFSIZE * 6 + 1];
	snprintf(buf, sizeof(buf), "<![CDATA[%s]]>", s);
	return buf;
}

/* filteransi: borrowed from ytht */
void
filteransi(char *line)
{
        int i, stat, j;
        stat = 0;
        j = 0;
        for (i = 0; line[i]; i++) {
                if (line[i] == '\033')
                        stat = 1;
                if (!stat) {
                        if (j != i)
                                line[j] = line[i];
                        j++;   
                }
                if (stat && ((line[i] > 'a' && line[i] < 'z')
                             || (line[i] > 'A' && line[i] < 'Z')
                             || line[i] == '@'))
                        stat = 0;
        }
        line[j] = 0;
}

/* bbs specified functions */

char*
fileguid(const char *bname, const char *fname)
{
	static char buf[BUFSIZE];

	snprintf(buf, sizeof(buf), "[%s].[%s]", bname, fname);
	return buf;
}

char*
fileurl(const char *bname, const char *fname)
{
	static char buf[BUFSIZE];

	snprintf(buf, sizeof(buf),
		 "http://bbs.sysu.edu.cn/bbscon?board=%s&amp;file=%s",
		 bname, fname);
	return buf;
}

char*
threadurl(const char *bname, const char *fname)
{
	static char buf[BUFSIZE];

	snprintf(buf, sizeof(buf),
		 "http://bbs.sysu.edu.cn/bbstcon?board=%s&amp;file=%s",
		 bname, fname);
	return buf;
}


char*
boardurl(const char *bname)
{
	static char buf[BUFSIZE];

	snprintf(buf, sizeof(buf), "http://bbs.sysu.edu.cn/bbsdoc?board=%s", bname);
	return buf;
}

/* rss generator */

int
generate_sample(FILE *fp, const char *bname, const char *fname, int itempos)
{
	FILE *ftext;
	char filename[PATH_MAX + 1];
	char buf[BUFSIZE];
	int num_line;
	int num_blank;
	char *ptr;
	
	snprintf(filename, sizeof(filename), "%s/boards/%s/%s", BBSHOME, bname, fname);
	if ((ftext = fopen(filename, "r")) == NULL) {
		ufprintf(fp, "          <description>%s</description>\n", "");
		return -1;
	}

	ufprintf(fp, "          <description><![CDATA[");
	
	/* 过滤头部信息 */
	while (fgets(buf, sizeof(buf), ftext)) {
		if (buf[0] == '-' && buf[1] == '-')
			goto done;
		if (buf[0] == '\r' || buf[0] == '\n')
			break;
	}


	num_line = 0;
	num_blank = 0;
	while (fgets(buf, sizeof(buf), ftext) && num_line < SAMPLE_LINES) {
		for (ptr = buf; *ptr != '\r' && *ptr != '\n' && *ptr != '\0'; ptr++);
		*ptr = '\0';
		if (strcmp(buf, "--") == 0) break;
		filteransi(buf);

		for (ptr = buf; *ptr == ' '; ptr++);
		if (*ptr == '\0') {
			num_blank++;
			if (num_blank <= 1)
				ufprintf(fp, "%s<br>\n", buf);
		} else {
			num_blank = 0;
			ufprintf(fp, "%s<br>\n", buf);
		}
		num_line++;
	}
done:
	ufprintf(fp, "]]></description>");
	fclose(ftext);
	return 0;
}

#if 0
int
generate_sample(FILE *fp, const char *bname, const char *fname, int itempos)
{
	FILE *ftext;
	char sample[BUFSIZE], buf[BUFSIZE];
	char filename[PATH_MAX + 1];
	char *ptr;
	int len = 0;
	int i;

	snprintf(filename, sizeof(filename), "%s/boards/%s/%s", BBSHOME, bname, fname);
	if ((ftext = fopen(filename, "r")) == NULL) {
		ufprintf(fp, "          <description>%s</description>\n", "");
		return -1;
	}

	memset(sample, 0, sizeof(sample));
	while (fgets(buf, sizeof(buf), ftext) != NULL) {
		if (buf[0] == '-' && buf[1] == '-')
			goto done;
		if (buf[0] == '\r' || buf[0] == '\n')
			break;
	}

	while (fgets(buf, sizeof(buf), ftext) != NULL) {
		char *lptr = &buf[0], *rptr;

		if (buf[0] == '-' && buf[1] == '-')
			break;

		filteransi(buf);

		/* monster: 对合集, 转载信息头的处理...超ugly, 亟待改进 */
		/* pudding: BBS 文章纯文本性质决定的, 貌似没啥好办法 */
		if (!strncmp(buf, "─", 2) || !strncmp(buf, "【", 2))
			continue;
		if (!strncmp(buf, "发信人:", 7) || !strncmp(buf, "标  题:", 7) || !strncmp(buf, "发信站:", 7))
			continue;
		if (strncmp(buf, ": ", 2) == 0) continue;

		while (*lptr != 0) {
			if (*lptr != ' ' && *lptr != '\t' && *lptr !='\r' && *lptr != '\n')
				break;
			lptr++;
		}

		if (*lptr == 0) continue;

		rptr = buf + strlen(buf) - 1;

		while (rptr >= lptr) {
			if (*rptr != ' ' && *rptr != '\t' && *rptr !='\r' && *rptr != '\n')
				break;
			rptr--;
		}

		strncat(sample, lptr, rptr - lptr + 1);
		len += ((rptr - lptr) + 1);
		if (len > SAMPLELEN) break;
	}

	if ((ptr = strstr(sample, "。")) != NULL && (ptr - sample) > SAMPLELENMIN2) {
		*(ptr + 2) = 0;
		goto done;
	}

	if ((ptr = strstr(sample, "！")) != NULL && (ptr - sample) > SAMPLELENMIN2) {
		*(ptr + 2) = 0;
		goto done;
	}

	if ((ptr = strstr(sample, "？")) != NULL && (ptr - sample) > SAMPLELENMIN2) {
		*(ptr + 2) = 0;
		goto done;
	}

	if ((ptr = strrchr(sample, '.')) != NULL && (ptr - sample) > SAMPLELENMIN) {
		*(ptr + 1) = 0;
		goto done;
	}

	if ((ptr = strrchr(sample, '!')) != NULL && (ptr - sample) > SAMPLELENMIN) {
		*(ptr + 1) = 0;
		goto done;
	}

	if ((ptr = strrchr(sample, '?')) != NULL && (ptr - sample) > SAMPLELENMIN) {
		*(ptr + 1) = 0;
		goto done;
	}

	if ((ptr = strrchr(sample, ';')) != NULL && (ptr - sample) > SAMPLELENMIN) {
		*(ptr + 1) = 0;
		goto done;
	}

	if ((ptr = strrchr(sample, ',')) != NULL && (ptr - sample) > SAMPLELENMIN) {
		*(ptr + 1) = 0;
		goto done;
	}
	/* Pudding: 中文整字判断 */
	for (i = 0; sample[i] != '\0' && i < SAMPLELEN; i++) {
		if (sample[i] & 0x80) {
			if (sample[i + 1] == '\0') {
				sample[i] = '\0';
				break;
			} else i++;
		}
	}
	if (sample[i] != '\0') {
		sample[i] = '\0';
		strcat(sample, " ...");
	}
done:
	ufprintf(fp, "          <description>%s</description>\n", fix_xmlstr(sample));
	fclose(ftext);
	return 0;
}
#endif

int
generate_rssitem(FILE *fp, const char *bname, const char *fname, const char *title, const char *author, const time_t filetime, const int itempos, int with_bname)
{
	int r;
	char mytitle[BUFSIZE];
	strcpy(mytitle, title);

	/* Pudding: 中文整字判断 */
	/* fix btitle */
	int titlelen = strlen(mytitle);
	int i;
	for (i = 0; i < titlelen; i++) {
		if (mytitle[i] & 0x80) {
			if (mytitle[i + 1] == '\0') {
				mytitle[i] = '\0';
				break;
			} else i++;
		}
	}

	

	ufprintf(fp, "      <item>\n");
	if (with_bname)
		ufprintf(fp, "          <title>%s - %s - %s</title>\n", bname, fix_xmlstr(mytitle), author);
	else
		ufprintf(fp, "          <title>%s - %s</title>\n", fix_xmlstr(mytitle), author);
/* 	ufprintf(fp, "          <link>%s</link>\n", threadurl(bname, fname)); */
	ufprintf(fp, "          <link>%s</link>\n", fileurl(bname, fname));
	r = generate_sample(fp, bname, fname, itempos);
	ufprintf(fp, "          <pubDate>%s</pubDate>\n", time_tostring(filetime));
	ufprintf(fp, "          <author>%s.bbs@bbs.sysu.edu.cn (%s)</author>\n", author, author);
	ufprintf(fp, "          <comments>%s</comments>\n", threadurl(bname, fname));
	ufprintf(fp, "      <guid isPermaLink=\"false\">%s</guid>\n", fileguid(bname, fname));
	ufprintf(fp, "      </item>\n");

	return r;
}



int
generate_rssfile(const char *bname, const char *btitle, const char *bdesc)
{
	FILE *fp, *fdir;
	struct fileheader fhdr;
	char fname[PATH_MAX + 1], dname[PATH_MAX + 1];
	time_t expiration = time(NULL) - TIMEFACTOR;
	int itempos = 0;
	int numitems;
	struct stat fs;

	snprintf(dname, sizeof(dname), "%s/boards/%s/.DIR", BBSHOME, bname);
	if ((fdir = fopen(dname, "r")) == NULL)
		return -1;
	if (stat(dname, &fs) != 0) return -1;
	numitems = fs.st_size / sizeof(struct fileheader);

	snprintf(fname, sizeof(fname), "%s/%s.xml", RSSDIR, bname);
	if ((fp = fopen(fname, "w")) == NULL) {
		fclose(fdir);
		return -1;
	}

	ufprintf(fp, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");
	ufprintf(fp, "<?xml-stylesheet type=\"text/xsl\" href=\"feedstyle.xml\" ?>\n");
	ufprintf(fp, "<rss version=\"2.0\">\n");
	ufprintf(fp, "  <channel>\n");
	ufprintf(fp, "     <title>%s%s</title>\n", CHANNELPREFIX, btitle);
	ufprintf(fp, "     <link>%s</link>\n", boardurl(bname));
	ufprintf(fp, "     <description>%s</description>\n", bdesc);
	ufprintf(fp, "     <language>%s</language>\n", LANG);
	ufprintf(fp, "     <pubDate>%s</pubDate>\n", time_tostring(time(NULL)));
	ufprintf(fp, "     <ttl>180</ttl>\n");

	/* the image element */
	ufprintf(fp, "     <image>\n");
	ufprintf(fp, "        <url>http://bbs.sysu.edu.cn/icons/title.gif</url>\n");
	ufprintf(fp, "        <title>%s%s</title>\n", CHANNELPREFIX, btitle);
	ufprintf(fp, "        <link>%s</link>\n", boardurl(bname));
	ufprintf(fp, "     </image>\n");

	for (itempos = numitems; itempos > 0; itempos--) {
		if (fseek(fdir, (itempos - 1) * sizeof(struct fileheader), SEEK_SET) != 0) break;
		if (fread(&fhdr, sizeof(fhdr), 1, fdir) != 1) break;
		if ((fhdr.flag & FILE_MARKED || fhdr.flag & FILE_DIGEST ||
		     fhdr.id == atoi(fhdr.filename + 2)) &&
		    fhdr.filetime > expiration && strcmp(fhdr.owner, BBSID)) {
			generate_rssitem(fp, bname, fhdr.filename, fhdr.title, fhdr.owner, fhdr.filetime, itempos, 0);
		}
		if (fhdr.filetime <= expiration) break;
	}

	ufprintf(fp, "   </channel>\n");
	ufprintf(fp, "</rss>\n");

	fclose(fdir);
	fclose(fp);
	return 0;
}

int
collect_dig(const char *bname, const char *btitle)
{
	FILE *fp, *fdir;
	struct fileheader fhdr;
	char dname[PATH_MAX + 1];
	time_t expiration = time(NULL) - 3600 * 24;
	int itempos = 0;
	int numitems;
	struct stat fs;
	digitem_t *item;

	snprintf(dname, sizeof(dname), "%s/boards/%s/.DIR", BBSHOME, bname);
	if ((fdir = fopen(dname, "r")) == NULL)
		return -1;
	if (stat(dname, &fs) != 0) return -1;
	numitems = fs.st_size / sizeof(struct fileheader);

	for (itempos = numitems; itempos > 0; itempos--) {
		if (fseek(fdir, (itempos - 1) * sizeof(struct fileheader), SEEK_SET) != 0) break;
		if (fread(&fhdr, sizeof(fhdr), 1, fdir) != 1) break;
		if ((fhdr.flag & FILE_MARKED) && fhdr.filetime > expiration && 
		     strcmp(fhdr.owner, BBSID)) {
			if (ndigitems >= MAX_NDIGITEMS) continue;
			item = digitem + ndigitems;
			strlcpy(item->bname, bname, sizeof(item->bname));
			strlcpy(item->btitle, btitle, sizeof(item->btitle));
			strlcpy(item->title, fhdr.title, sizeof(item->title));
			strlcpy(item->filename, fhdr.filename, sizeof(item->filename));
			strlcpy(item->owner, fhdr.owner, sizeof(item->owner));
			item->filetime = fhdr.filetime;
			item->itempos = itempos;
			ndigitems++;
		}
		if (fhdr.filetime <= expiration) break;
	}
	fclose(fdir);
	return 0;
}

int
cmp_digitem(const void *item1, const void *item2)
{
	return ((digitem_t *)item2)->filetime - ((digitem_t *)item1)->filetime;
}

int
generate_overall(const char *filename)
{
	FILE *fp = fopen(filename, "w");
	if (!fp) return -1;
	int i;
	digitem_t *item;

	ufprintf(fp, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");
	ufprintf(fp, "<?xml-stylesheet type=\"text/xsl\" href=\"feedstyle.xml\" ?>\n");
	ufprintf(fp, "<rss version=\"2.0\">\n");
	ufprintf(fp, "  <channel>\n");
	ufprintf(fp, "     <title>%s24小时精彩文章</title>\n", CHANNELPREFIX);
	ufprintf(fp, "     <link>http://bbs.sysu.edu.cn/bbsall</link>\n");
	ufprintf(fp, "     <description>24小时精彩文章</description>\n");
	ufprintf(fp, "     <language>%s</language>\n", LANG);
	ufprintf(fp, "     <pubDate>%s</pubDate>\n", time_tostring(time(NULL)));
	ufprintf(fp, "     <ttl>180</ttl>\n");

	/* the image element */
	ufprintf(fp, "     <image>\n");
	ufprintf(fp, "        <url>http://bbs.sysu.edu.cn/icons/title.gif</url>\n");
	ufprintf(fp, "        <title>%s24小时精彩文章</title>\n", CHANNELPREFIX);
	ufprintf(fp, "        <link>http://bbs.sysu.edu.cn/bbsall</link>\n");
	ufprintf(fp, "     </image>\n");
	for (i = 0; i < ndigitems; i++) {
		item = digitem + i;
		generate_rssitem(fp, item->bname, item->filename, item->title, item->owner,
				 item->filetime, item->itempos, 1);
	}
	ufprintf(fp, "   </channel>\n");
	ufprintf(fp, "</rss>\n");
	fclose(fp);
	return 0;
}

int
main(int argc, char **argv)
{
	FILE *fp;
	struct boardheader bhdr;

	if ((fp = fopen(BBSHOME"/.BOARDS", "r")) == NULL)
		return -1;

	gb2utf8 = iconv_open("utf-8", "gbk");
	while (fread(&bhdr, 1, sizeof(struct boardheader), fp) == sizeof(struct boardheader)) {
		if (argc == 1 || !strcmp(argv[1], bhdr.filename)) {
			if ((bhdr.level != 0) && !(bhdr.level & PERM_NOZAP || bhdr.level & PERM_POSTMASK))
				continue;
			if (bhdr.flag & (BRD_RESTRICT | BRD_NOPOSTVOTE))
				continue;

			generate_rssfile(bhdr.filename, &bhdr.title[11], &bhdr.title[11]);
			collect_dig(bhdr.filename, &bhdr.title[11]);
		}
	}
	fclose(fp);

	/* Now sort digitem, and output an overall digest */
	qsort(digitem, ndigitems, sizeof(digitem_t), cmp_digitem);
	generate_overall(RSSDIR"/overall.xml");	
	iconv_close(gb2utf8);
	return 0;
}
