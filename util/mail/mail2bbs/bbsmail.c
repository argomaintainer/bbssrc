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
#include "libBBS.h"
#include "bbsmail.h"

#define BUFLEN	1024
#define MAXLEN	64 * 1024 	/* restrict content length within 64K */

void maillog(char *from, char *recv, char *msgid)
{
	FILE *fp;
	time_t now;

	if ((fp = fopen("reclog/imail.log", "a+")) != NULL) {
		now = time(NULL);
		fprintf(fp, "%24.24s %s mailed %s\n", ctime(&now), from, recv);
		fprintf(fp, "Message-ID: %s\n\n", (msgid[0] == 0) ? "None" : msgid);
		fclose(fp);
	}
}

int getline(FILE *fin, char *buf)
{
	int l;

	if (fgets(buf, BUFLEN, fin) == NULL) {
		buf[0] = 0;
		return -1;
	}

	/* remove tailing CRLF */
	l = strlen(buf);
	if (l > 1 && buf[l - 2] == '\r') {
		buf[l - 2] = 0;
		return 0;
	}

	if (buf[l - 1] == '\n')
		buf[l - 1] = 0;

	return 0;
}

int process_header(FILE *fin, FILE *fout, char *owner, char *recv, char *title, char *email)
{
	int len;
	char *ptr, *ptr2;
	char buf[BUFLEN], date[BUFLEN];
	char src[BUFLEN], to[BUFLEN], msgid[BUFLEN] = { 0 };

	while (1) {
		getline(fin, buf);
		if (buf[0] == 0) break;		/* header ends with null line */

		if (!strncasecmp(buf, "From: ", 6)) {
			ptr = buf + 6;
			if (ptr[0] == 0)		/* corrupted header */
				return -1;

			/* extract "owner" */
			if (ptr[0] == '\"') {
				++ptr;
				if ((ptr2 = strchr(ptr, '\"')) == NULL)
					return -1;	/* corrupted header */
			} else {
				if ((ptr2 = strchr(ptr, ' ')) == NULL)
					ptr2 = ptr + strlen(ptr) - 1;
			}
			len = ptr2 - ptr + 1;
			strlcpy(owner, ptr, (len > IDLEN) ? IDLEN + 1 : len);

			if ((ptr = strchr(owner, '@')) != NULL)
				*ptr = '\0';
			if ((ptr = strchr(owner, '.')) != NULL)
				*ptr = '\0';
			strlcat(owner, ".", IDLEN + 1);

			/* extract "email" if available */
			ptr = buf + 6;

			if ((ptr = strchr(ptr, '<')) != NULL) {
				if ((ptr2 = strchr(ptr, '>')) == NULL)
					return -1;	/* corrupted header */
				len = ptr2 - ptr;
				strlcpy(email, ptr + 1, len);
			} else {
				strlcpy(email, buf + 6, BUFLEN);
			}

			continue;
		}

		if (!strncasecmp(buf, "To: ", 4)) {
			/* this part works very similar to the extraction of "owner" */

			ptr = buf + 4;
			if (ptr[0] == 0)		/* corrupted header */
				continue;

			/* extract "reciver" */
			if (ptr[0] != '<') {
				if (ptr[0] == '\"') {
					++ptr;
					if ((ptr2 = strchr(ptr, '\"')) == NULL)
						return -1;	/* corrupted header */
				} else {
					if ((ptr2 = strchr(ptr, ' ')) == NULL)
						ptr2 = ptr + strlen(ptr);
				}
				len = ptr2 - ptr + 1;
				strlcpy(to, ptr, (len > IDLEN) ? IDLEN + 1 : len);
			} else {
				/* extract "reciver" form email address if available */
				if ((ptr = strchr(ptr, '<')) != NULL) {
					if ((ptr2 = strchr(ptr, '>')) == NULL)
						return -1;	/* corrupted header */
					len = ptr2 - ptr;
					strlcpy(to, ptr + 1, len);
				} else {
					return -1;		/* no email address found */
				}
			}

			if ((ptr = strchr(to, '@')) != NULL)
				*ptr = 0;
			if ((ptr = strchr(to, '.')) != NULL)
				*ptr = 0;
			if (strlen(to) > IDLEN)
				return -1;
			strlcpy(recv, to, IDLEN + 1);

			continue;
		}

		if (!strncasecmp(buf, "Subject: ", 9)) {
			strlcpy(title, buf + 9, TITLELEN);
			continue;
		}

		if (!strncasecmp(buf, "Date: ", 6)) {
			strcpy(date, buf + 6);
			continue;
		}

		if (!strncasecmp(buf, "Received: from ", 15)) {
			if ((ptr = strchr(buf + 15, '[')) == NULL)
				continue;
			if ((ptr2 = strchr(ptr, ']')) == NULL)
				continue;
			strlcpy(src, ptr + 1, ptr2 - ptr);
			continue;
		}

		if (!strncasecmp(buf, "Content-Length: ", 16)) {
			/* check whether content length exceeds limitation */
			if (atoi(buf + 15) > MAXLEN) {
				return -1;
			}

			continue;
		}

		if (!strncasecmp(buf, "Message-Id: ", 12)) {
			strlcpy(msgid, buf + 12, sizeof(msgid));
			continue;
		}

		/* skip other fields... */
	}

	if (owner[0] == 0 || recv[0] == 0 || strchr(email, '@') == NULL)
		return -1;

	/* set default title */
	if (title[0] == 0) {
		strcpy(title, "无主题");
	}

	/* write mail header */
	fprintf(fout, "寄信人: %s %s\n", owner, (email[0] == 0) ? email : "");
	fprintf(fout, "标  题: %s\n", title);
	fprintf(fout, "发信站: %s 信差 (%s)\n", BBSNAME, date);
	if (src[0] != 0) fprintf(fout, "来  源: %s\n", src);
	fputc('\n', fout);

	/* append a tailing dot to "owner" */
	len = strlen(owner);
	owner[len] = '.';
	owner[len + 1] = 0;

	maillog(email, recv, msgid);

	return 0;
}

int
main(int argc, char **argv)
{
	char owner[IDLEN + 2] = { 0 };
	char recv[IDLEN + 2] = { 0 };
	char title[TITLELEN] = { 0 };
	char buf[BUFLEN], email[BUFLEN];
	char fname[80];
	int result;
	FILE *fp;

	setgid((gid_t) BBSGID);
	setuid((uid_t) BBSUID);
	chdir(BBSHOME);
	umask(022);

	sprintf(fname, "mail/.tmp/bbsmail.%d", getpid());
	if ((fp = fopen(fname, "w")) == NULL) {
		printf("Cannot open temporary file\r\n");
		return EX_CANTCREAT;
	}

	if ((process_header(stdin, fp, owner, recv, title, email)) == -1) {
		printf("Header corrupted\r\n");
		fclose(fp);
		unlink(fname);
		return EX_DATAERR;
	}

	if (strcmp(recv, "SYSOP") == 0 && strstr(title, "deliver mail check.") != NULL) {
		mailcheck(title, email);
		return 0;
	}

	while (getline(stdin, buf) != -1) {
		fprintf(fp, "%s\n", buf);
	}
	fprintf(fp, "\n--\n\033[m\033[1;31m※ 来源:．%s %s．[FROM: %s]\033[m\n",
		BBSNAME, BBSHOST, BBSHOST);
	fclose(fp);

	result = postmail(fname, recv, title, owner, 0, YEA);
	switch (result) {
	case 0:
		result = EX_OK;
		break;
	case 1:
		printf("User unknown: %s\r\n", recv);
		result = EX_NOUSER;
		break;
	case 3:
	case 4:
	case 6:
		printf("Permission denied\r\n");
		result = EX_NOPERM;
		break;
	default:
		if (result < 0) {
			printf("Cannot create mail file\r\n");
			result = EX_CANTCREAT;
		} else {
			return 255;
		}
	}

	unlink(fname);
	return result;
}
