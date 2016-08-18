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

/*the code below is for post_stat */
struct postrecord {
	char user[IDLEN + 1];
	char title[49];
	unsigned short cnt;
};

struct userrecord {
	char user[IDLEN + 1];
	unsigned num;
	struct userrecord *next;
};

static unsigned short prused = 0;
static unsigned short prsize = 128;
static struct postrecord *precord = NULL;
static struct userrecord *urecord = NULL;

void
init_postrecord(void)
{
	prused = 0;
	prsize = 128;
	precord = NULL;
}

struct postrecord *
countpost(char *user, char *title)
{
	int i, j;

//init precord, prsize, prused
	if (precord == NULL) {
		precord = (struct postrecord *)
		    malloc(prsize * sizeof (struct postrecord));
		if (precord == NULL)
			return NULL;
		memset(precord, 0, prsize * sizeof (struct postrecord));
	}
//search user, title
	if (prused > 0) {
		for (i = 0; i < prused; i++) {
			if ((0 == strcmp(precord[i].user, user)) &&
			    (0 == strncmp(precord[i].title, title, 48))) {
//found, add cnt
				struct postrecord r;

				precord[i].cnt++;
				memcpy(&r, &precord[i], sizeof (r));
				for (j = i - 1; j >= 0; j--) {
					if (precord[j].cnt > r.cnt - 1)
						break;
				}

				if (j + 1 < i) {
					memcpy(&precord[i], &precord[j + 1], sizeof(r));
					memcpy(&precord[j + 1], &r, sizeof(r));
				}

				return precord;
			}
		}
	}
//not found, insert

	if (prused == prsize) {
//full ,need more space
//debug         prints("realloc: %d", prused);pressreturn();
		precord = realloc(precord,
				  (prsize +
				   128) * sizeof (struct postrecord));
		if (precord == NULL)
			return NULL;
		memset(&precord[prsize], 0,
		       128 * sizeof (struct postrecord));
		prsize += 128;
	}
//insert
	strlcpy(precord[prused].user, user, sizeof(precord[prused].user));
	strlcpy(precord[prused].title, title, sizeof(precord[prused].title));
	precord[prused].cnt = 1;
	prused++;
	return precord;
}

/* monster: 统计用户发文数目 */
void
free_urecord(struct userrecord *p)
{
	if (p != NULL) {
		free_urecord(p->next);
		free(p);
		p = NULL;
	}
}

void
countuser(char *user, int num)
{
	struct userrecord *p = urecord, *prev = NULL;

	while (p && strcmp(user, p->user) >= 0) {
		prev = p;
		p = p->next;
	}

	if (prev && !strcmp(user, prev->user)) {
		prev->num += num;
		return;
	}

	if (prev == NULL) {
		if (urecord == NULL) {
			p = urecord = (struct userrecord *) calloc(1, sizeof (struct userrecord));
			p->next = NULL;
		} else {
			p = (struct userrecord *) calloc(1, sizeof (struct userrecord));
			p->next = urecord;
			urecord = p;
		}
	} else {
		p = (struct userrecord *) calloc(1, sizeof (struct userrecord));
		p->next = prev->next;
		prev->next = p;
	}

	strcpy(p->user, user);
	p->num = num;
}

/* Added by betterman 06/11/25 */
int
bm_ann_report(void)
{
        char bname[STRLEN], buf[256], tmpfile[PATH_MAX + 1];
        FILE *ftmp;
        int fd;
        time_t begin, end;
        int year, month, num;
        struct tm time;
        struct anntrace trace;
	struct userrecord *p = NULL;
	int pos, i;
	struct boardheader fh;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("统计精华区整理数字");
	move(2, 0);
	make_blist();
	namecomplete("输入讨论区名称: ", bname);
	if (*bname == '\0') {
		move(3, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	pos = search_record(BOARDS, &fh, sizeof (fh), cmpbnames, bname);
	if (!pos) {
		move(3, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
        snprintf(tmpfile, 256, "0Announce/boards/%s/.trace", bname);
        if ((fd = open(tmpfile, O_RDONLY)) == -1) {
                prints("记录文件打开出错");
                pressreturn();
                return -1;
	}
        getdata(3, 0, "输入起始年份: ", buf, 6, DOECHO, YEA);
        year = atoi(buf);
        if(year < 2000 || year > 2020){
                prints("错误的年份");
                pressreturn();
                close(fd);
                return -1;
        }
        getdata(4, 0, "输入起始月份: ", buf, 6, DOECHO, YEA);
        month = atoi(buf) - 1;
        if(month < 0 || month > 11){
                prints("错误的月份");
                pressreturn();
                close(fd);
                return -1;
        }
        getdata(5, 0, "输入统计月数: ", buf, 6, DOECHO, YEA);
        num = atoi(buf);
        if(num < 1 || num > 12){
                prints("错误的统计月数");
                pressreturn();
                close(fd);
                return -1;
        }
        memset(&time, 0, sizeof(time));
        time.tm_sec = 0;
        time.tm_min = 0;         /* minutes */
        time.tm_hour = 0;        /* hours */
        time.tm_mday = 1;        /* day of the month */
        time.tm_mon = month;         /* month */
        time.tm_year = year - 1900;  /* year */
        time.tm_wday = 0;        /* mktime() ignores this content */
        time.tm_yday = 0;        /* mktime() ignores this content */
        time.tm_isdst = 0;       /* daylight saving time */
        if((begin = mktime(&time)) == -1){
                prints("起始日期错误");
                pressreturn();
                close(fd);
                return -1;
        }
        time.tm_mon = month + num; /* mktime() will normalize the outside legal interval ^_^ */
        if((end = mktime(&time)) == -1){
                prints("结束日期错误");
                pressreturn();
                close(fd);
                return -1;
        }

	free_urecord(urecord);
	urecord = NULL;

	while (read(fd, &trace, sizeof(trace)) == sizeof(trace)) {
               if(trace.otime < begin) continue;
               else if(trace.otime > end )break;
               else countuser(trace.executive, 1);
	}
        close(fd);

	snprintf(tmpfile, sizeof(tmpfile), "tmp/ann_trace.poststat.%d", getpid());
	if ((ftmp = fopen(tmpfile, "w")) == NULL) {
		prints("记录文件打开出错");
		pressreturn();
		return -1;
	}
	fprintf(ftmp, "统计版面：%s\n", bname);
	fprintf(ftmp, "起始时间：%s", ctime(&begin));
	fprintf(ftmp, "中止时间：%s\n", ctime(&end));
	fprintf(ftmp, "操 作 者     记 录 数  \n");
	fprintf(ftmp,
		"=========================================================================\n");

	p = urecord;
	i = 0;
	while (p) {
		fprintf(ftmp, "%-12.12s %5d\n", p->user, p->num);
		p = p->next;
	}
	free_urecord(urecord);
	urecord = NULL;

	fclose(ftmp);

	ansimore(tmpfile, NA);
	if (askyn("要将统计结果寄回到信箱吗", NA, NA) == YEA) {
		char titlebuf[TITLELEN];

		snprintf(titlebuf, sizeof(titlebuf), "%s讨论区精华区操作记录数目统计结果", bname);
		mail_sysfile(tmpfile, currentuser.userid, titlebuf);
	}
	unlink(tmpfile);

	return 0;
        
}

/* monster: this function originally written by windeye, modified by monster */
int
bm_post_stat(void)
{
	char bname[STRLEN], num[8], buf[256], sbrd[STRLEN];
	char sdate[17] = { '\0' }, edate[17] = { '\0' }, suser[20], tmpfile[PATH_MAX + 1];
	int pos, tpos, i, j;
	struct boardheader fh;
	FILE *fp, *ftmp;
	char *phead, *ptail, *ptitle;
	struct postrecord *ppr = NULL;
	struct userrecord *p = NULL;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("统计文章发表数字");
	move(2, 0);
	make_blist();
	namecomplete("输入讨论区名称: ", bname);
	if (*bname == '\0') {
		move(3, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	pos = search_record(BOARDS, &fh, sizeof (fh), cmpbnames, bname);
	if (!pos) {
		move(3, 0);
		prints("错误的讨论区名称");
		pressreturn();
		clear();
		return -1;
	}
	if (fh.flag & ANONY_FLAG) {
		move(3, 0);
		prints("匿名版，还是别查了");
		pressreturn();
		clear();
		return -1;
	}
	getdata(4, 0, "输入位移量(一个估计值，单位为K): ", num, 6, DOECHO, YEA);
	tpos = atoi(num);
	if (tpos <= 0 || tpos > 64 * 1024) {
		prints("错误位移\n");
		pressreturn();
		return -1;
	}
	tpos *= (-1024);

	if ((fp = fopen("reclog/trace", "r")) == NULL) {
		prints("记录文件打开出错");
		pressreturn();
		return -1;
	}

	if (fseek(fp, tpos, SEEK_END) == -1)
		rewind(fp);

	/* skip a line to accurate datetime */
	if (fgets(buf, sizeof(buf), fp) == NULL) {
		prints("记录文件打开出错");
		pressreturn();
		return -1;
	}

	while (fgets(buf, sizeof(buf), fp))
		if (sscanf(buf, "%*s %16c", sdate) > 0)
			break;

	if (sdate[0] == '\0') {
		prints("找不到发文记录");
		pressreturn();
		return -1;
	}

	move(5, 0);
	prints("记录统计开始时间为: %s", sdate);
	move(6, 0);
	if (askyn("是否继续统计？", NA, NA) != YEA) {
		return -1;
	}
	init_postrecord();
	snprintf(sbrd, sizeof(sbrd), "' on '%s'\n", bname);	//fgets的结果可能不包含\n
	while (fgets(buf, 255, fp) != NULL) {
//buf末尾为sbrd
//debug:        if(strstr(buf,bname) == NULL) continue;
//debug:        if((ptail = strchr(buf,'\n')) != NULL) *ptail = '\0';
		ptail = buf + strlen(buf) - strlen(sbrd);
		if (strcmp(ptail, sbrd)) {
			prints("ptail is :%s\nsbrd is :%s\n", ptail, sbrd);
			continue;
		}

		/* monster: parse author & title of article from trace line */
		j = 0;
		while ((buf[j] != 0) && (buf[j] != ' ') && (j < 255))
			j++;
		if (j == 0)
			continue;
		// snprintf(suser, (size_t)(j + 1), "%s", buf);
		strlcpy(suser, buf, j + 1);
		if ((phead = strstr(buf, " posted '")) == NULL)
			continue;
		ptitle = phead + 9;
		ptail = strchr(ptitle, '\'');
		if (ptail == NULL)
			continue;
		else
			*ptail = '\0';
		if (strncmp(ptitle, "Re: ", 4) == 0)
			ptitle += 4;

		ppr = countpost(suser, ptitle);
		if (ppr == NULL) {
			report("bm_post_stat: memory not enough(used %d of %d)!\n",
				prused, (int)sizeof (struct postrecord));
			fclose(fp);
			return 0;
		}

		/* monster: due to Easterly's request */
		sscanf(buf, "%*s %16c", edate);
	}
	fclose(fp);

	snprintf(tmpfile, sizeof(tmpfile), "tmp/trace.poststat.%d", getpid());
	if ((ftmp = fopen(tmpfile, "w")) == NULL) {
		prints("记录文件打开出错");
		pressreturn();
		return -1;
	}

	if (edate[0] == '\0') {
		time_t now = time(NULL);

		snprintf(edate, sizeof(edate), "%16.16s", ctime(&now));
	}

	fprintf(ftmp, "统计版面：%s\n", bname);
	fprintf(ftmp, "起始时间：%s\n", sdate);
	fprintf(ftmp, "中止时间：%s\n\n", edate);
	fprintf(ftmp, "篇 数  发 文 者       标 题\n");
	fprintf(ftmp,
		"=========================================================================\n");

	if (ppr != NULL) {
		for (i = 0; i < prused; i++) {
			fprintf(ftmp, "%5d  %-12.12s   %-48.48s\033[m\n",
				ppr[i].cnt, ppr[i].user, ppr[i].title);
			countuser(ppr[i].user, ppr[i].cnt);
		}
	}

	fprintf(ftmp, "\n\n");
	fprintf(ftmp,
		"发 文 者     篇 数              发 文 者     篇 数\n");
	fprintf(ftmp,
		"=========================================================================\n");

	p = urecord;
	i = 0;
	while (p) {
		fprintf(ftmp, "%-12.12s %s%5d\033[m              ", p->user,
			(p->num < 100) ? "" : "\033[1;31m", p->num);
		if ((++i) % 2 == 0)
			fputc('\n', ftmp);

		p = p->next;
	}
	free_urecord(urecord);
	urecord = NULL;

	fclose(ftmp);

	ansimore(tmpfile, NA);
	if (askyn("要将统计结果寄回到信箱吗", NA, NA) == YEA) {
		char titlebuf[TITLELEN];

		snprintf(titlebuf, sizeof(titlebuf), "%s讨论区文章数目统计结果", bname);
		mail_sysfile(tmpfile, currentuser.userid, titlebuf);
	}
	if (ppr != NULL)
		free(ppr);
	unlink(tmpfile);

	return 0;
}

/*
	 封禁/解封 相关函数
*/

int
addtodeny(char *uident, char *msg, int ischange, int flag, struct fileheader *header)
{
	char buf[50], strtosave[256], buf2[50], tmpbuf[1024];
	char tmpfile[PATH_MAX + 1], orgfile[PATH_MAX + 1], direct[PATH_MAX + 1]; 
	struct denyheader dh;
	time_t nowtime;
	int day, seek, err = 0;
	FILE *rec, *org;

	if (uident[0] == '\0' || getuser(uident, NULL) == 0)
		return -1;

	if (flag & D_FULLSITE) {
		strcpy(direct, "boards/.DENYLIST");
	} else {
		setboardfile(direct, currboard, ".DENYLIST");
	}

	seek = search_record(direct, &dh, sizeof(struct denyheader), denynames, uident);

	if (seek && !ischange) {
		move(3, 0);
		prints(" %s 已经在名单中。", (flag & D_ANONYMOUS) ? "文章作者" : uident);
		pressanykey();
		return -1;
	}

	if (ischange && !seek) {
		move(3, 0);
		prints(" %s 不在名单中。", (flag & D_ANONYMOUS) ? "文章作者" : uident);
		pressanykey();
		return -1;
	}

	getdata(3, 0, "输入说明: ", buf, 40, DOECHO, YEA);
	if (killwordsp(buf) == 0)
		return -1;

	nowtime = time(NULL);
	getdatestring(nowtime);

	do {
		getdata(4, 0, "输入封禁时间[缺省为 1 天, 0 为放弃]: ", buf2, 4, DOECHO, YEA);
		day = (buf2[0] == '\0') ? 1 : atoi(buf2);

		/* monster: 限制版主封人天数 */
		if (!HAS_PERM(PERM_SYSOP | PERM_OBOARDS) && day > 21)
			continue;

		if (day == 0) {
			move(6, 0);
			outs("取消封禁操作");
			pressanykey();
			return -1;
		}
	} while (day < 0 || (!HAS_PERM(PERM_OBOARDS | PERM_SYSOP) && day > 21));

	nowtime += day * 86400;
	getdatestring(nowtime);
	snprintf(strtosave, sizeof(strtosave), "%-12s %-40s %14.14s解封", uident, buf, datestring);
	/* Pudding: 改变封文格式 */
/* 	if (!ischange) { */
	
		sprintf(msg,
			"\n  \033[1;32m%s\033[m 网友: \n\n"
			"    您已经被 \033[1;32m%s\033[m 取消在 \033[1;4;36m%s\033[m %s的发文权利 \033[1;35m%d\033[m 天。\n\n"
			"    您被封禁的原因是: \033[1;4;33m%s\033[m\n\n"
			"    您将在 \033[1;35m%14.14s\033[m 获得解封。如有疑问，请查看\033[1;33m站规版规\033[m相关部分。\n\n"
			"    如有异议，请到 \033[1;4;36mComplain\033[m 版按格式提出申诉，多谢合作。\n\n",
			uident,
			currentuser.userid,
			(flag & D_FULLSITE) ? "本站" : currboard, (flag & D_FULLSITE) ? "" : "版",
			day, buf,
			datestring);
/*		
	} else {
		sprintf(msg,
			"\n  \033[1;4;32m%s\033[m 网友: \n\n"
			"    关于您在 \033[1;4;36m%s\033[m 被取消 \033[1;4;33m发文\033[m 权利问题，现变更如下：\n\n"
			"    封禁的原因： \033[1;4;33m%s\033[m\n\n"
			"    从现在开始，停止该权利时间： \033[1;4;35m%d\033[m 天\n\n"
			"    请您于 \033[1;4;35m%14.14s\033[m 向 \033[1;4;32m%s\033[m 发信申请解封。\n\n",
			uident, (flag & D_FULLSITE) ? "全站" : currboard,
			buf, day, datestring, currentuser.userid);			
	}	
*/
	memset(&dh, 0, sizeof (dh));
	snprintf(dh.filename, sizeof(dh.filename), "D.%d.%c", time(NULL), (getpid() % 26) + 'A');
	setboardfile(tmpfile, currboard, dh.filename);

	if ((rec = fopen(tmpfile, "w")) == NULL) {
		err = 1;
		goto error_process;
	}

	fprintf(rec, "封禁原因：%s\n", buf);
	fprintf(rec, "解封日期：%14.14s\n", datestring);
	fprintf(rec, "执行者：  %s\n", currentuser.userid);

	if ((flag & D_ANONYMOUS) == 0)
		fprintf(rec, "封禁者：  %s\n", uident);

	fprintf(rec, "\n附文：\n\n");

	if (flag & D_NOATTACH) {
		fprintf(rec, "(直接封禁，无附文)\n");
	} else {
		if (ischange) {
			setboardfile(orgfile, currboard, header->filename);
			if ((org = fopen(orgfile, "r")) == NULL) {
				err = 1;
				goto error_process;
			}

			while (fgets(tmpbuf, 1024, org) != NULL) {
				if (strncmp(tmpbuf, "附文", 4) == 0)
					break;
			}

			if (fgets(tmpbuf, 1024, org)) {
				while (fgets(tmpbuf, 1024, org) != NULL)
					fputs(tmpbuf, rec);
			}

			fclose(org);
			unlink(orgfile);
		} else {
			setboardfile(orgfile, currboard, header->filename);
			if ((org = fopen(orgfile, "r")) == NULL) {
				err = 1;
				goto error_process;
			}

			while (fgets(tmpbuf, 1024, org) != NULL)
				fputs(tmpbuf, rec);
		}
	}

      error_process:
	if (rec)
		fclose(rec);

	if (err) {
		unlink(tmpfile);
		presskeyfor("封禁失败, 请与系统维护联系, 按<Enter>继续...");
		return (-1);
	}

	strlcpy(dh.executive, currentuser.userid, sizeof(dh.executive));
	strlcpy(dh.blacklist, uident, sizeof(dh.blacklist));
	strlcpy(dh.title, buf, sizeof(dh.title));
	dh.undeny_time = nowtime;
	dh.filetime = time(NULL);

	if (append_record(direct, &dh, sizeof (dh)) == -1) {
		err = 1;
		goto error_process;
	}

	return 1;
}

int
delfromdeny(char *uident, int flag)
{
	int uid;
	char repbuf[STRLEN], msgbuf[1024];
	struct userec lookupuser;

	if (uident[0] == '\0')
		return -1;

	if ((uid = getuser(uident, NULL)) == 0)
		return (flag & D_IGNORENOUSER) ? 1 : -1;

	if (flag & D_FULLSITE) {
		if (get_record(PASSFILE, &lookupuser, sizeof(struct userec), uid) == -1)
			return -1;

		lookupuser.userlevel |= PERM_POST;
		substitute_record(PASSFILE, &lookupuser, sizeof (struct userec), uid);

		snprintf(repbuf, sizeof(repbuf), "恢复 %s 的全站发文权利", uident);
		securityreport(repbuf);
		snprintf(msgbuf, sizeof(msgbuf), 
				"\n  %s 网友：\n\n"
				"    因封禁时间已过，现恢复您在本站的发文权利。\n\n",
				uident);
		autoreport(repbuf, msgbuf, NA, uident, NULL);
	} else {
		snprintf(repbuf, sizeof(repbuf), "恢复 %s 在 %s 版的发文权利", uident, currboard);
		securityreport(repbuf);
		snprintf(msgbuf, sizeof(msgbuf),
			"\n  \033[1;32m%s\033[m 网友：\n\n"
			"    因封禁时间已过，现恢复您在 \033[1;4;36m%s\033[m 版的发文权利。\n\n",
			uident, currboard);
		autoreport(repbuf, msgbuf, (flag & D_ANONYMOUS) ? NA : YEA, uident, NULL);
		return 1;
	}
	return 1;
}

/* 全站封禁 */
int
add_deny_fullsite(void)
{
	int uid;
	char msgbuf[1024], repbuf[STRLEN], uident[IDLEN + 2];
	struct userec lookupuser;

	stand_title("封禁使用者");
	move(2, 0);
	usercomplete("输入准备加入封禁名单的使用者ID: ", uident);

	if (addtodeny(uident, msgbuf, 0, D_NOATTACH | D_FULLSITE | D_NODENYFILE, NULL) == 1) {
		if ((uid = getuser(uident, &lookupuser)) != 0) {
			lookupuser.userlevel &= ~PERM_POST;
			substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), uid);

			snprintf(repbuf, sizeof(repbuf), "取消 %s 的全站发文权利", uident);
			securityreport(repbuf);
			if (msgbuf[0] != '\0') {
				       autoreport(repbuf, msgbuf, NA, uident, NULL);
			}
		}
	}
	return FULLUPDATE;
}

int
del_deny_fullsite(int ent, struct denyheader *fileinfo, char *direct)
{
	int i;

	move(t_lines - 1, 0);
	if (askyn("确定恢复封禁者的发文权利", NA, NA) == YEA) {
		if ((i = delfromdeny(fileinfo->blacklist, D_FULLSITE | D_IGNORENOUSER)) == 1) {
			delete_record("boards/.DENYLIST", sizeof (struct denyheader), ent);

			if (get_num_records("boards/.DENYLIST", sizeof (struct denyheader)) == 0)
				return DOQUIT;
		}
	}

	return PARTUPDATE;
}

int
change_deny_fullsite(int ent, struct denyheader *fileinfo, char *direct)
{
	char msgbuf[1024], repbuf[STRLEN], uident[IDLEN + 2];

	stand_title("封禁使用者 (修改)");

	if (addtodeny(fileinfo->blacklist, msgbuf, 1,
		      ((digestmode == 10) ? D_ANONYMOUS : 0) | D_FULLSITE | D_NODENYFILE, (struct fileheader *)fileinfo) == 1) {
		delete_record(currdirect, sizeof(struct denyheader), ent);
		snprintf(repbuf, sizeof(repbuf), "修改对 %s 被取消全站发文权利的处理", fileinfo->blacklist);
		securityreport(repbuf);
		if (msgbuf[0] != '\0') {
			autoreport(repbuf, msgbuf, NA, uident, NULL);
		}
	}
	return FULLUPDATE;
}

int
read_denyinfo(int ent, struct denyheader *fileinfo, char *direct)
{
	clear();
	getdatestring(fileinfo->undeny_time);
	prints("封禁原因：%s\n", fileinfo->title);
	prints("解封日期：%14.14s\n", datestring);
	prints("执行者：  %s\n", fileinfo->executive);
	prints("封禁者：  %s\n", fileinfo->blacklist);
	prints("\n附文：\n\n");
	prints("(直接封禁，无附文)\n");
	pressanykey();

	return FULLUPDATE;
}

struct one_key deny_comms[] = {
	{ 'a',		add_deny_fullsite },
	{ 'd', 		del_deny_fullsite },
	{ 'E',		change_deny_fullsite },
	{ 'r',		read_denyinfo },
	{ '\0', 	NULL }
};

void
denytitle(void)
{
	strcpy(genbuf, (chkmail(NA) == YEA) ? "[您有信件]" : BoardName);
	showtitle("全站封禁名单", genbuf);
	prints("离开[\033[1;32m←\033[m,\033[1;32mq\033[m] 选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 阅读[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 封禁[\033[1;32ma\033[m] 修改封禁[\033[1;32mE\033[m] 解除封禁[\033[1;32md\033[m] 求助[\033[1;32mh\033[m] \n");
	prints("\033[1;37;44m 编号   %-12s %6s %-40s[封禁列表] \033[m\n", "封 禁 者", "日  期", " 封 禁 原 因");
}

int
bm_post_perm(void)
{
	digestmode = 11;
	if (get_num_records("boards/.DENYLIST", sizeof(struct denyheader)) > 0) {
		i_read(ADMIN, "boards/.DENYLIST", NULL, NULL, denytitle, readdoent, update_endline, &deny_comms[0],
		       get_records, get_num_records, sizeof(struct denyheader));
	} else {
		modify_user_mode(ADMIN);
		add_deny_fullsite();
	}
	digestmode = NA;
	return 0;
}

int
update_boardlist(int action, slist *list, char *uident)
{
	char boardctl[PATH_MAX + 1];

	sethomefile(boardctl, uident, "board.ctl");
	switch (action) {
	case LE_ADD:
		file_appendline(boardctl, currboard);
		break;
	case LE_REMOVE:
		del_from_file(boardctl, currboard);
		break;
	}

	/* monster: 更新操作者自身的限制版列表 */
	if (!strcmp(uident, currentuser.userid)) {
		load_restrict_boards();
	}
	return YEA;
}

int
control_user(void)
{
	struct boardheader *bp;
	char boardctl[STRLEN];

	bp = getbcache(currboard);
	if ((bp->flag & BRD_RESTRICT) == 0)
		return DONOTHING;

	setboardfile(boardctl, currboard, "board.ctl");
	if (!HAS_PERM(PERM_SYSOP) && (!current_bm || !dashf(boardctl)))
		return DONOTHING;

	listedit(boardctl, "编辑『本版成员』名单", update_boardlist);
	return FULLUPDATE;
}

/* monster: 查询指定用户发文情况 */

static int post_count;

void
poststat(FILE * fp, char *board, char *owner, time_t dt)
{
	time_t at;
	int fd, low, high, mid, stated = NA;
	char filename[PATH_MAX + 1];
	struct fileheader header;
	struct stat st;

	dt = time(NULL) - 86400 * dt;
	snprintf(filename, sizeof(filename), "boards/%s/.DIR", board);

	if ((fd = open(filename, O_RDONLY)) == -1)
		return;

	if (fstat(fd, &st) == -1) {
		close(fd);
		return;
	}

	/* monster: 采用二分检索加快速度 */
	low = 0;
	high = st.st_size / sizeof(header) - 1;
	while (low <= high) {
		mid = (low + high) / 2;
		if (lseek(fd, (off_t)(mid * sizeof(header)), SEEK_SET) == -1)
			return;
		if (read(fd, &header, sizeof (header)) != sizeof (header))
			return;
		at = header.filetime;

		if (dt > at) {
			low = mid + 1;
		} else if (dt < at) {
			high = mid - 1;
		} else {
			break;
		}
	}

	do {
		at = header.filetime;
		if (!strcmp(header.owner, owner) && (at > dt)) {
			if (stated == NA) {
				stated = YEA;
				fprintf(fp, "\n%s: \n", board);
			}
			fprintf(fp, "%5d.  %24.24s  %s\n", ++post_count, ctime(&at), header.title);
		}
	} while (read(fd, &header, sizeof (header)) == sizeof (header));
	close(fd);
}

int
user_poststat(void)
{
	char buf[PATH_MAX + 1], id[IDLEN + 2];
	FILE *fp;
	int i, day;

	stand_title("统计用户发文情况");
	modify_user_mode(ADMIN);

	move(2, 0);
	usercomplete("请输入欲查询的使用者代号: ", id);
	if (id[0] == '\0') {
		clear();
		return 0;
	}

	getdata(3, 0, "统计最近多少天的发文情况(1~7): ", buf, 2, DOECHO, YEA);
	move(5, 0);

	day = buf[0] - '0';
	if (day < 1 || day > 7) {
		prints("很抱歉，系统只能统计1~7天内的发文情况");
		pressanykey();
		return 0;
	}

	snprintf(buf, sizeof(buf), "tmp/user_poststat.%s.%d", id, getpid());
	if ((fp = fopen(buf, "w")) == NULL) {
		prints("统计失败");
		pressanykey();
		return 0;
	}

	post_count = 0;
	prints("统计中，请稍候...");
	refresh();

	fprintf(fp, "以下是%s最近%d天的发文情况，统计结果不包括\n", id, day);
	fprintf(fp, "在限制版以及使用匿名发表的文章：\n");

	resolve_boards();
	for (i = 0; i < numboards; i++) {
		/* monster: 不统计限制版面文章 */
		if ((bcache[i].level & PERM_POSTMASK) ||
		    ((bcache[i].level & ~PERM_POSTMASK) != 0))
			continue;

		poststat(fp, bcache[i].filename, id, day);
	}
	fclose(fp);

	ansimore(buf, NA);
	if (askyn("要将统计结果寄回到信箱吗", NA, NA) == YEA) {
		char titlebuf[TITLELEN];

		snprintf(titlebuf, sizeof(titlebuf), "%s最近%d天的发文情况", id, day);
		mail_sysfile(buf, currentuser.userid, titlebuf);
	}
	unlink(buf);
	return 0;
}

/* monster: 相关主题特殊操作 */

inline int
bmfunc_match(struct fileheader *fileinfo, struct bmfuncarg *arg)
{
	if (arg->flag & LOCATE_THREAD) {
		return (fileinfo->id == arg->id) ? YEA : NA;
	} else if (arg->flag & LOCATE_AUTHOR) {
		return (!strcasecmp(fileinfo->owner, arg->author)) ? YEA : NA;
	} else if (arg->flag & LOCATE_TITLE) {
		return (strstr2(fileinfo->title, arg->title) != NULL) ? YEA : NA;
	} else if ((arg->flag & LOCATE_SELECTED) && (fileinfo->flag & FILE_SELECTED)) {
		fileinfo->flag &= ~FILE_SELECTED;	// remove selection mark
		return YEA;
	} else if (arg->flag & LOCATE_ANY) {
		return YEA;
	}
	return NA;
}

int
bmfunc_del(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;
	char *ptr, filename[PATH_MAX + 1];

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	if (INMAIL(uinfo.mode)) {           // 删除邮件
		if (fileinfo->flag & FILE_MARKED)
			return KEEPRECORD;

		strlcpy(filename, currdirect, sizeof(filename));
		if ((ptr = strrchr(filename, '/')) == NULL)
			return KEEPRECORD;
		strcpy(ptr + 1, fileinfo->filename);
		unlink(filename);
	} else {                        // 删除版面文章
		if (fileinfo->flag & (FILE_MARKED | FILE_DIGEST))
			return KEEPRECORD;

		cancelpost(currboard, currentuser.userid, fileinfo, 0, YEA);
	}

	return REMOVERECORD;
}

int
bmfunc_underline(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	if (fileinfo->flag & FILE_NOREPLY) {
		fileinfo->flag &= ~FILE_NOREPLY;
	} else {
		fileinfo->flag |= FILE_NOREPLY;
	}
	return KEEPRECORD;
}

int
bmfunc_mark(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	if (fileinfo->flag & FILE_MARKED) {
		fileinfo->flag &= ~FILE_MARKED;
	} else {
		fileinfo->flag |= FILE_MARKED;
	}
	return KEEPRECORD;
}

int
bmfunc_cleanmark(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	if (fileinfo->flag & FILE_DIGEST)
		dodigest(0, fileinfo, currdirect, YEA, NA);
	fileinfo->flag &= FILE_ATTACHED;
	return KEEPRECORD;
}

int
bmfunc_import(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg; /* arg->extrarg[0] ==> import_visited */
	char *ptr, filename[PATH_MAX + 1];

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	strlcpy(filename, currdirect, sizeof(filename));
	if ((ptr = strrchr(filename, '/')) == NULL)
		return KEEPRECORD;
	strcpy(ptr + 1, fileinfo->filename);

	if ((*(int *)arg->extrarg[0] == YEA) || !(fileinfo->flag & FILE_VISIT))
		if (ann_import_article(filename, fileinfo->title, fileinfo->owner, fileinfo->flag & FILE_ATTACHED, YEA) == 0)
			fileinfo->flag |= FILE_VISIT;

	return KEEPRECORD;
}

int
bmfunc_savepost(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	ann_savepost(currboard, fileinfo, YEA);
	return KEEPRECORD;
}

/*
   monster: 把指定文章加入到合集中

      用法: addtocombine(文章句柄, 合集句柄, 文章信息);
*/

int
addtocombine(FILE * fp1, FILE * fp, struct fileheader *fileinfo)
{
	int i, len, blankline = 0;
	char temp[LINELEN];

	/* 生成分隔符 */
	snprintf(temp, sizeof(temp), "(%24.24s) ", ctime(&fileinfo->filetime));
	len = strlen(fileinfo->owner) + strlen(temp) + 1;
	fprintf(fp, "\033[1;36m──── \033[37m%s %s\033[36m ", fileinfo->owner, temp);

	if (len % 2 != 0) {
		fputc(' ', fp);
		++len;
	}
	len = 30 - len / 2;
	for (i = 0; i < len; i++)
		fprintf(fp, "─");
	fprintf(fp, "\033[m\n\n");

	/* 过滤信头 */
	while (fgets(temp, sizeof(temp), fp1) && temp[0] != '\n');

	/* 把文章内容(文件头和第一个 '--\n' 之间)加入合集文件中 */
	while (fgets(temp, sizeof(temp), fp1)) {
		if (temp[0] == '\n' || temp[0] == '\r') {
			blankline = 1;
			continue;
		} else {
			if (blankline) fputc('\n', fp);
			blankline = 0;
		}

		if (temp[0] == '-' && temp[1] == '-' && temp[2] == '\n')
			break;
		fputs(temp, fp);
	}
	fputc('\n', fp);

	return 0;
}

int
bmfunc_combine(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;
	struct bmfuncarg *arg = (struct bmfuncarg *)extrarg; /* arg->extrarg[0] ==> count, [1] ==> fp, [2] ==> dele_orig */
	char *ptr, filename[PATH_MAX + 1];
	FILE *fp;

	if (bmfunc_match(fileinfo, arg) == NA)
		return KEEPRECORD;

	strlcpy(filename, currdirect, sizeof(filename));
	if ((ptr = strrchr(filename, '/')) == NULL)
		return KEEPRECORD;
	strcpy(ptr + 1, fileinfo->filename);

	if ((fp = fopen(filename, "r")) != NULL) {
		addtocombine(fp, (FILE *)arg->extrarg[1], fileinfo);
		fclose(fp);
		++(*(int *)arg->extrarg[0]);
	}

	if ((*(int *)arg->extrarg[2] == YEA) && !(fileinfo->flag & FILE_MARKED)) {
		if (INMAIL(uinfo.mode)) {
			unlink(filename);
		} else {
			if (!(fileinfo->flag & FILE_DIGEST)) {
				cancelpost(currboard, currentuser.userid, fileinfo, NA, YEA);
			} else {
				return KEEPRECORD;
			}
		}
		return REMOVERECORD;
	}

	return KEEPRECORD;
}

int
bmfunc(int ent, struct fileheader *fileinfo, char *direct, int dotype)
{
	FILE *fp;
	char ch[7] = { '\0' }, filename[PATH_MAX + 1];
	int dotype2, dele_orig, count = 0, first = 1, end = 999999;
	struct bmfuncarg arg;

	static char *items[5] =
		{ "相同主题", "相同作者", "相关主题" , "区段", "选定文章"};

	static char *items2[6] =
		{ "删除", "锁定", "清除标记", "精华区", "暂存档", "合集" };

	if (!strcmp(currboard, "syssecurity") || !current_bm || digestmode)
		return DONOTHING;

	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);

	if (dotype < 0) {
		getdata(t_lines - 1, 0,	"执行: 1) 相同主题  2) 相同作者  3) 相关主题  4) 区段  0) 取消 [0]: ",
			ch, 2, DOECHO, YEA);
		dotype = ch[0] - '1';
	}

	switch (dotype) {
	case 0:		/* 相同主题 */
		arg.id = fileinfo->id;
		arg.flag = LOCATE_THREAD;
		break;
	case 1:		/* 相同作者 */
		strlcpy(arg.author, fileinfo->owner, sizeof(arg.author));
		arg.flag = LOCATE_AUTHOR;
		break;
	case 2:		/* 相关主题 */
		arg.flag = LOCATE_TITLE;
		break;
	case 3:		/* 区段 */
		arg.flag = LOCATE_ANY;
		break;
	case 4:		/* 选定文章 */
		arg.flag = LOCATE_SELECTED;
		break;
	default:
		goto quit;
	}

	snprintf(genbuf, sizeof(genbuf), "%s (1)删除 (2)锁定 (3)清除标记 (4)精华区 (5)暂存档 (6)合集 ? [0]: ",
		 items[dotype]);
	getdata(t_lines - 1, 0,	genbuf, ch, 2, DOECHO, YEA);
	dotype2 = ch[0] - '1';

	if (dotype2 < 0 || dotype2 > 5)
		goto quit;

	move(t_lines - 1, 0);
	snprintf(genbuf, sizeof(genbuf), "确定要执行%s[%s]吗", items[dotype], items2[dotype2]);
	if (askyn(genbuf, NA, NA) == NA)
		goto quit;

	switch (dotype) {
	case 0:		/* 相同主题 */
		move(t_lines - 1, 0);
		snprintf(genbuf, sizeof(genbuf), "是否从此主题第一篇开始%s (Y)第一篇 (N)目前这一篇", items2[dotype2]);
		if (askyn(genbuf, YEA, NA) == YEA) {
			if ((first = locate_article(direct, fileinfo, ent, LOCATE_THREAD | LOCATE_FIRST, &fileinfo->id)) == -1)
				first = ent;
		} else {
			first = ent;
		}
		break;
	case 1:         /* 相同作者 */
		move(t_lines - 1, 0);
		snprintf(genbuf, sizeof(genbuf), "是否从此作者第一篇开始%s (Y)第一篇 (N)目前这一篇", items2[dotype2]);
		if (askyn(genbuf, YEA, NA) == YEA) {
			if ((first = locate_article(direct, fileinfo, ent, LOCATE_AUTHOR | LOCATE_FIRST, fileinfo->owner)) == -1)
				first = ent;
		} else {
			first = ent;
		}
		break;
	case 2:         /* 相关主题 */
		clear_line(t_lines - 1);
		getdata(t_lines - 1, 0, "请输入主题关键字: ", arg.title, sizeof(arg.title), DOECHO, YEA);
		if (arg.title[0] == '\0') {
			presskeyfor("主题关键字不能为空, 按任意键继续...");
			goto quit;
		}
		break;
	case 3:         /* 区段 */
		clear_line(t_lines - 1);
		getdata(t_lines - 1, 0, "首篇文章编号: ", ch, 7, DOECHO, YEA);
		if ((first = atoi(ch)) <= 0) {
			move(t_lines - 1, 50);
			prints("错误编号...");
			egetch();
			goto quit;
		}
		getdata(t_lines - 1, 25, "末篇文章编号: ", ch, 7, DOECHO, YEA);
		if ((end = atoi(ch)) <= first) {
			move(t_lines - 1, 50);
			prints("错误编号...");
			egetch();
			goto quit;
		}
		break;
	case 4:		/* 选定文章 */
		first = 1;
		end = 999999;
		break;
	}

	switch (dotype2) {
	case 0:         /* 删除 */
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_del, &arg);
		update_lastpost(currboard);
		break;
	case 1:         /* 锁定 (使文章不可回复) */
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_underline, &arg);
		break;
	case 2:		/* 清除标记 */
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_cleanmark, &arg);
		break;
	case 3:		/* 精华区 */
		if (ann_loadpaths() == -1) {
			presskeyfor("对不起, 您没有设定丝路或丝路设定有误. 请用 f 设定丝路.");
		} else {
			int import_visited;

			import_visited = askyn("是否收录曾收入精华区的文章", YEA, YEA);
			arg.extrarg[0] = &import_visited;

			update_ainfo_title(YEA);
			if (process_records(direct, sizeof(struct fileheader), first, end, bmfunc_import, &arg) == -1)
				presskeyfor("操作失败, 请与系统维护联系, 按任意键继续...");
			update_ainfo_title(NA);
		}
		break;
	case 4:		/* 暂存档 */
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_savepost, &arg);
		break;
	case 5:		/* 合集 */
		clear_line(t_lines - 1);
		dele_orig = askyn("是否要删除原文？", NA, NA);

		snprintf(filename, sizeof(filename), "boards/.tmp/combine.%d.A", time(NULL));
		if ((fp = fopen(filename, "w")) == NULL) {
			presskeyfor("操作失败, 请与系统维护联系, 按任意键继续...");
			goto quit;
		}

		switch (dotype) {
		case 0:		/* 相同主题 */
			if ((fileinfo->title[0] == 'R' || fileinfo->title[0] == 'r') &&
			     fileinfo->title[1] == 'e' && fileinfo->title[2] == ':' &&
			     fileinfo->title[3] == ' ') {
				snprintf(save_title, sizeof(save_title), "【合集】%s", fileinfo->title + 4);
			} else {
				snprintf(save_title, sizeof(save_title), "【合集】%s", fileinfo->title);
			}
			break;
		case 1:         /* 相同作者 */
			snprintf(save_title, sizeof(save_title), "【合集】%s的文章", arg.author);
			break;
		case 2:         /* 相关主题 */
			snprintf(save_title, sizeof(save_title), "【合集】%s", arg.title);
			break;
		case 3:         /* 区段 */
			snprintf(save_title, sizeof(save_title), "【合集】%d - %d", first, end);
			break;
		case 4:		/* 选定文章 */
			snprintf(save_title, sizeof(save_title), "【合集】%s版选定文章", currboard);
			break;
		}

		arg.extrarg[0] = &count;
		arg.extrarg[1] = fp;
		arg.extrarg[2] = &dele_orig;
		process_records(direct, sizeof(struct fileheader), first, end, bmfunc_combine, &arg);

		fprintf(fp, "\033[m\n--\n\033[1;%dm※ 来源:. %s %s. [FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, currentuser.lasthost);
		fclose(fp);

		if (count > 0) {
			if (strncmp(save_title, "【合集】【合集】", 16) == 0) {
				postfile(filename, currboard, save_title + 8, 2);
				report("posted '%s' on '%s'", save_title + 8, currboard);
			} else {
				postfile(filename, currboard, save_title, 2);
				report("posted '%s' on '%s'", save_title, currboard);
			}
		}
		update_lastpost(currboard);
		update_total_today(currboard);
		unlink(filename);
		break;
	}

	return DIRCHANGED;

quit:
	saveline(t_lines - 1, 1);
	return DONOTHING;
}

int
bmfuncs(int ent, struct fileheader *fileinfo, char *direct)
{
	return bmfunc(ent, fileinfo, direct, -1);
}
