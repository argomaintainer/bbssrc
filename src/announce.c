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

/* written & mantained by monster (monster@marco.zsu.edu.cn) */

#include "bbs.h"

struct anninfo ainfo;

static int sauthor = 1;
static struct annpath paths[8];

/* wrapper of getdata */
int
a_prompt(char *pmt, char *buf, int len, int clearlabel)
{
	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, pmt, buf, len, DOECHO, clearlabel);
	my_ansi_filter(buf);
	saveline(t_lines - 1, 1);
	return (buf[0] == '\0') ? -1 : 0;
}

static int
real_check_annmanager(char *direct)
{
	char *p;
	char cdir[STRLEN];
	struct boardheader *bp;

	/* 不能修改动态生成的条目 */
	if (direct[0] == '@')
		return NA;

	/* SYSOP和精华区主管可以编辑任意条目 */
	if (HAS_PERM(PERM_SYSOP) || HAS_PERM(PERM_ANNOUNCE))
		return YEA;

	/* 检查是否个人文集所有者 */
	if (HAS_PERM(PERM_PERSONAL)) {
		snprintf(cdir, sizeof(cdir), "0Announce/personal/%c/%s/", mytoupper(currentuser.userid[0]),
			 currentuser.userid);

		if (strncmp(direct, cdir, strlen(cdir)) == 0)
			return YEA;
	}

	/* 检查是否精华区所在版的版主 */
	if (strncmp(direct, "0Announce/boards/", 17) != 0)
		return NA;

	if ((p = strchr(direct + 17, '/')) == NULL)
		return NA;

	strlcpy(cdir, direct + 17, p - direct - 16);
	if ((bp = getbcache(cdir)) == NULL)
		return NA;
	return check_bm(currentuser.userid, bp->BM);
}

int
check_annmanager(char *direct)
{
	char resolved[PATH_MAX + 1];

	if (real_check_annmanager(direct) == YEA)
		return YEA;

	/* 对direct的真实路径再做一次检查 */
	if (realpath(direct, resolved) != NULL) {
		return real_check_annmanager(resolved + strlen(BBSHOME) + 1);
	}

	return NA;
}

int
get_annjunk(char *direct)
{
	char *ptr;

	/* 确定记录文件名 */
	if (strncmp(ainfo.basedir, "0Announce/boards/", 17) == 0) {
		if ((ptr = strchr(ainfo.basedir + 18, '/')) == NULL) {
			strcpy(direct, ainfo.basedir);
		} else {
			strlcpy(direct, ainfo.basedir, ptr - ainfo.basedir + 1);
		}

		strcat(direct, "/junk");
	} else if (strncmp(ainfo.basedir, "0Announce/personal/", 19) == 0) {
		if (!isalpha(ainfo.basedir[19]) || ainfo.basedir[20] != '/')
			return -1;

		if ((ptr = strchr(ainfo.basedir + 21, '/')) == NULL) {
			strcpy(direct, ainfo.basedir);
		} else {
			strlcpy(direct, ainfo.basedir, ptr - ainfo.basedir + 1);
		}

		strcat(direct, "/junk");
	}
	if(dashf(direct))
		return 0;
	if(!dash(direct))
		f_mkdir(direct , 0755);
	return 1;
}


int
get_atracename(char *fname)
{
	char *ptr;

	/* 确定记录文件名 */
	if (strncmp(ainfo.basedir, "0Announce/boards/", 17) == 0) {
		if ((ptr = strchr(ainfo.basedir + 18, '/')) == NULL) {
			strcpy(fname, ainfo.basedir);
		} else {
			strlcpy(fname, ainfo.basedir, ptr - ainfo.basedir + 1);
		}

		strcat(fname, "/.trace");
		return 0;
	} else if (strncmp(ainfo.basedir, "0Announce/personal/", 19) == 0) {
		if (!isalpha(ainfo.basedir[19]) || ainfo.basedir[20] != '/')
			return -1;

		if ((ptr = strchr(ainfo.basedir + 21, '/')) == NULL) {
			strcpy(fname, ainfo.basedir);
		} else {
			strlcpy(fname, ainfo.basedir, ptr - ainfo.basedir + 1);
		}

		strcat(fname, "/.trace");
		return 0;
	}

	return -1;
}

/* 精华区操作记录 */
void
atrace(int operation, char *info0, char *info1)
{
	int fd;
	char fname[PATH_MAX + 1];
	struct anntrace trace;

	/* 只记录对版面精华区和个人文集的操作记录 */
	if (get_atracename(fname) == -1)
		return;

	if ((fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, 0644)) > 0) {
		memset(&trace, 0, sizeof(trace));
		trace.operation = operation;
		trace.otime = time(NULL);
		strlcpy(trace.executive, currentuser.userid, sizeof(trace.executive));
		strlcpy(trace.location, ainfo.title, sizeof(trace.location));
		if (info0 != NULL) strlcpy(trace.info[0], info0, sizeof(trace.info[0]));
		if (info1 != NULL) strlcpy(trace.info[1], info1, sizeof(trace.info[1]));
		write(fd, &trace, sizeof(trace));
		close(fd);
	}
}

void
atracetitle(void)
{
	if (chkmail(NA)) {
		prints("\033[1;33;44m[精华区]\033[1;37m%*s[您有信件，按 M 看新信]%*s\033[m\n", 19, " ", 29, " ");
	} else {
		prints("\033[1;33;44m[精华区]\033[1;37m%*s%s%*s\033[m\n", 25, " ", "精华区操作记录", 32, " ");
	}

	clrtoeol();
	prints("离开[\033[1;32m←\033[m,\033[1;32mq\033[m] 选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 阅读[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 清空[\033[1;32mE\033[m]\n");
	prints("\033[1;37;44m 编号   %-12s %6s %-50s \033[m\n", "执 行 者", "日  期", " 操  作");
}

static inline char *
get_atracedesc(struct anntrace *ent)
{
	switch (ent->operation) {
	case ANN_COPY:
		return "复制条目";
	case ANN_CUT:
		return "剪切条目";
	case ANN_MOVE:
		return ent->info[0];
	case ANN_EDIT:
		return "编辑文章";
	case ANN_CREATE:
		return ent->info[0];
	case ANN_DELETE:
		return "删除条目";
	case ANN_CTITLE:
		return "更改标题";
	case ANN_ENOTES:
		return "编辑备忘录";
	case ANN_DNOTES:
		return "删除备忘录";
	case ANN_INDEX:
		return "生成精华区索引";
	}

	return "";
}

char *
atracedoent(int num, void *ent_ptr)
{
	static char buf[128];
	struct anntrace *ent = (struct anntrace *)ent_ptr;

#ifdef COLOR_POST_DATE
	struct tm *mytm;
	char color[8] = "\033[1;30m";
#endif

	#ifdef COLOR_POST_DATE

	mytm = localtime(&ent->otime);
	color[5] = mytm->tm_wday + 49;

	snprintf(buf, sizeof(buf), " %4d   %-12.12s %s%6.6s\033[m  ● %-.45s ",
		 num, ent->executive, color, ctime(&ent->otime) + 4, get_atracedesc(ent));

	#else

	snprintf(buf, sizeof(buf), " %4d   %-12.12s %6.6s  ● %-.45s ",
		 num, ent->executive, ctime(&ent->otime) + 4, get_atracedesc(ent));

	#endif

	return buf;
}

int
atrace_empty(int ent, struct anntrace *traceinfo, char *direct)
{
	char fname[PATH_MAX + 1];

        /* 只有SYSOP和精华区主管可以清空精华区操作记录 */
        if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ANNOUNCE))
                return DONOTHING;

	if (get_atracename(fname) == -1)
		return DONOTHING;

	if (askyn("确定要清空精华区操作记录吗", NA, YEA) == NA)
		return PARTUPDATE;

	snprintf(genbuf, sizeof(genbuf), "位置: %s", fname);
	securityreport2("精华区操作记录被清空", NA, genbuf);

	unlink(fname);
	return NEWDIRECT;
}

int
atrace_read(int ent, struct anntrace *traceinfo, char *direct)
{
	clear();

	/* 基本信息 */
	getdatestring(traceinfo->otime);
	prints("操作描述: %s\n", get_atracedesc(traceinfo));
	prints("操作位置: %s\n", traceinfo->location);
	prints("操作日期: %s\n", datestring);
	prints("执行者:   %s\n", traceinfo->executive);

	/* 附加信息 */
	if (traceinfo->operation != ANN_DNOTES && traceinfo->operation != ANN_ENOTES)
		prints("\n附加信息: \n\n");

	switch (traceinfo->operation) {
	case ANN_COPY:
	case ANN_CUT:
		prints("条目标题: %s\n", traceinfo->info[0]);
		prints("原位置:   %s\n", traceinfo->info[1]);
		break;
	case ANN_EDIT:
	case ANN_DELETE:
		prints("条目标题: %s\n", traceinfo->info[0]);
		break;
	case ANN_MOVE:
	case ANN_CREATE:
		prints("条目标题: %s\n", traceinfo->info[1]);
		break;
	case ANN_CTITLE:
		prints("原标题:   %s\n", traceinfo->info[0]);
		prints("新标题:   %s\n", traceinfo->info[1]);
		break;
	}

	clear_line(t_lines - 1);
	outs("\033[1;31;44m[精华区操作记录]  \033[33m │ 结束 Q,← │ 上一项 U,↑│ 下一项 <Space>,↓ │          \033[m");

	switch (egetch()) {
		case KEY_DOWN:
		case ' ':
		case '\n':
			return READ_NEXT;
		case KEY_UP:
		case 'u':
		case 'U':
			return READ_PREV;
	}

	return FULLUPDATE;
}

struct one_key atrace_comms[] = {
	{ 'E',		atrace_empty	},
	{ 'f',          t_friends       },
	{ 'o',          fast_cloak      },
	{ 'r',          atrace_read     },
	{ Ctrl('V'), 	x_lockscreen_silent },
	{ '\0',         NULL }
};

void
anntitle(void)
{
	int len;

	static char *satype[3] = { "整  理", "作  者", "      " };

	if (chkmail(NA)) {
		prints("\033[1;33;44m[精华区]\033[1;37m%*s[您有信件，按 M 看新信]%*s\033[m\n", 19, " ", 28, " ");
	} else {
		len = 32 - strlen(ainfo.title) / 2;
		prints("\033[1;33;44m[精华区]\033[1;37m%*s%s%*s\033[m\n", len, " ", ainfo.title, 71 - strlen(ainfo.title) - len, " ");
	}

	prints("           \033[1;32m F\033[37m 寄回自己的信箱  \033[32m↑↓\033[37m 移动 \033[32m → <Enter> \033[37m读取 \033[32m ←,q\033[37m 离开\033[m\n");
	prints("\033[1;44;37m 编号   %-20s                         %s          编辑日期  \033[m\n",
	       "[类别] 标    题", satype[sauthor]);
}

char *
anndoent(int num, void *ent_ptr)
{
	struct tm *pt;
	static char buf[128];
	struct annheader *ent = (struct annheader *)ent_ptr;
	char itype;
	int tnum;

	static char *ctype[5] = {
		"\033[1;36m文件\033[m", "\033[1;37m目录\033[m",
		"\033[1;32m衔接\033[m", "\033[1;35m留言\033[m",
		"\033[1;31m未知\033[m"
	};

	if (ent->flag & ANN_FILE) {
		tnum = 0;
	} else if (ent->flag & ANN_DIR) {
		tnum = 1;
	} else if ((ent->flag & ANN_LINK) || (ent->flag & ANN_RLINK)) {
		tnum = 2;
	} else if (ent->flag & ANN_GUESTBOOK) {
		tnum = 3;
	} else {
		tnum = 4;
	}

	itype = ((ainfo.manager == YEA) && (ent->flag & ANN_SELECTED)) ? '$' : ' ';

	// monster: Here we assume time_t to be 32bit integer. The code may break
	//          if it's not the case. Note standard did not specify type & range
	//          for time_t.
	time_t mtime = ent->mtime;
	pt = localtime(&mtime);
	ent->mtime = mtime;

	if (sauthor != 2 && ent->filename[0] != '@') {
		snprintf(buf, sizeof(buf), "%5d %c [%s] %-37.37s %-12s [\033[1;37m%04d\033[m.\033[1;37m%02d\033[m.\033[1;37m%02d\033[m]",
			  num, itype, ctype[tnum], ent->title, (sauthor == NA) ? ent->editor : ent->owner,
			  pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday);
	} else {
		snprintf(buf, sizeof(buf), "%5d %c [%s] %-50.50s [\033[1;37m%04d\033[m.\033[1;37m%02d\033[m.\033[1;37m%02d\033[m]",
			  num, itype, ctype[tnum], ent->title,
			  pt->tm_year + 1900, pt->tm_mon + 1, pt->tm_mday);
	}

	return buf;
}

int
ann_switch_sauthor(int ent, struct annheader *anninfo, char *direct)
{
	sauthor = (sauthor + 1) % 3;
	return FULLUPDATE;
}

static int
ann_add_file(char *filename, char *title, char *owner, char *direct, int flag, int remove)
{
	struct annheader fileinfo;
	char fname[PATH_MAX + 1];
	char *ptr;

	/* 检查权限 */
	if (ainfo.manager != YEA && !((ainfo.flag & ANN_GUESTBOOK) && (HAS_ORGPERM(PERM_POST))))
		return -2;

	/* 初始化 */
	memset(&fileinfo, 0, sizeof(fileinfo));

	/* 设定参数 */
	if (title == NULL || title[0] == '\0') {
		a_prompt("标题: ", fileinfo.title, sizeof(fileinfo.title), YEA);
	} else {
		strlcpy(fileinfo.title, title, sizeof(fileinfo.title));
	}
	if (fileinfo.title[0] == '\0')
		return -2;

	if (getfilename(ainfo.basedir, fname, 0, NULL) == -1)
		return -1;

	if (filename != NULL && filename[0] != '\0') {
		if (f_cp(filename, fname, O_TRUNC) != 0)
			return -1;
	} else {
		/* 编辑文章 */
		if (vedit(fname, EDIT_MODIFYHEADER) == -1)
			return -2;
	}

	if ((ptr = strrchr(fname, '/')) == NULL)
		return -1;
	strlcpy(fileinfo.filename, ptr + 1, sizeof(fileinfo.filename));

	fileinfo.flag = ANN_FILE | flag;
	fileinfo.mtime = time(NULL);
	strlcpy(fileinfo.owner, owner, sizeof(fileinfo.owner));
	strlcpy(fileinfo.editor, currentuser.userid, sizeof(fileinfo.editor));

	/* 添加记录 */
	if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1)
		return -1;

	if (remove == YEA)
		unlink(filename);

	atrace(ANN_CREATE, "新增文件", fileinfo.title);
	return 0;
}

int
ann_import_article(char *fname, char *title, char *owner, int attached, int batch)
{
	char direct[PATH_MAX + 1];

	 /* 批量操作不重新装入丝路 */
	if (batch == NA && ann_loadpaths() == -1)
		return -1;

	snprintf(direct, sizeof(direct), "%s/.DIR", paths[0].path);
	ainfo.manager = check_annmanager(direct);
	strlcpy(ainfo.basedir, paths[0].path, sizeof(ainfo.basedir));
	strlcpy(ainfo.title, paths[0].title, sizeof(ainfo.title));	

	return ann_add_file(fname, title, owner, direct, attached ? ANN_ATTACHED : 0, NA);
}

int
ann_add_article(int ent, struct annheader *anninfo, char *direct)
{
	if (ann_add_file(NULL, NULL, currentuser.userid, direct, 0, NA) == -1)
		presskeyfor("操作失败...");

	return FULLUPDATE;
}

static int
add_copypaste_record(int fd, struct annheader *anninfo, int copymode)
{
	struct anncopypaste rec;

	/* 记录相关信息 */
	strlcpy(rec.basedir, ainfo.basedir, sizeof(rec.basedir));
	strlcpy(rec.location, ainfo.title, sizeof(rec.location));
	memcpy(&rec.fileinfo, anninfo, sizeof(rec.fileinfo));
	rec.copymode = copymode;
	rec.fileinfo.flag &= ~ANN_SELECTED;

	/* 写入记录 */
	return write(fd, &rec, sizeof(rec));
}

int
ann_copy(int ent, struct annheader *anninfo, char *direct)
{
	int fd;
	char fname[PATH_MAX + 1];

	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0')
		return DONOTHING;

	if (anninfo->flag != ANN_FILE && anninfo->flag != ANN_DIR) {
		presskeyfor("只能对目录/文件操作");
		return PARTUPDATE;
	}

	sethomefile(fname, currentuser.userid, "copypaste");
	if ((fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
		goto error_process;

	f_exlock(fd);
	if (add_copypaste_record(fd, anninfo, ANN_COPY) == -1) {
		close(fd);
		goto error_process;
	}

	close(fd);
	presskeyfor("档案标识完成. (注意! 粘贴文章后才能将文章 delete!)");
	return PARTUPDATE;

error_process:
	if (fd != -1) close(fd);
	presskeyfor("操作失败...");
	return PARTUPDATE;
}

int
ann_cut(int ent, struct annheader *anninfo, char *direct)
{
	int fd;
	char fname[PATH_MAX + 1];

	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0')
		return DONOTHING;

	if (anninfo->flag != ANN_FILE && anninfo->flag != ANN_DIR) {
		presskeyfor("只能对目录/文件操作");
		return PARTUPDATE;
	}

	sethomefile(fname, currentuser.userid, "copypaste");
	if ((fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
		goto error_process;

	f_exlock(fd);
	if (add_copypaste_record(fd, anninfo, ANN_CUT) == -1) {
		close(fd);
		goto error_process;
	}

	close(fd);
	presskeyfor("档案标识完成. (注意! 粘贴文章后才能将文章 delete!)");
	return PARTUPDATE;

error_process:
	if (fd != -1) close(fd);
	presskeyfor("操作失败...");
	return PARTUPDATE;
}

static int
cmpafilename(void *filename_ptr, void *fileinfo_ptr)
{
	char *filename = (char *)filename_ptr;
	struct annheader *fileinfo = (struct annheader *)fileinfo_ptr;

	return (!strcmp(filename, fileinfo->filename)) ? YEA : NA;
}

int
ann_paste(int ent, struct annheader *anninfo, char *direct)
{
	time_t now;
	int id, fd, count = 0;
	char fname[PATH_MAX + 1], src[PATH_MAX + 1], dst[PATH_MAX + 1], cmdbuf[2 * PATH_MAX + 15];
	char olddirect[PATH_MAX + 1];
	struct anncopypaste rec;
	struct annheader fileinfo;
	struct stat st;

	if (ainfo.manager != YEA)
		return DONOTHING;

	sethomefile(fname, currentuser.userid, "copypaste");
	if ((fd = open(fname, O_RDWR)) == -1)
		return DONOTHING;

	f_exlock(fd);		/* monster: caution, not a wise solution, as the copy process may take long time */
	while (read(fd, &rec, sizeof(rec)) == sizeof(rec)) {
		snprintf(src, sizeof(src), "%s/%s", rec.basedir, rec.fileinfo.filename);
		snprintf(dst, sizeof(dst), "%s/%s", ainfo.basedir, rec.fileinfo.filename);

		/* 避免产生回路 */
		if (strstr(dst, src) == dst)
			continue;

		/* 保证源文件/目录存在 */
		if (!dash(src)) {
			clear();
			continue;
		}

		switch (rec.copymode) {
		case ANN_COPY:  /* 拷贝 */
			if (rec.fileinfo.filename[0] != '@') {
				now = time(NULL);
				while (1) {
					snprintf(rec.fileinfo.filename + 2, sizeof(rec.fileinfo.filename) - 2, "%d.A", (int)(now++));
					snprintf(dst, sizeof(dst), "%s/%s", ainfo.basedir, rec.fileinfo.filename);

					if (stat(dst, &st) == -1) {
						snprintf(cmdbuf, sizeof(cmdbuf), "/bin/cp -p -f -R %s %s", src, dst);
						system(cmdbuf);
						break;
					}
				}
			}

			if (append_record(currdirect, &rec.fileinfo, sizeof(rec.fileinfo)) != -1) {
				atrace(ANN_COPY, rec.fileinfo.title, rec.location);
				++count;
			}
			break;
		case ANN_CUT:   /* 剪切 */
			if (rec.fileinfo.filename[0] != '@') {
				strlcpy(fname, rec.fileinfo.filename, sizeof(fname));
				if (rename(src, dst) == -1) {
					now = time(NULL);
					while (1) {
						snprintf(rec.fileinfo.filename + 2, sizeof(rec.fileinfo.filename) - 2, "%d.A", (int)(now++));
						snprintf(dst, sizeof(dst), "%s/%s", ainfo.basedir, rec.fileinfo.filename);

						/* 更名前必须保证目标不存在, 否则目标文件会被覆盖 */
						if (stat(dst, &st) == -1)
							if (rename(src, dst) == 0)
								break;
					}
				}
			}

			if (append_record(currdirect, &rec.fileinfo, sizeof(rec.fileinfo)) != -1) {
				atrace(ANN_CUT, rec.fileinfo.title, rec.location);
				++count;
			}

			snprintf(olddirect, sizeof(olddirect), "%s/.DIR", rec.basedir);
			if ((id = search_record(olddirect, &fileinfo, sizeof(fileinfo), cmpafilename, fname)) > 0)
				delete_record(olddirect, sizeof(fileinfo), id);
			break;
		}
	}

	close(fd);
	unlink(fname);

	if (count == 0)
		presskeyfor("操作失败");

	return PARTUPDATE;
}

static int
ann_rangecheck(void *rptr, void *extrarg)
{
	struct annheader *anninfo = (struct annheader *)rptr;
	char src[PATH_MAX + 1];
	char dst[PATH_MAX+1];	
	char cmdbuf[PATH_MAX + 1];

	if (anninfo->flag & (ANN_DIR | ANN_GUESTBOOK))
		return -1;

	get_annjunk(genbuf);
	snprintf(dst, sizeof(dst), "%s/%d_%s_%s", genbuf, time(NULL), currentuser.userid, anninfo->title);
	snprintf(src, sizeof(src), "%s/%s", ainfo.basedir, anninfo->filename);

        if (rename(src, dst) != 0) {
	  snprintf(cmdbuf, sizeof(cmdbuf), "/bin/cp -p -f -R %s %s", src, dst);
	  system(cmdbuf);
	  f_rm(src);
	}

	atrace(ANN_DELETE, anninfo->title, NULL);
	return (dash(src)) ? KEEPRECORD : REMOVERECORD;
}

int
ann_delete_range(int ent, struct annheader *anninfo, char *direct)
{
	char num[8];
	int inum1, inum2, result;
	return -1;
	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0' && anninfo->title[0] == '\0')
		return DONOTHING;

	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "首篇文章编号: ", num, 7, DOECHO, YEA);
	inum1 = atoi(num);
	if (inum1 <= 0)
		goto error_range;

	getdata(t_lines - 1, 25, "末篇文章编号: ", num, 7, DOECHO, YEA);
	inum2 = atoi(num);
	if (inum2 < inum1 + 1)
		goto error_range;

	move(t_lines - 1, 50);
	if (askyn("确定删除", NA, NA) == YEA) {
		result = process_records(currdirect, 128, inum1, inum2, ann_rangecheck, NULL);
		fixkeep(direct, inum1, inum2);

		if (result == -1)
			goto error_process;
	}

	return NEWDIRECT;

error_range:
	move(t_lines - 1, 50);
	clrtoeol();
	prints("区间错误...");
	egetch();
	return NEWDIRECT;

error_process:
	presskeyfor("删除失败...");
	return NEWDIRECT;
}

int
ann_delete_item(int ent, struct annheader *anninfo, char *direct)
{
	char src[PATH_MAX + 1];
	char dst[PATH_MAX+1];	
	char cmdbuf[PATH_MAX + 1];

	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0' && anninfo->title[0] == '\0')
		return DONOTHING;

	clear_line(t_lines - 1);
	if (anninfo->flag & ANN_DIR) {
		if (askyn("删除整个子目录, 别开玩笑哦, 确定吗", NA, YEA) == NA)
			return NEWDIRECT;
	} else {
		if (askyn("删除此档案, 确定吗", NA, YEA) == NA)
			return NEWDIRECT;
	}

	get_annjunk(genbuf);
	snprintf(dst, sizeof(dst), "%s/%d_%s_%s", genbuf, time(NULL), currentuser.userid, anninfo->title);
	snprintf(src, sizeof(src), "%s/%s", ainfo.basedir, anninfo->filename);
	snprintf(cmdbuf, sizeof(cmdbuf), "/bin/cp -p -f -R %s %s", src, dst);
	system(cmdbuf);
	f_rm(src);
	if (delete_record(direct, sizeof(struct annheader), ent) == -1)
		presskeyfor("删除失败...");

	atrace(ANN_DELETE, anninfo->title, NULL);
	return NEWDIRECT;
}

int
ann_edit_article(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1];

	/* 检查权限 */
	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0' || anninfo->filename[0] == '@' || !(anninfo->flag & ANN_FILE))
		return DONOTHING;

	/* 编辑文章 */
	snprintf(fname, sizeof(fname), "%s/%s", ainfo.basedir, anninfo->filename);
	if (vedit(fname, EDIT_MODIFYHEADER) != -1) {
		anninfo->mtime = time(NULL);
		strlcpy(anninfo->editor, currentuser.userid, sizeof(anninfo->editor));
		safe_substitute_record(direct, (struct fileheader *)anninfo, ent, NA);
		atrace(ANN_EDIT, anninfo->title, NULL);
	}

	return FULLUPDATE;
}

int
ann_select_item(int ent, struct annheader *anninfo, char *direct)
{
	if (ainfo.manager != YEA)
		return DONOTHING;

	if (anninfo->filename[0] == '\0')
		return DONOTHING;

	if (anninfo->flag & ANN_SELECTED) {
		anninfo->flag &= ~ANN_SELECTED;
	} else {
		anninfo->flag |= ANN_SELECTED;
	}

	safe_substitute_record(direct, (struct fileheader *)anninfo, ent, NA);
	return PARTUPDATE;
}

static int
ann_delete_selected(void *rptr, void *extrarg)
{
	struct annheader *anninfo = (struct annheader *)rptr;
	char src[PATH_MAX + 1];
	char dst[PATH_MAX+1];	
	char cmdbuf[PATH_MAX + 1];

	if (!(anninfo->flag & ANN_SELECTED))
		return KEEPRECORD;

	anninfo->flag &= ~ANN_SELECTED;
	get_annjunk(genbuf);
	snprintf(dst, sizeof(dst), "%s/%d_%s_%s", genbuf, time(NULL), currentuser.userid, anninfo->title);
	snprintf(src, sizeof(src), "%s/%s", ainfo.basedir, anninfo->filename);
	snprintf(cmdbuf, sizeof(cmdbuf), "/bin/cp -p -f -R %s %s", src, dst);
	system(cmdbuf);
	f_rm(src);
	atrace(ANN_DELETE, anninfo->title, NULL);
	return (dash(src)) ? KEEPRECORD : REMOVERECORD;
}

int
ann_copycut_selected(void *rptr, void *extrarg)
{
	struct annheader *anninfo = (struct annheader *)rptr;
	int *flags = (int *)extrarg;

	if (!(anninfo->flag & ANN_SELECTED))
		return KEEPRECORD;

	anninfo->flag &= ~ANN_SELECTED;
	if (add_copypaste_record(flags[0], anninfo, flags[1]) != -1)
		flags[3]++;

	return KEEPRECORD;
}

int
ann_process_selected(int ent, struct annheader *anninfo, char *direct)
{
	char ch[2] = { '\0' }, fname[PATH_MAX + 1];
	int fd, result, flags[2];

	if (ainfo.manager != YEA)
		return DONOTHING;

	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "执行: 1) 复制  2) 剪切  3) 删除  0) 取消 [0]: ", ch, 2, DOECHO, YEA);
	if (ch[0] < '1' || ch[0] > '3') {
		saveline(t_lines - 1, 1);
		return DONOTHING;
	}

	switch (ch[0]) {
	case '1':
	case '2':
		sethomefile(fname, currentuser.userid, "copypaste");
		if ((fd = open(fname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1)
			goto error_process;

		flags[0] = fd;
		flags[1] = (ch[0] == '1') ? ANN_COPY : ANN_CUT;
		f_exlock(fd);
		if (process_records(direct, sizeof(struct annheader), 1, 999999, ann_copycut_selected, &flags) == -1) {
			close(fd);
			goto error_process;
		}		

		close(fd);
		presskeyfor("档案标识完成. (注意! 粘贴文章后才能将文章 delete!)");
		break;
	case '3':
		clear_line(t_lines - 1);
		if (askyn("确定删除", NA, NA) == YEA) {
			/* monster: here we choose a big enough number to avoid an extra
			 *          get_num_records call */
			result = process_records(direct, sizeof(struct annheader), 1, 999999, ann_delete_selected, NULL);
			fixkeep(direct, 1, 999999);

			if (result == -1)
				goto error_process;
		}
	}

	return (ch[0] == '1' || ch[0] == '2') ? NEWDIRECT : PARTUPDATE;

error_process:
	presskeyfor("操作失败...");
	return (ch[0] == '1' || ch[0] == '2') ? NEWDIRECT : PARTUPDATE;
}

static
int create_folder(int flag)
{
	struct annheader fileinfo;
	char dname[PATH_MAX + 1];
	char *ptr;

	/* 检查权限 */
	if (ainfo.manager == NA)
		return -2;

	/* 初始化 */
	memset(&fileinfo, 0, sizeof(fileinfo));

	/* 设定参数并创建目录 */
	if (a_prompt("标题: ", fileinfo.title, sizeof(fileinfo.title), YEA) == -1)
		return -2;

	if (getdirname(ainfo.basedir, dname) == -1)
		return -1;

	if ((ptr = strrchr(dname, '/')) == NULL)
		return -1;
	strlcpy(fileinfo.filename, ptr + 1, sizeof(fileinfo.filename));

	fileinfo.flag = flag;
	fileinfo.mtime = time(NULL);

	/* 添加记录 */
	if (append_record(currdirect, &fileinfo, sizeof(fileinfo)) == -1)
		return -1;

	return 0;
}

int
ann_create_guestbook(int ent, struct annheader *anninfo, char *direct)
{
	if (ainfo.manager == NA)
		return DONOTHING;

	if (create_folder(ANN_GUESTBOOK) == -1) {
		presskeyfor("操作失败...");
	} else {
		atrace(ANN_CREATE, "创建留言簿", anninfo->title);
	}
	return PARTUPDATE;
}

int
ann_create_folder(int ent, struct annheader *anninfo, char *direct)
{
	if (ainfo.manager == NA)
		return DONOTHING;

	if (create_folder(ANN_DIR) == -1) {
		presskeyfor("操作失败...");
	} else {
		atrace(ANN_CREATE, "创建目录", anninfo->title);
	}
	return PARTUPDATE;
}

int
ann_create_special(int ent, struct annheader *anninfo, char *direct)
{
	char ch[2], bname[BFNAMELEN];
	struct annheader fileinfo;

	/* 只有SYSOP和精华区主管可以创建特殊条目 */
	if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ANNOUNCE))
		return DONOTHING;

	stand_title("创建特殊条目");
	move(2, 0);
	prints("[\033[1;32m1\033[m] 空条目\n");
	prints("[\033[1;32m2\033[m] 讨论区精华\n");
	prints("[\033[1;32m3\033[m] 分类讨论区精华\n");
	getdata(6, 0, "你要创建哪一种特殊条目: ", ch, 2, DOECHO, YEA);

	/* 初始化fileinfo */
	memset(&fileinfo, 0, sizeof(fileinfo));
	fileinfo.flag = ANN_DIR;
	fileinfo.mtime = time(NULL);

	switch (ch[0]) {
	case '1':
		strcpy(fileinfo.filename, "@NULL");
		if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1)
			goto error_process;
		break;
	case '2':
		move(7, 0);
		prints("输入讨论区名 (按空白键自动搜寻): ");

		make_blist();
		namecomplete(NULL, bname);

		if (bname[0] == '\0')
			return FULLUPDATE;

		getdata(9, 0, "标题: ", fileinfo.title, sizeof(fileinfo.title), DOECHO, YEA);
		my_ansi_filter(fileinfo.title);
		if (fileinfo.title[0] == '\0')
			return FULLUPDATE;

		strcpy(fileinfo.filename, "@BOARDS");
		strlcpy(fileinfo.owner, bname, sizeof(fileinfo.owner) + sizeof(fileinfo.editor));
		if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1)
			goto error_process;
		break;
	case '3':
		do {
			getdata(8, 0, "分类讨论区编号(0-9, A-Z, *): ", ch, 2, DOECHO, YEA);
		} while (!isdigit(ch[0]) && !isalpha(ch[0]) && ch[0] != '*');

		getdata(9, 0, "标题: ", fileinfo.title, sizeof(fileinfo.title), DOECHO, YEA);
		my_ansi_filter(fileinfo.title);
		if (fileinfo.title[0] == '\0')
			return FULLUPDATE;

		snprintf(fileinfo.filename, sizeof(fileinfo.filename), "@GROUP:%c", ch[0]);
		if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1)
			goto error_process;
		break;
	}

	return FULLUPDATE;

error_process:
	presskeyfor("操作失败...");
	return FULLUPDATE;
}

int
ann_loadpaths(void)
{
	int fd, retval;
	char fname[PATH_MAX + 1];

	sethomefile(fname, currentuser.userid, "apaths");
	if ((fd = open(fname, O_RDONLY)) == -1)
		return -1;

	memset(paths, 0, sizeof(paths));
	retval = (read(fd, paths, sizeof(paths)) == sizeof(paths)) ? 0 : -1;
	close(fd);

	return retval;
}

int
ann_savepaths(void)
{
	int fd;
	char fname[PATH_MAX + 1];

	sethomefile(fname, currentuser.userid, "apaths");
	if ((fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
		return -1;

	if (write(fd, paths, sizeof(paths)) != sizeof(paths)) {
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

static int
ann_select_hpath(void)
{
	int i;
	char ch[2] = { '\0' };

	stand_title("历史路径");
	move(3, 0);
	for (i = 1; i < 8; i++) {
		if (paths[i].title[0] == '\0' || paths[i].path[0] == '\0')
			continue;

		prints("%d. 位置: ", i);
		if (paths[i].board[0] != '\0') {
			prints("%s\n", paths[i].board);
		} else {
			if (paths[i].board[1] != '\0') {
				prints("%s (个人文集)\n", &paths[i].board[1]);
			} else {
				outs("精华区公告栏\n");
			}
		}
		prints("   标题: %s\n\n", paths[i].title);
	}

	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "请选择丝路的编号 (1~7): ", ch, 2, DOECHO, YEA);

	if (ch[0] >= '1' && ch[0] <= '7') {
		i = ch[0] - '0';
		if (paths[i].board[0] != '\0' || paths[i].board[1] != '\0') {
			memcpy(&paths[0], &paths[i], sizeof(paths[0]));
			memset(&paths[i], 0, sizeof(paths[i]));
			ann_savepaths();
			presskeyfor("已将该路径设为丝路, 请按任意键继续...");
			return FULLUPDATE;
		}
	}

	presskeyfor("操作已取消, 请按任意键继续...");
	return FULLUPDATE;
}

int
ann_setpath(int ent, struct annheader *anninfo, char *direct)
{
	struct annpath paths2[8];

	char *ptr, ch[2] = { '\0' };
	int i, j;

	if (ainfo.manager != YEA)
		return DONOTHING;

	ann_loadpaths();
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0,
		"设定丝路: 1) 当前路径  2) 历史路径  0) 取消 [1]: ",
		ch, 2, DOECHO, YEA);

	if (ch[0] == '0')
		return PARTUPDATE;

	if (ch[0] == '2')
		return ann_select_hpath();

	memset(paths2, 0, sizeof(paths2));
	for (i = 0, j = 1; i <= 6; i++) {
		if (paths[i].path[0] == '\0' || strcmp(paths[i].path, ainfo.basedir) == 0)
			continue;
		memmove(&paths2[j], &paths[i], sizeof(paths[i]));
		++j;
	}
	memmove(paths, paths2, sizeof(paths));

	memset(&paths[0], 0, sizeof(paths[0]));
	strlcpy(paths[0].title, ainfo.title, sizeof(paths[0].title));
	strlcpy(paths[0].path, ainfo.basedir, sizeof(paths[0].path));
	if (strncmp(ainfo.basedir, "0Announce/boards/", 17) == 0) {
		strlcpy(paths[0].board, ainfo.basedir + 17, sizeof(paths[0].board));
		if ((ptr = strchr(paths[0].board, '/')) != NULL)
			*ptr = '\0';
	} else if (strncmp(ainfo.basedir, "0Announce/personal/", 19) == 0) {
		if (strlen(ainfo.basedir) < 23 || ainfo.basedir[20] != '/') {
			paths[0].board[0] = '\0';
			paths[0].board[1] = '\0';
		} else {
			strlcpy(paths[0].board + 1, ainfo.basedir + 20, sizeof(paths[0].board));
			if ((ptr = strchr(paths[0].board + 1, '/')) != NULL)
				*ptr = '\0';
			paths[0].board[0] = '\0';
		}
	}

	ann_savepaths();
	presskeyfor("已将该路径设为丝路, 请按任意键继续...");
	return FULLUPDATE;
}

int
ann_help(int ent, struct annheader *anninfo, char *direct)
{
	show_help("help/announcereadhelp");
	return FULLUPDATE;
}

int
ann_savepost(char *key, struct fileheader *fileinfo, int nomsg)
{
	char fname[PATH_MAX + 1];
	int ans = NA;

	if (nomsg == NA) {
		clear_line(t_lines - 1);
		snprintf(genbuf, sizeof(genbuf), "确定将 [%-.40s] 存入暂存档吗", fileinfo->title);
		if (askyn(genbuf, NA, YEA) == NA)
			return FULLUPDATE;
	}

	sethomefile(fname, currentuser.userid, "savepost");
	if (dashf(fname)) {
		ans = (nomsg) ? YEA : askyn("要附加在旧暂存档之后吗", NA, YEA);
	}

	if (INMAIL(uinfo.mode)) {
		snprintf(genbuf, sizeof(genbuf), "mail/%c/%s/%s", mytoupper(currentuser.userid[0]),
			 currentuser.userid, fileinfo->filename);
	} else {
		snprintf(genbuf, sizeof(genbuf), "boards/%s/%s", key, fileinfo->filename);
	}

	if (!ans) unlink(fname);
	f_cp(genbuf, fname, O_APPEND | O_CREAT);

	if (nomsg == NA) {
		presskeyfor("已将该文章存入暂存档, 请按<Enter>继续...");
	}

	return FULLUPDATE;
}

int
ann_export_savepost(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1];

	sethomefile(fname, currentuser.userid, "savepost");
	if (!dashf(fname))
		return DONOTHING;

	if (ann_add_file(fname, NULL, currentuser.userid, direct, 0, YEA) == -1)
		presskeyfor("操作失败...");

	return PARTUPDATE;
}

int
ann_make_symbolink(int ent, struct annheader *anninfo, char *direct)
{
	FILE *fp;
	time_t now;
	char ch[2];
	char type_char;
	char lpath[PATH_MAX + 2], resolved[PATH_MAX + 1];
	char spath[PATH_MAX + 1], fname[PATH_MAX + 1];
	struct annheader fileinfo;

	/* 只有SYSOP和精华区主管可以创建本地链接 */
	if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ANNOUNCE))
		return DONOTHING;

	sethomefile(fname, currentuser.userid, "slink");
	if ((fp = fopen(fname, "r")) == NULL)
		goto set_link;

	if (fgets(lpath, sizeof(lpath), fp) == NULL) {
		fclose(fp);
		goto set_link;
	}

	fclose(fp);

	getdata(t_lines - 1, 0,
		"本地链接: 1) 创建链接  2) 设置源路径  0) 取消 [0]: ",
		ch, 2, DOECHO, YEA);

	if (ch[0] == '1') goto create_link;
	if (ch[0] == '2') goto set_link;

	return PARTUPDATE;

set_link:               /* 记录源路径 */
	if ((fp = fopen(fname, "w")) == NULL)
		goto error_set_link;
	type_char = anninfo->filename[0];
	if (type_char != 'G' && type_char != 'D' && type_char != 'M')
		type_char = 'D';		/* Pudding: It must be a personal-announce-directory */
/*
	snprintf(lpath, sizeof(lpath), "%c%s/%s/%s", anninfo->filename[0], BBSHOME,
		 ainfo.basedir, anninfo->filename);
*/
	snprintf(lpath, sizeof(lpath), "%c%s/%s/%s", type_char, BBSHOME,
		 ainfo.basedir, anninfo->filename);

	fputs(lpath, fp);
	fclose(fp);
	presskeyfor("路径设置成功，请在目标路径处按 L 创建链接...");
	return PARTUPDATE;

create_link:            /* 创建本地链接 */
	memset(&fileinfo, 0, sizeof(fileinfo));
	if (a_prompt("标题: ", fileinfo.title, sizeof(fileinfo.title), NA) == -1)
		return PARTUPDATE;

	if (realpath(lpath + 1, resolved) == NULL)
		goto error_create_link;

	/* 检查源路径是否在BBS目录以外 */
	if (strncmp(resolved, BBSHOME"/", strlen(BBSHOME) + 1) != 0)
		goto error_create_link;

	/* 创建链接 */
	now = time(NULL);
	while (1) {
		snprintf(spath, sizeof(spath), "%s/%c.%d.L", ainfo.basedir, lpath[0], (int)now);
		if (symlink(lpath + 1, spath) == 0)
			break;
		if (errno != EEXIST)
			goto error_create_link;
	}

	fileinfo.flag = ANN_LINK;
	fileinfo.mtime = time(NULL);
	strlcpy(fileinfo.owner, currentuser.userid, sizeof(fileinfo.owner));
	strlcpy(fileinfo.editor, currentuser.userid, sizeof(fileinfo.editor));
	strlcpy(fileinfo.filename, strrchr(spath, '/') + 1, sizeof(fileinfo.filename));

	/* 添加记录 */
	if (append_record(currdirect, &fileinfo, sizeof(fileinfo)) == -1)
		goto error_create_link;

	unlink(fname);
	atrace(ANN_CREATE, "创建衔接", fileinfo.title);
	return PARTUPDATE;

error_set_link:         /* 记录源路径的错误处理 */
	presskeyfor("设置路径失败...");
	return PARTUPDATE;

error_create_link:      /* 创建本地链接的错误处理 */
	presskeyfor("创建本地链接失败...");
	return PARTUPDATE;
}

int
ann_move_item(int ent, struct annheader *anninfo, char *direct)
{
	char buf[TITLELEN];
	int dst;

	/* 检查权限 */
	if (ainfo.manager != YEA)
		return DONOTHING;

	snprintf(genbuf, sizeof(genbuf), "请输入第 %d 项的新次序: ", ent);
	a_prompt(genbuf, buf, 6, YEA);
	if ((dst = atoi(buf)) <= 0)
		return DONOTHING;

	if (move_record(direct, sizeof(struct annheader), ent, dst) != -1) {
		snprintf(buf, sizeof(buf), "把条目从第%d项移动到%d项", ent, dst);
		atrace(ANN_MOVE, buf, anninfo->title);
	}

	return DIRCHANGED;
}

int
ann_read(int ent, struct annheader *anninfo, char *direct)
{
	char *ptr, fname[PATH_MAX + 1];
	char attach_info[1024];
	int flag;

	if (anninfo->filename[0] == '\0')
		return FULLUPDATE;

	/* 检查是否为空条目 */
	if (strcmp(anninfo->filename, "@NULL") == 0)
		return FULLUPDATE;

	/* 检查是否为版面精华区 */
	if (strcmp(anninfo->filename, "@BOARDS") == 0)
		return show_board_announce(anninfo->owner);

	/* 检查是否为特殊条目 */
	if (anninfo->filename[0] == '@')
		return show_announce(anninfo->filename, anninfo->title, 0);

	/* 获取文件/目录名 */
	strlcpy(fname, direct, sizeof(fname));
	if ((ptr = strrchr(fname, '/')) == NULL)
		return FULLUPDATE;
	*ptr = '\0';
	snprintf(fname, sizeof(fname), "%s/%s", fname, anninfo->filename);

	/* 判断条目类型 */
	if (anninfo->flag & ANN_LINK) {
		switch (anninfo->filename[0]) {
		case 'G':       /* 留言 */
			flag = ANN_GUESTBOOK;
			break;
		case 'D':       /* 目录 */
			flag = ANN_DIR;
			break;
		case 'M':       /* 文件 */
			flag = ANN_FILE;
			break;
		default:
			return FULLUPDATE;
		}
	} else {
		flag = anninfo->flag;
	}

	if (flag & ANN_FILE) {
		if (anninfo->flag & ANN_ATTACHED) {
	                snprintf(attach_info, sizeof(attach_info),
                	         "http://%s/bbsanc?path=%s",
				 BBSHOST, fname + 10);
		}
		
		if (ansimore4(fname, NULL, NULL, (anninfo->flag & ANN_ATTACHED) ? attach_info : NULL, NA) == -1) {
			clear();
			move(10, 29);
			prints("对不起，文章内容丢失!");
			pressanykey();
			return FULLUPDATE;
		}

		clear_line(t_lines - 1);
		outs("\033[1;31;44m[阅读精华区资料]  \033[33m │ 结束 Q,← │ 上一项 U,↑│ 下一项 <Space>,↓ │          \033[m");

		switch (egetch()) {
			case KEY_DOWN:
			case ' ':
			case '\n':
				return READ_NEXT;
			case KEY_UP:
			case 'u':
			case 'U':
				return READ_PREV;
		}
	} else if ((flag & ANN_DIR) || (flag & ANN_GUESTBOOK)) {
		strlcat(fname, "/.DIR", sizeof(fname));
		return show_announce(fname, anninfo->title, flag);
	}
	return FULLUPDATE;
}

int
ann_select_board(int ent, struct annheader *anninfo, char *direct)
{
	char bname[BFNAMELEN];

	move(0, 0);
	clrtoeol();
	prints("选择一个讨论区 (英文字母大小写皆可)\n");
	prints("输入讨论区名 (按空白键自动搜寻): ");
	clrtoeol();

	make_blist();
	namecomplete(NULL, bname);
	return show_board_announce(bname);
}

int
ann_change_title(int ent, struct annheader *anninfo, char *direct)
{
	char oldtitle[TITLELEN];

	/* 检查权限 */
	if (ainfo.manager != YEA)
		return DONOTHING;
	
	strlcpy(oldtitle, anninfo->title, sizeof(oldtitle));
	a_prompt("标题: ", anninfo->title, sizeof(anninfo->title), NA);
	if (anninfo->title[0] == '\0')
		return DONOTHING;
	if (safe_substitute_record(direct, (struct fileheader *)anninfo, ent, NA) != -1)
		atrace(ANN_CTITLE, oldtitle, anninfo->title);

	return PARTUPDATE;
}

int
ann_edit_notes(int ent, struct annheader *anninfo, char *direct)
{
	char ans[4], fname[PATH_MAX + 1];
	int aborted;

	if (!HAS_PERM(PERM_ACBOARD) && ainfo.manager == NA)     /* monster: suggested by MidautumnDay */
		return DONOTHING;

	snprintf(fname, sizeof(fname), "%s/welcome", ainfo.basedir);

	clear();
	move(1, 0);
	prints("编辑/删除备忘录");
	getdata(3, 0, "(E)编辑 (D)删除 (A)取消 [E]: ", ans, 2, DOECHO, YEA);
	if (ans[0] == 'A' || ans[0] == 'a') {
		aborted = -1;
	} else if (ans[0] == 'D' || ans[0] == 'd') {
		move(4, 0);
		if (askyn("真的要删除备忘录", NA, NA) == YEA) {
			move(5, 0);
			unlink(fname);
			atrace(ANN_DNOTES, NULL, NULL);
			prints("备忘录已经删除...\n");
		}
	} else {
		if (vedit(fname, EDIT_MODIFYHEADER) != -1) {
			atrace(ANN_ENOTES, NULL, NULL);
			prints("备忘录已更新\n");
		}
	}
	pressreturn();
	return FULLUPDATE;
}

int
ann_show_notes(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1];

	snprintf(fname, sizeof(fname), "%s/welcome", ainfo.basedir);
	if (dashf(fname)) show_help(fname);

	return FULLUPDATE;
}

int
ann_crosspost(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1], tname[PATH_MAX + 1];
	char bname[BFNAMELEN], title[TITLELEN], buf[8192];
	FILE *fin, *fout;
	int len;

	if ((!HAS_PERM(PERM_POST) && !HAS_PERM(PERM_WELCOME)) || !(anninfo->flag & ANN_FILE))
		return DONOTHING;

	snprintf(fname, sizeof(fname), "%s/%s", ainfo.basedir, anninfo->filename);
	snprintf(tname, sizeof(tname), "%s/cross.%5d", ainfo.basedir, getpid());

	clear();
	if (get_a_boardname(bname, "请输入要转贴的讨论区名称: ")) {
		move(1, 0);
		if (deny_me(bname) || !haspostperm(bname) || check_readonly(bname)) {
			prints("\n\n此讨论区是唯读的, 或是您尚无权限在此发表文章。");
		} else {
			snprintf(genbuf, sizeof(genbuf), "你确定要转贴到 %s 版吗", bname);
			if (askyn(genbuf, NA, NA) == 1) {
				move(3, 0);

				if ((fin = fopen(fname, "r")) == NULL)
					goto error_process;

				if ((fout = fopen(tname, "w")) == NULL) {
					fclose(fin);
					goto error_process;
				}

				while ((len = fread(buf, 1, sizeof(buf), fin)) > 0)
					fwrite(buf, 1, len, fout);

				fprintf(fout, "\n--\n\033[m\033[1;%2dm※ 转载:．%s %s．[FROM: %s]\033[m\n",
					(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, fromhost);

				fclose(fin);
				fclose(fout);

				set_safe_record();
				strlcpy(quote_user, anninfo->owner, sizeof(quote_user));
				snprintf(title, sizeof(title), "[转载] %s", anninfo->title);
				postfile(tname, bname, title, 3);
				unlink(tname);
				prints("已经帮你转贴至 %s 版了", bname);
			}
		}
	}

	pressreturn();
	return FULLUPDATE;

error_process:
	prints("转贴失败: 系统发生错误");
	pressreturn();
	return FULLUPDATE;
}

#ifdef INTERNET_EMAIL

int
ann_forward(int ent, struct annheader *anninfo, char *direct, int uuencode)
{
	struct fileheader fileinfo;
	char fname[PATH_MAX + 1], tname[PATH_MAX + 1], buf[8192];
	FILE *fin, *fout;
	int len;

	if (!HAS_PERM(PERM_FORWARD) || !(anninfo->flag & ANN_FILE))
		return DONOTHING;

	snprintf(fname, sizeof(fname), "%s/%s", ainfo.basedir, anninfo->filename);
	snprintf(tname, sizeof(tname), "%s/forward.%5d", ainfo.basedir, getpid());

	if ((fin = fopen(fname, "r")) == NULL)
		return DONOTHING;

	if ((fout = fopen(tname, "w")) == NULL) {
		fclose(fin);
		return DONOTHING;
	}

	getdatestring(time(NULL));
	fprintf(fout, "寄信人: %s (%s)\n", currentuser.userid, currentuser.username);
	fprintf(fout, "标  题: %s\033[m\n", anninfo->title);
	fprintf(fout, "发信站: %s (%s)\n", BoardName, datestring);
	fprintf(fout, "来  源: %s\n\n", currentuser.lasthost);

	while ((len = fread(buf, 1, sizeof(buf), fin)) > 0)
		fwrite(buf, 1, len, fout);

	fclose(fin);
	fclose(fout);

	memcpy(&fileinfo, anninfo, sizeof(fileinfo));
	snprintf(fileinfo.filename, sizeof(fileinfo.filename), "forward.%5d", getpid());

	switch (doforward(ainfo.basedir, &fileinfo, uuencode)) {
	case 0:
		prints("文章转寄完成!\n");
		break;
	case -1:
		prints("转寄失败: 系统发生错误.\n");
		break;
	case -2:
		prints("转寄失败: 不正确的收信地址.\n");
		break;
	case -3:
		prints("您的信箱超限，暂时无法使用邮件服务.\n");
		break;
	default:
		prints("取消转寄...\n");
	}
	unlink(tname);
	pressreturn();
	clear();
	return FULLUPDATE;
}

int
ann_mail_forward(int ent, struct annheader *anninfo, char *direct)
{
	return ann_forward(ent, anninfo, direct, NA);
}

int
ann_mail_u_forward(int ent, struct annheader *anninfo, char *direct)
{
	return ann_forward(ent, anninfo, direct, YEA);
}

#endif

int
ann_view_atrace(int ent, struct annheader *anninfo, char *direct)
{
	char fname[PATH_MAX + 1], olddirect[PATH_MAX + 1];

	/* monster: 讨论区主管可以查看版面精华区记录, 但不意味着讨论区主管可以更改精华区内容 */
	if ((!strncmp(direct, "0Announce/boards/", 17) && HAS_PERM(PERM_OBOARDS)) || ainfo.manager == YEA) {
 		if (get_atracename(fname) == -1)
			return DONOTHING;
	} else {
		return DONOTHING;
	}

	strlcpy(olddirect, currdirect, sizeof(olddirect));
	i_read(DIGESTRACE, fname, NULL, NULL, atracetitle, atracedoent, update_atraceendline, &atrace_comms[0],
	       get_records, get_num_records, sizeof(struct anntrace));
	strlcpy(currdirect, olddirect, sizeof(currdirect));
	return NEWDIRECT;
}

/* 精华区索引生成 */
void
ann_process_item(FILE *fp, char *directory, struct annheader *header, int *last, int level,int onlydir) 
{
	char buf[256] = { '\0' }, tdir[PATH_MAX + 1];
	int traverse, color;
	int i, maxlen;

	if ((header->flag & ANN_LINK) || (header->flag & ANN_RLINK)) {	// 衔接
		traverse = NA;
		color = 32;			
	} else if (header->flag & ANN_DIR) {				// 目录
		traverse = (level < 15) ? YEA : NA;
		color = 37;
	} else if (header->flag & ANN_GUESTBOOK) {			// 留言簿
		traverse = (level < 15) ? YEA : NA;
		color = 35;
	} else {							// 文件 - 其它
		traverse = NA;
		color = 0;
	}

	for (i = 0; i < level - 1; i++)
		strlcat(buf, (last[i] == NA) ? "│   " : "     ", sizeof(buf));
	strlcat(buf, (last[level - 1] == NA) ? "├─ " : "└─ ", sizeof(buf));

	if (color > 0)
		snprintf(buf, sizeof(buf), "%s\033[1;%dm", buf, color);

	maxlen = 78 - level * 5;
	if (strlen(header->title) > maxlen) {
		char title[TITLELEN];
		
		strlcpy(title, header->title, sizeof(title));
		title[maxlen] = '\0';
		title[maxlen - 1] = '.';
		title[maxlen - 2] = '.';
		title[maxlen - 3] = '.';
		strlcat(buf, title, sizeof(buf));
	} else {
		strlcat(buf, header->title, sizeof(buf));
	}
	strlcat(buf, (color > 0) ? "\033[m\n" : "\n", sizeof(buf));
	fputs(buf, fp);
	
	if (traverse == YEA) {
		snprintf(tdir, sizeof(tdir), "%s/%s", directory, header->filename);
		ann_index_traverse(fp, tdir, last, level + 1,onlydir);
	}
}

void
ann_index_traverse(FILE *fp, char *directory, int *last, int level,int onlydir) /*SuperDog：索引中只显示目录和留言时为YEA,否则为NA*/
{
	char filename[PATH_MAX + 1];
	struct annheader header, rheader;
	FILE *fdir;

	snprintf(filename, sizeof(filename), "%s/.DIR", directory);
	if ((fdir = fopen(filename, "r")) == NULL)
		return;

	if (fread(&header, 1, sizeof(header), fdir) != sizeof(header)) {
		fclose(fdir);
		return;
	}
	if (onlydir==YEA && !((header.flag & ANN_DIR) || (header.flag & ANN_GUESTBOOK)))
	{
		while (fread(&header,1,sizeof(header),fdir) == sizeof(header))
			if ((header.flag & ANN_DIR) || (header.flag & ANN_GUESTBOOK)) break;
	}

	last[level - 1] = NA;
	while (fread(&rheader, 1, sizeof(rheader), fdir) == sizeof(rheader)) {
		if (onlydir==YEA) {
			if ((rheader.flag & ANN_DIR) || (rheader.flag & ANN_GUESTBOOK))
			{
				ann_process_item(fp, directory, &header, last, level, onlydir);
				memcpy(&header, &rheader, sizeof(header));
			}
		}
		else {
			ann_process_item(fp, directory, &header, last, level, onlydir);
                        memcpy(&header, &rheader, sizeof(header));
		}
	}
	last[level - 1] = YEA;
	if (onlydir==YEA) {
		if ((header.flag & ANN_DIR) || (header.flag & ANN_GUESTBOOK))
			ann_process_item(fp, directory, &header, last, level, onlydir);
	}
	else ann_process_item(fp, directory, &header, last, level, onlydir);
	fclose(fdir);
}

int
ann_make_index(int ent, struct annheader *anninfo, char *direct)
{
	FILE *fp;
	int last[16];
	char filename[PATH_MAX + 1];

	if (ainfo.manager != YEA)
		return DONOTHING;

	snprintf(filename, sizeof(filename), "tmp/annindex.%5d", getpid());
	if ((fp = fopen(filename, "w")) == NULL)
		return DONOTHING;

	clear_line(t_lines - 1);
	prints("正在生成精华区索引，请稍候。。。");
	refresh();

	last[0] = NA;
	fprintf(fp, "\033[1;41;33m精华区索引：%s%*s\033[m\n", ainfo.title, (int)(67 - strlen(ainfo.title)), " ");
	ann_index_traverse(fp, ainfo.basedir, last, 1, NA);
	fclose(fp);
	ann_add_file(filename, "精华区索引", "", currdirect, 0, YEA);
	atrace(ANN_INDEX, NULL, NULL);		

	return PARTUPDATE;
}

int 
ann_make_dirindex(int ent,struct annheader *anninfo,char *direct)
{
	FILE *fp;
	int last[16];
	char filename[PATH_MAX + 1];
	if (ainfo.manager != YEA)
		return DONOTHING;
	snprintf(filename, sizeof(filename), "tmp/anndirindex.%5d", getpid());
	if ((fp = fopen(filename, "w")) == NULL)
		return DONOTHING;

	clear_line(t_lines - 1);
	prints("正在生成精华区目录索引，请稍候。。。");
	refresh();

	last[0] = NA;
	fprintf(fp, "\033[1;41;33m精华区目录索引：%s%*s\033[m\n", ainfo.title, (int)(67 - strlen(ainfo.title)), " ");
	ann_index_traverse(fp, ainfo.basedir, last, 1, YEA);
	fclose(fp);
	ann_add_file(filename, "精华区目录索引", "", currdirect, 0, YEA);
	atrace(ANN_INDEX, NULL, NULL);		

	return PARTUPDATE;

}
struct one_key ann_comms[] = {
	{ 'A',          ann_switch_sauthor   },
	{ 'a',          ann_add_article      },
	{ 'c',          ann_copy             },
	{ 'D',          ann_delete_range     },
	{ 'd',          ann_delete_item      },
	{ 'E',          ann_edit_article     },
	{ 'e',          ann_select_item      },
	{ 'G',          ann_create_guestbook },
	{ 'g',          ann_create_folder    },
	{ 'f',          ann_setpath          },
	{ 'h',          ann_help             },
	{ 'i',          ann_export_savepost  },
	{ 'I',		ann_make_index	     },
        { Ctrl('D'),    ann_make_dirindex    }, 
	{ 'L',          ann_make_symbolink   },
	{ 'm',          ann_move_item        },
	{ 'o',          fast_cloak           },
	{ 'p',          ann_paste            },
	{ 'r',          ann_read             },
	{ 's',          ann_select_board     },
	{ 'T',          ann_change_title     },
	{ 'x',          ann_cut              },
	{ 'W',          ann_edit_notes       },
	{ '.',          ann_create_special   },
	{ ',',          ann_view_atrace      },
        { '/', 		title_search_down    },         
        { '?',		title_search_up      },           
#ifdef INTERNET_EMAIL
	{ 'F',          ann_mail_forward     },
	{ 'U',          ann_mail_u_forward   },
#endif
	{ KEY_TAB,      ann_show_notes       },
	{ Ctrl('C'),    ann_crosspost        },
	{ Ctrl('E'),    ann_process_selected },
	{ Ctrl('P'),    ann_add_article      },
	{ Ctrl('V'), 	x_lockscreen_silent },
	{ '\0',         NULL }
};

struct one_key ann_comms_readonly[] = {
	{ 'A',          ann_switch_sauthor   },
	{ 'h',          ann_help             },
	{ 'o',          fast_cloak           },
	{ 'r',          ann_read             },
	{ 's',          ann_select_board     },
	{ Ctrl('V'), 	x_lockscreen_silent },
	{ '\0',         NULL }
};

static struct boardheader *blist;       /* 区内版面列表 */
static int bcount;                      /* 区内版面数目 */
static char groupid;                    /* 区标示 */

static int
get_num_records_blist(char *filename, int size)
{
	return bcount;
}

static int
get_records_blist(char *filename, void *rptr, int size, int id, int number)
{
	struct annheader fileinfo;
	void *ptr = rptr;
	int count = 0;

	memset(&fileinfo, 0, sizeof(fileinfo));
	strlcpy(fileinfo.filename, "@BOARDS", sizeof(fileinfo.filename));
	fileinfo.flag = ANN_DIR;
	fileinfo.mtime = time(NULL);

	while ((number--) && (id <= bcount)) {
		strlcpy(fileinfo.title, blist[id - 1].title + 8, sizeof(fileinfo.title));
		strlcpy(fileinfo.owner, blist[id - 1].filename, sizeof(fileinfo.owner) + sizeof(fileinfo.editor));
		memcpy(ptr, &fileinfo, sizeof(fileinfo));
		ptr += size;
		++id;
		++count;
	}
	return count;
}

static void
init_blist()
{
	int i, size = 25;
	char *prefix = NULL, buf[STRLEN] = "EGROUP*";

	bcount = 0;

	if (groupid != '*') {
		buf[6] = groupid;
		if ((prefix = sysconf_str(buf)) == NULL || prefix[0] == '\0')   /* 获取区前缀 */
			return;
	}

	/* 分配内存 */
	if ((blist = malloc(sizeof(struct boardheader) * size)) == NULL)
		return;

	/* 生成区内版面列表 */
	for (i = 0; i < numboards; i++) {
		/* 隐藏不可见版面和它区版面 */
		if (bcache[i].filename[0] == '\0')
			continue;

		if (!(bcache[i].level & PERM_POSTMASK) && !HAS_PERM(bcache[i].level) && !(bcache[i].level & PERM_NOZAP))
			continue;

		if (groupid != '*' && strchr(prefix, bcache[i].title[0]) == NULL)
			continue;

		if (bcache[i].flag & BRD_RESTRICT)
			if (restrict_boards == NULL || strsect(restrict_boards, bcache[i].filename, "\n\t ") == NULL)
				continue;

		/* 复制版面属性至blist备用, 如不够内存则重新分配, 增量为25 */
		if (bcount == size - 1) {
			size += 25;
			if ((blist = realloc(blist, sizeof(struct boardheader) * size)) == NULL) {
				bcount = 0;
				return;
			}
		}

		memcpy(&blist[bcount], &bcache[i], sizeof(struct boardheader));
		++bcount;
	}
}

static void
free_blist()
{
	if (blist != NULL) {
		free(blist);
		blist = NULL;
		bcount = 0;
	}
}

int
show_announce_special(char *direct)
{
	if (strncmp(direct, "@GROUP:", 7) == 0) {               // 分类讨论区精华 (区内列表)
		if (bcount > 0)                                 // 因为blist, bcount等变量公用, 故不可重入
			return DONOTHING;

		groupid = direct[7];
		i_read(DIGEST, direct, init_blist, free_blist, anntitle, anndoent, update_annendline, &ann_comms_readonly[0],
		       get_records_blist, get_num_records_blist, sizeof(struct annheader));

		return NEWDIRECT;
	} else if (strncmp(direct, "@PERSONAL:", 10) == 0) {    // 个人文集
		return show_personal_announce(direct + 10);
	} else if (strcmp(direct, "@UP") == 0) {                // 返回上一层目录
		return DOQUIT;
	}

	return DONOTHING;
}

int
show_announce(char *direct, char *title, int flag)
{
	int result = NEWDIRECT;
	struct anninfo sinfo;
	char *ptr;

	memcpy(&sinfo, &ainfo, sizeof(sinfo));                  // 保存上一层参数
	ainfo.manager = check_annmanager(direct);               // 检查是否可以管理该子目录
	ainfo.flag = flag;                                      // 设置标志
	strlcpy(ainfo.direct, direct, sizeof(ainfo.direct));    // 设置索引文件名
	strlcpy(ainfo.title, title, sizeof(ainfo.title));       // 设置标题
	strlcpy(ainfo.basedir, direct, sizeof(ainfo.basedir));  // 求出精华区所在目录
	if ((ptr = strrchr(ainfo.basedir, '/')) != NULL)
		*ptr = '\0';

	if (direct[0] == '@') {
		result = show_announce_special(direct);
	} else {
		/* 显示备忘录 */
		ann_show_notes(0, NULL, NULL);

		i_read(DIGEST, direct, NULL, NULL, anntitle, anndoent, update_annendline, &ann_comms[0],
			      get_records, get_num_records, sizeof(struct annheader));
	}

	memcpy(&ainfo, &sinfo, sizeof(ainfo));                  // 恢复上一层参数
	strlcpy(currdirect, ainfo.direct, sizeof(currdirect));  // 恢复索引文件名

	return result;
}

/* 查看指定版面的精华区 */
int
show_board_announce(char *bname)
{
	struct boardheader *bp;
	char direct[PATH_MAX + 1];

	if ((bname[0] == '\0') || ((bp = getbcache(bname)) == NULL))
		return FULLUPDATE;

	snprintf(direct, sizeof(direct), "0Announce/boards/%s/.DIR", bname);
	return show_announce(direct, bp->title + 11, ANN_DIR);
}

/* 查看指定用户的个人文集 */
int
show_personal_announce(char *userid)
{
	char direct[PATH_MAX + 1];
	struct annheader fileinfo;

	strlcpy(ainfo.direct, currdirect, sizeof(ainfo.direct));
	if (userid == NULL || userid[0] == '\0' || userid[0] == '*')
		return show_announce("0Announce/personal/.DIR", BBSNAME" 个人文集", ANN_DIR);

	if (isalpha(userid[0]) && userid[1] == '\0') {
		snprintf(direct, sizeof(direct), "0Announce/personal/%c/.DIR", mytoupper(userid[0]));
		return show_announce(direct, BBSNAME" 个人文集", ANN_DIR);
	}

	snprintf(direct, sizeof(direct), "0Announce/personal/%c/.DIR", mytoupper(userid[0]));
	if (search_record(direct, &fileinfo, sizeof(fileinfo), cmpafilename, userid) <= 0)
		return DONOTHING;

	snprintf(direct, sizeof(direct), "0Announce/personal/%c/%s/.DIR", mytoupper(userid[0]), userid);
	return show_announce(direct, fileinfo.title, ANN_DIR);
}

/* 查看指定用户的个人文集 */
int
pannounce()
{
	char userid[IDLEN + 2];

	clear();
	move(2, 0);
	usercomplete("您想看谁的个人文集: ", userid);

	if (userid[0] != '\0') {
		show_personal_announce(userid);
	}

	return FULLUPDATE;
}

/* 进入文章作者的个人文集 */
int
author_announce(int ent, struct fileheader *fileinfo, char *direct)
{
	return show_personal_announce(fileinfo->owner);
}

/* 进入当前版精华区 */
int
currboard_announce()
{
	strlcpy(ainfo.direct, currdirect, sizeof(ainfo.direct));
	return show_board_announce(currboard);
}

/* 进入精华区公布栏 */
int
announce()
{
	strlcpy(ainfo.direct, currdirect, sizeof(ainfo.direct));
	return show_announce("0Announce/.DIR", "精华区公布栏", ANN_DIR);
}

/* 创建个人文集 */
int
add_personalcorpus()
{
	int id;
	char userid[IDLEN + 2];
	char direct[PATH_MAX + 1], title[TITLELEN];
	struct annheader fileinfo;
	struct userec lookupuser;

	/* SYSOP和精华区主管才可以创建个人文集 */
	if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ANNOUNCE))
		return DONOTHING;

	modify_user_mode(ADMIN);
	stand_title("创建个人文集");

	move(2, 0);
	usercomplete("您想为谁创建个人文集: ", userid);
	clear_line(2);
	if (userid[0] == '\0' || (id = getuser(userid, &lookupuser)) == 0)
		return 0;

	snprintf(direct, sizeof(direct), "0Announce/personal/%c/.DIR", mytoupper(userid[0]));
	if (search_record(direct, &fileinfo, sizeof(fileinfo), cmpafilename, userid) <= 0) {
		/* 初始化 fileinfo */
		memset(&fileinfo, 0, sizeof(fileinfo));
		strlcpy(fileinfo.filename, userid, sizeof(fileinfo.filename));
		fileinfo.flag = ANN_DIR;
		fileinfo.mtime = time(NULL);

		/* 设定文集标题 */
		getdata(2, 0, "标题: ", title, sizeof(fileinfo.title), DOECHO, YEA);
		my_ansi_filter(title);
		if (title[0] == '\0')
			return 0;

		memset(fileinfo.title, '-', sizeof(fileinfo.title));
		memcpy(fileinfo.title, userid, strlen(userid));
		strlcpy(fileinfo.title + 19, title, sizeof(fileinfo.title) - 19);

		/* 添加记录 */
		if (append_record(direct, &fileinfo, sizeof(fileinfo)) == -1) {
			move(4, 0);
			outs("创建失败...");
			pressanykey();
			return 0;
		}
	}

	/* 赋予文集管理权限 */
	if (!(lookupuser.userlevel & PERM_PERSONAL)) {
		lookupuser.userlevel |= PERM_PERSONAL;
		if (substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), id) == -1) {
			move(4, 0);
			outs("无法赋予使用者文集管理权限...");
			pressanykey();
			return 0;
		}
	}

	/* 创建个人文集目录 */
	snprintf(direct, sizeof(direct), "0Announce/personal/%c/%s", mytoupper(userid[0]), userid);
	if (f_mkdir(direct, 0755) == -1) {
		move(4, 0);
		outs("创建失败...");
		pressanykey();
		return 0;
	}

	move(4, 0);
	prints("%s的个人文集创建成功", userid);
	snprintf(genbuf, sizeof(genbuf), "为%s创建个人文集", userid);
	securityreport(genbuf);
	pressanykey();

	return 0;
}
