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

void
presskeyfor(char *msg)
{
	showansi = 1;
	//saveline(t_lines - 1, 0);
	move(t_lines - 1, 0);
	clrtoeol();
	outs(msg);
	egetch();
	move(t_lines - 1, 0);
	clrtoeol();
	//saveline(t_lines - 1, 1);
}

void
pressanykey(void)
{
	presskeyfor("\033[m                                \033[5;1;33m按任何键继续...\033[m");
}

void
pressreturn(void)
{
	showansi = 1;
	move(t_lines - 1, 0);
	clrtoeol();
	outs("                              \033[1;33m请按 ◆\033[5;36mEnter\033[m\033[1;33m◆ 继续\033[m");
	while (egetch() != '\n');
	move(t_lines - 1, 0);
	clrtoeol();
	refresh();
}

int
askyn(char *str, int def, int gobottom)
{
	int x, y;
	char realstr[100];
	char ans[3];

	snprintf(realstr, sizeof(realstr), "%s %s", str, (def) ? "(YES/no)? [Y]" : "(yes/NO)? [N]");
	if (gobottom) {
		move(t_lines - 1, 0);
		x = t_lines - 1;
		y = 0;
	} else {
		getyx(&x, &y);
	}
	clrtoeol();
	getdata(x, y, realstr, ans, 2, DOECHO, YEA);

	if (ans[0] != 'Y' && ans[0] != 'y' && ans[0] != 'N' && ans[0] != 'n')
		return def;

	return (ans[0] == 'Y' || ans[0] == 'y') ? YEA : NA;
}

void
printdash(char *msg)
{
	char buf[80], *ptr;
	int len;

	memset(buf, '=', 79);
	buf[79] = '\0';
	if (msg != NULL) {
		len = strlen(msg);
		if (len > 76)
			len = 76;
		ptr = &buf[40 - (len >> 1)];
		ptr[-1] = ' ';
		ptr[len] = ' ';
		strlcpy(ptr, msg, len);
	}
	prints("%s\n", buf);
}

/* 990807.edwardc fix beep sound in bbsd .. */

void
bell(void)
{
	static char sound[1] = { Ctrl('G') };

	send(0, sound, sizeof (sound), 0);
}

void
show_message(char *msg)
{

	move(BBS_PAGESIZE + 2, 0);
	clrtoeol();
	if (msg != NULL) {
		prints("\033[1m%s\033[m", msg);
	}
	refresh();
}

/* freestyler: 校外可以发文, 注释掉了 
#ifdef AUTHHOST
void
count_perm_unauth(void)
{
	if (guestuser) {
		perm_unauth = 0;
	} else {
		if (HAS_ORGPERM(PERM_SYSOP) || HAS_ORGPERM(PERM_OBOARDS) ||
		   HAS_ORGPERM(PERM_ACBOARD) || HAS_ORGPERM(PERM_ACHATROOM))
			perm_unauth = currentuser.userlevel;
		else
			perm_unauth = currentuser.userlevel & UNAUTH_PERMMASK;
	}
}
#endif
*/


/* monster: I move this function out of record.c because I want to make it bbs-independent */
/* 从PASSFILE get_record 到currentuser */
int
set_safe_record(void)
{
        struct userec tmp;
        extern int ERROR_READ_SYSTEM_FILE;
    
        if (get_record(PASSFILE, &tmp, sizeof (currentuser), usernum) == -1) {
                report("set_safe_record: error in record '%4d %12.12s'", usernum,
                        currentuser.userid);
                ERROR_READ_SYSTEM_FILE = YEA;
                abort_bbs();
                return -1;
        }
        currentuser.numposts = tmp.numposts;
        currentuser.userlevel = tmp.userlevel;
        currentuser.numlogins = tmp.numlogins;
        currentuser.stay = tmp.stay;
        currentuser.usertitle = tmp.usertitle;
	memcpy(currentuser.reginfo, tmp.reginfo, sizeof(currentuser.reginfo));

/* freestyler: 校外可以发文了
#ifdef AUTHHOST
	count_perm_unauth();
#endif
*/
        return 0;
}

char datestring[30];

int
getdatestring(time_t now)
{
	struct tm *tm;
	char weeknum[7][3] = { "天", "一", "二", "三", "四", "五", "六" };

	tm = localtime(&now);
	snprintf(datestring, sizeof(datestring), "%4d年%02d月%02d日%02d:%02d:%02d 星期%2s",
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, weeknum[tm->tm_wday]);
	return (tm->tm_sec % 10);
}

int getfilename(char *basedir, char *filename, int flag, unsigned int *id)
{
	char fname[PATH_MAX + 1], ident = 'A';
	int fd = 0, count = 0;
	time_t now = time(NULL);

	while (1) {
		if (count++ > MAX_POSTRETRY)
			return -1;

		snprintf(fname, sizeof(fname), "%s/M.%d.%c", basedir, (int)now, ident);
		if (flag & GFN_LINK) {
			if(link(filename, fname) == 0) {
				unlink(filename);
				break;
			}
		} else {
			if ((fd = open(fname, O_CREAT | O_EXCL | O_WRONLY, 0644)) != -1) {
				if (!(flag & GFN_NOCLOSE)) {
					close(fd);
					fd = 0;
				}
				break;
			}

			if (errno == EEXIST) {   // monster: 仅当文件存在时才重试
				if (!(flag & GFN_SAMETIME) || ident == 'Z') {
					ident = 'A';
					++now;
				} else {
					++ident;
				}
				continue;
			}
		}
		return -1;
	}

	if ((flag & GFN_UPDATEID) && id != NULL)
		*id = now;

	strlcpy(filename, fname, sizeof(fname));
	return fd;
}

int getdirname(char *basedir, char *dirname)
{
	char dname[PATH_MAX + 1], ident = 'A';
	int count = 0;
	time_t now = time(NULL);

	while (1) {
		if (count++ > MAX_POSTRETRY)
			return -1;

		snprintf(dname, sizeof(dname), "%s/D.%d.%c", basedir, (int)now, ident);
		if (f_mkdir(dname, 0744) != -1)
			break;

		if (errno == EEXIST) {
			if (ident == 'Z') {
				ident = 'A';
				++now;
			} else {
				++ident;
			}
			continue;
		}

		return -1;
	}

	strlcpy(dirname, dname, sizeof(dname));
	return 0;
}

time_t calltime = 0;

int
setcalltime(void)
{
        char ans[6];
        int ttt;
        
        move(1, 0);
        clrtoeol();
        getdata(1, 0, "几分钟后要系统提醒你: ", ans, 3, DOECHO, YEA);
        ttt = atoi(ans);
        if (ttt > 0) {  
                calltime = time(NULL) + ttt * 60;
        }
        return 0;
}
 
void
check_calltime(void)
{
        int line;
        
        if (calltime != 0 && time(NULL) >= calltime) {
                if (uinfo.mode == TALK)
                        line = t_lines / 2 - 1;
                else
                        line = 0;
                        
                saveline(line, 0);      /* restore line */
                bell();
                bell();
                bell();
                move(line, 0);
                clrtoeol();   
                prints("\033[1;44;32m系统通告: \033[37m%-65s\033[m",
                       "系统闹钟 铃～～～～～～");
                igetkey();
                move(line, 0);
                clrtoeol();   
                saveline(line, 1);
                calltime = 0;
        }
}

/* monster: mmap optimized version of countln */
/* return the number of lines of fname */
int
countln(char *fname)
{
        int fd;
        char *ptr;
        int count = 0, i;
        struct stat st;
                
        if ((fd = open(fname, O_RDONLY)) < 0)
                return 0;
        if (fstat(fd, &st) < 0 || st.st_size <= 0) {
                close(fd);
                return 0;
        }
        ptr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
        close(fd);
                
        if (ptr == MAP_FAILED) {
                return 0;
	}
 
        for (i = 0; i < st.st_size; i++) {
                if (ptr[i] == 10)
                        count++;
        }
        munmap(ptr, st.st_size);
        
        return count;
}


int
check_host(char *fname, char *name, int nofile)
{
	FILE *list;
	char buf[40], *ptr;

	if ((list = fopen(fname, "r")) != NULL) {
		while (fgets(buf, 40, list)) {
			ptr = strtok(buf, " \n\t\r");
			if (ptr != NULL && *ptr != '#') {
				if (!strcmp(ptr, name)) {
					fclose(list);
					return 1;
				}
				if (ptr[0] == '-' &&
				    !strcmp(name, &ptr[1])) {
					fclose(list);
					return 0;
				}
				if (ptr[strlen(ptr) - 1] == '.' &&
				    !strncmp(ptr, name, strlen(ptr) - 1)) {
					fclose(list);
					return 1;
				}
				if (ptr[0] == '.' &&
				    strlen(ptr) < strlen(name)
				    && !strcmp(ptr,
					       name + strlen(name) -
					       strlen(ptr))) {
					fclose(list);
					return 1;
				}
			}
		}
		fclose(list);
		return 0;
	}
	return nofile;
}

int cmpattfname(void *fname, void *ah) {
	if (!strcmp(((struct attacheader *)ah)->filename, (char *)fname)) return 1;
	return 0;
}


/* 获取附件头信息到 ah */
int getattach(char *board, char *fname, struct attacheader *ah) 
{
	char dirfile[STRLEN];

	snprintf(dirfile, sizeof(dirfile), "attach/%s/.DIR", board);

	return search_record(dirfile, ah, sizeof(*ah), cmpattfname, fname);
}


int is_picture(struct attacheader *attptr) {
	return (!strcasecmp(attptr->filetype, "jpg") || 
		!strcasecmp(attptr->filetype, "bmp") ||
		!strcasecmp(attptr->filetype, "gif") || 
		!strcasecmp(attptr->filetype, "jpeg") ||
		!strcasecmp(attptr->filetype, "png")
	);
}
