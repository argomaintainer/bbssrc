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

    Adms Bulletin Board System
    Copyright (C) 2013, Mo Norman, LTaoist6@gmail.com

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

struct postheader header;
extern int child_pid;
int digestmode;
int local_article;
struct userec currentuser;	
int usernum = 0;
char currboard[BFNAMELEN + 1];
int current_bm;	/* is current bm ? */
char someoneID[31];
int FFLL = 0;
int ReadID = 0;
int noreply = NA;
int anonymousmail = 0;
static int postandmail = 0;
extern char *restrict_boards;

unsigned int posts_article_id;	/* for write_posts() */


#ifdef  INBOARDCOUNT 
int 	is_inboard = 0;		/* 用户进入版面?  */
#endif 
char	attach_link[1024];	/* 附件链接 */
char	attach_info[1024];	/* 附件信息 */



#ifdef MARK_X_FLAG
int markXflag = 0;
#endif
int mailtoauthor = 0;

char genbuf[BUFLEN];
char save_title[TITLELEN];
char quote_title[TITLELEN], quote_board[BFNAMELEN + 1];
char quote_file[PATH_MAX + 1], quote_user[IDLEN + 2];
struct	fileheader* quote_fh;	/* freestyler: for 转载文章 */

#ifndef NOREPLY
char replytitle[STRLEN];
#endif

int totalusers, usercounter;

extern int friendflag;

/* monster: 检查版面是否只读 */
inline int
check_readonly(char *board)
{
	struct boardheader *bp;

	bp = getbcache(board);
	return (bp->flag & BRD_READONLY) ? YEA : NA;
}
/* Cypress:  检查版面flag是否有某个attr */
inline int 
check_board_attr(char *board, unsigned int attr)
{
	struct boardheader *bp;

	bp = getbcache(board);
	return (bp->flag & attr) ? YEA : NA;
}

inline int
check_bm(char *userid, char *bm)
{
	char *p, ch;

	if ((p = strcasestr(bm, userid)) == NULL) /* ignore case search */
		return NA;

	if (p != bm) {
		while (1) {
			ch = *(p - 1);
			p += strlen(userid);
			if (ch == ' ' || ch == '\t' || ch == '(')
				break;
			if (*p == '\0' || (p = strcasestr(p, userid)) == NULL)
				return NA;
		}
	}  else {
		p += strlen(userid);
	}

	return (*p == '\0' || *p == ' ' || *p == '\t' || *p == ')');
}

/* Added by cancel at 2001.09.10 */
/* 检查是否到文章上限，是则返回0表示不能发文，否则返回1表示可以发文 */
/* monster: 增加版面标志的检查 */

inline int
check_max_post(char *board)
{
	int num;
	char filename[PATH_MAX + 1];
	struct boardheader *bp;

	bp = getbcache(board);
	if ((HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
	     check_bm(currentuser.userid, bp->BM)) ||
	    HAS_PERM(PERM_BLEVELS))
		return YEA;
	
	if (bp->flag & NOPLIMIT_FLAG)
		return YEA;

	snprintf(filename, sizeof(filename), "boards/%s/%s", board, DOT_DIR);
	num = get_num_records(filename, sizeof (struct fileheader));

	if ((!(bp->flag & BRD_MAXII_FLAG) && num >= MAX_BOARD_POST) ||
	    ((bp->flag & BRD_MAXII_FLAG) && num >= MAX_BOARD_POST_II)) {
		clear();
		outs("\n\n           对不起，该版的文章数已经到了上限，暂时无法发文   \n");
		outs("                        请耐心等候站长的处理");
		pressreturn();
		clear();
		return NA;
	}
	return YEA;
}

/* Added End */

/* 是否替换 例如 `$userid' 的字符串 */
inline int
check_stuffmode(void)
{
	return (uinfo.mode == RMAIL) ? YEA : NA;
}

inline int
isowner(struct userec *user, struct fileheader *fileinfo)
{
	if ((strcmp(fileinfo->owner, user->userid)) &&
	     (strcmp(fileinfo->realowner, user->userid)))
		return NA;

	return (fileinfo->filetime < user->firstlogin) ? NA : YEA;
}

void
setrid(int id)
{
	FFLL = 1;
	ReadID = (id == 0) ? -1 : id;
}

void
setbdir(char *buf, char *boardname)
{
	switch (digestmode) {
	case 0:
		sprintf(buf, "boards/%s/%s", boardname, DOT_DIR);
		break;
	case 1:
		sprintf(buf, "boards/%s/%s", boardname, DIGEST_DIR);
		break;
	case 2:
		sprintf(buf, "boards/%s/%s", boardname, THREAD_DIR);
		break;
	case 3:
		sprintf(buf, "boards/%s/%s", boardname, MARKED_DIR);
		break;
	case 4:
		sprintf(buf, "boards/%s/%s", boardname, AUTHOR_DIR);
		break;
	case 5:         /* 同作者 */
	case 6:         /* 同作者 */
		sprintf(buf, "boards/%s/SOMEONE.%s.DIR.%d", boardname,
			someoneID, digestmode - 5);
		break;
	case 7:         /* 标题关键字 */
		sprintf(buf, "boards/%s/KEY.%s.DIR", boardname,
			currentuser.userid);
		break;
	case 8:         /* 回收站 */
		sprintf(buf, "boards/%s/%s", boardname, DELETED_DIR);
		break;
	case 9:         /* 废纸篓 */
		sprintf(buf, "boards/%s/%s", boardname, JUNK_DIR);
		break;
	case 10:        /* 封禁列表 */
	case 11:
		sprintf(buf, "boards/%s/%s", boardname, DENY_DIR);
		break;
	}
}

int
denynames(void *userid, void *dh_ptr)
{
	struct denyheader *dh = (struct denyheader *)dh_ptr;

	return (strcmp((char *)userid, dh->blacklist)) ? NA : YEA;
}

int
deny_me(char *bname)
{
	char buf[PATH_MAX + 1];
	struct denyheader dh;

	setboardfile(buf, bname, DENY_DIR);
	return search_record(buf, &dh, sizeof(struct denyheader), denynames, currentuser.userid);
}

int
deny_me_fullsite(void)
{
	struct denyheader dh;

	return search_record("boards/.DENYLIST", &dh, sizeof(struct denyheader), denynames, currentuser.userid);
}

int
shownotepad(void)
{
	modify_user_mode(NOTEPAD);
	ansimore("etc/notepad", YEA);
	return 0;
}

int
g_board_names(struct boardheader *bptr)
{
	if (bptr->flag & BRD_RESTRICT) {
		if (restrict_boards == NULL || strsect(restrict_boards, bptr->filename, "\n\t ") == NULL)
			return 0;
	}

	/* freestyler: 校内版面名不出现在namecomplete中 */
	if (valid_host_mask == HOST_AUTH_NA) {
		if (bptr->flag & BRD_INTERN) { /* 校内版面 */ 
			if (!HAS_PERM(PERM_SYSOP) && 
			    !HAS_PERM(PERM_OBOARDS) &&
			    !HAS_PERM(PERM_INTERNAL) &&
			    !HAS_PERM(PERM_BOARDS) )
				return 0;
		}
		if (bptr->flag & BRD_HALFOPEN) { /* 校外激活才可看 */
			if (!HAS_PERM(PERM_SYSOP) &&
			    !HAS_PERM(PERM_WELCOME))
				return 0;
		}
	} 

	if ((bptr->level & PERM_POSTMASK) || HAS_PERM(bptr->level)
	    || (bptr->level & PERM_NOZAP)) {
		AddToNameList(bptr->filename);
	}
	return 0;
}

void
make_blist(void)
{
	CreateNameList();
	apply_boards(g_board_names);
}

int
Select(void)
{
	modify_user_mode(SELECT);
	do_select(0, NULL, genbuf);
	return 0;
}

int
Post(void)
{
	if (currboard[0] == 0) {
		prints("\n\n先用 (S)elect 去选择一个讨论区。\n");
		pressreturn();
		clear();
		return 0;
	}
#ifndef NOREPLY
	*replytitle = '\0';
#endif
	do_post();
	return 0;
}

int
postfile(char *filename, char *nboard, char *posttitle, int mode)
{
	int result;
	char dbname[BFNAMELEN + 1];

/*
	monster: 找不到版面只可能发生两种情况

		 1. 产生一垃圾文件
		 2. 在post_cross中打开文件失败返回

		 故没有必要画蛇添足在post之前检查版面是否存在，
		 因为错误发生的可能性极小

	struct boardheader fh;

	if (search_record(BOARDS, &fh, sizeof(fh), cmpbnames, nboard) <= 0) {
		report("%s 讨论区找不到", nboard);
		return -1;
	}
*/
	strlcpy(quote_board, nboard, sizeof(quote_board));
	strlcpy(dbname, currboard, sizeof(dbname));
	strlcpy(currboard, nboard, sizeof(currboard));
	strlcpy(quote_file, filename, sizeof(quote_file));
	strlcpy(quote_title, posttitle, sizeof(quote_title));
	result = post_cross('l', mode);
	strlcpy(currboard, dbname, sizeof(currboard));
	return result;
}

int
postfile_cross(char *filename, char *qboard, char *nboard, char *posttitle)
{
	char dbname[BFNAMELEN + 1];

	strlcpy(quote_board, qboard, sizeof(quote_board));
	strlcpy(dbname, currboard, sizeof(dbname));
	strlcpy(currboard, nboard, sizeof(currboard));
	strlcpy(quote_file, filename, sizeof(quote_file));
	strlcpy(quote_title, posttitle, sizeof(quote_title));
	post_cross('l', 0);
	strlcpy(currboard, dbname, sizeof(currboard));
	return 0;
}

int
get_a_boardname(char *bname, char *prompt)
{
	struct boardheader fh;

	make_blist();
	namecomplete(prompt, bname);
	if (*bname == '\0') {
		return 0;
	}
	if (search_record(BOARDS, &fh, sizeof (fh), cmpbnames, bname) <= 0) {
		move(1, 0);
		prints("错误的讨论区名称\n");
		pressreturn();
		move(1, 0);
		return 0;
	}
	return 1;
}

#ifdef RECOMMEND
/* Add by Betterman 070423 */

int search_record_by_filename(void *filename, void *buf){
	// 现在的时间都是10位了的吧....
	return strcmp((char *)filename, ((struct fileheader *)buf)->filename)  ;
}

int get_origin_info(struct fileheader *fileinfo, int *origin_ent, 
								char *origin_bname, char *origin_fname, 
								struct fileheader *origin_fileinfo){
	FILE *inf;
	char filepath[STRLEN];
	char buf[8192];
	char *ptr, *ptr2;

	snprintf(filepath, sizeof(filepath), "boards/%s/%s", currboard, fileinfo->filename);
	if ((inf = fopen(filepath, "r")) == NULL) {
		report("get_origin_info: cannot open %bnames for reading", filepath);
		return 0;
	}

	while (fgets(buf, 256, inf) != NULL && buf[0] != '\n');
	fgets(buf, 256, inf);
	if ( (ptr = strstr(buf, "【 以下文字推荐自 ") ) && 
			(ptr2 = strstr(buf, "讨论区 】") )  ) {
		ptr += 23;
		ptr2 -= 6;
		if(ptr >= ptr2 || ptr - ptr2 >= STRLEN)
			return 0;
		memcpy(origin_bname, ptr, ptr2 - ptr);
		origin_bname[ptr2 - ptr] = '\0';

		fgets(buf, 256, inf);
		if(strlen(buf) < 47)
			return 0;
		if ((ptr = strstr(buf, "【 原文文件名")) != NULL) {
			ptr += 19;
			memcpy(origin_fname, ptr, FNAMELEN - 1);
			origin_fname[FNAMELEN - 1] = '\0';
			* strchr(origin_fname, ' ') = '\0';
		} else {
			return 0;
		}
	} else {
		return 0;
	}
	
	snprintf(buf, sizeof(buf), "boards/%s/.DIR", origin_bname);
	*origin_ent = search_record_bin(buf, origin_fileinfo, sizeof(*origin_fileinfo), 1, search_record_by_filename, origin_fname);

	if(*origin_ent == 0)
		return 0;
	return 1;
	
}

void
getrecommend(char *filepath, int mode)
{
	FILE *inf, *of;
	char buf[8192];
	char owner[248], *ptr;
	char filename[FNAMELEN];
	int count, owner_found = 0;

	modify_user_mode(POSTING);
	if ((inf = fopen(quote_file, "r")) == NULL) {
		report("getrecommend: cannot open %s for reading", quote_file);
		return;
	}

	if ((of = fopen(filepath, "w")) == NULL) {
		report("getrecommend: cannot open %s for writing", filepath);
		fclose(inf);
		return;
	}

	if (mode == 0) {
		write_header(of, 1 /* 不写入 .posts */, NA);
		if (fgets(buf, 256, inf) != NULL) {
			if ((ptr = strstr(buf, "信人: ")) == NULL) {
				owner_found = 0;
				strcpy(owner, "Unkown User");
			} else {
				ptr += 6;
				for (count = 0; ptr[count] != 0 && ptr[count] != ' ' && ptr[count] != '\n'; count++);

				if (count <= 1) {
					owner_found = 0;
					strcpy(owner, "Unkown User");
				} else {
					if (buf[0] == 27) {
						strcpy(owner, "自动发信系统");
					} else {
						strlcpy(owner, ptr, count + 1);
					}
					owner_found = 1;
				}
			}
		}

		strcpy(filename, strrchr(quote_file, '/') + 1);

		fprintf(of, "\033[1;37m【 以下文字推荐自 \033[32m%s \033[37m讨论区 】\n", quote_board);
		fprintf(of, "\033[1;37m【 原文文件名 \033[32m%s \033[37m 】\n", filename);
		fprintf(of, "\033[1;37m【 以下文字由 \033[32m%s \033[37m推荐 】\n",currentuser.userid);
	



		if (owner_found) {
			/* skip file header */
			while (fgets(buf, 256, inf) != NULL && buf[0] != '\n');
			fgets(buf, 256, inf);
			if ((strstr(buf, "【 以下文字转载自 ") && strstr(buf, "讨论区 】"))) {
				fgets(buf, 256, inf); 
				if (strstr(buf, "【 原文由") && strstr(buf, "所发表 】")) {
					fputs(buf, of);
				} else {
					fprintf(of, "【 原文由\033[32m %s\033[37m 所发表 】\033[m\n\n", owner);
				}
			} else {
				fprintf(of, "【 原文由\033[32m %s\033[37m 所发表 】\033[m\n\n", owner);
				fputs(buf, of);
			}
		} else {
			fprintf(of, "\033[m");
			fseek(inf, 0, SEEK_SET);
		}
	}
	while ((count = fread(buf, 1, sizeof(buf), inf)) > 0)
		fwrite(buf, 1, count, of);

	if (!mode) {			// 推荐记录
		fprintf(of, "--\n\033[m\033[1;%2dm※ 推荐:.%s %s.[FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, fromhost);
	}

	fclose(of);
	fclose(inf);
	quote_file[0] = '\0';
}

int post_recommend(struct fileheader *fileinfo, int mode){
	struct fileheader postfile;
	struct boardheader *bp;
	char bdir[STRLEN], filepath[STRLEN];
	char buf[256], whopost[IDLEN + 2];
	if (!haspostperm(currboard) && !mode) {
		outs("\n\n您尚无权限推荐文章，取消推荐\n");
		return -1;
	}

	memset(&postfile, 0, sizeof (postfile));

	if (!mode && strncmp(quote_title, "[推荐]", 6)) {
		snprintf(save_title, sizeof(save_title), "[推荐] %s", quote_title);
	} else {
		strlcpy(save_title, quote_title, sizeof(save_title));
	}

	snprintf(bdir, sizeof(bdir), "boards/%s", currboard);
	if ((getfilename(bdir, filepath, GFN_FILE | GFN_UPDATEID, &postfile.id)) == -1)
		return -1;
	strcpy(postfile.filename, strrchr(filepath, '/') + 1);

	if (mode == 1) {
		strcpy(whopost, BBSID);
		postfile.flag |= FILE_MARKED;
	} else {
		FILE* inf;
		char *ptr;
		int owner_found = 0, count = 0;
		if ((inf = fopen(quote_file, "r")) == NULL) {
			report("getrecommend: cannot open %s for reading", quote_file);
		} else {
			if (fgets(buf, 256, inf) != NULL) {
				if ((ptr = strstr(buf, "信人: ")) != NULL) {
					ptr += 6;
					for (count = 0; ptr[count] != 0 && ptr[count] != ' ' && ptr[count] != '\n'; count++);
					if (count > 1) 
						owner_found = 1;
				}
			}
		}
		if (owner_found) { /* 被转载再推荐时获取原作者 */
			/* skip file header */
			while (fgets(buf, 256, inf) != NULL && buf[0] != '\n');
			fgets(buf, 256, inf);
			if ((strstr(buf, "【 以下文字转载自 ") && strstr(buf, "讨论区 】"))) {
				fgets(buf, 256, inf); 
				if ((ptr = strstr(buf, "【 原文由")) && strstr(buf, "所发表 】")) {
					ptr = ptr + 15;
					for (count = 0; ptr[count] != '\033'; count++);
					strlcpy(whopost, ptr, count + 1);
				} else strcpy(whopost, fileinfo->owner);
			} else strcpy(whopost, fileinfo->owner);
		} else strcpy(whopost, fileinfo->owner);
	}

	strlcpy(postfile.owner, whopost, sizeof(postfile.owner));
	strlcpy(postfile.title, save_title, sizeof(postfile.title));
//      setboardfile(filepath, currboard, postfile.filename);

	bp = getbcache(currboard);
	local_article = YEA;

	getrecommend(filepath, mode);

	postfile.flag &= ~FILE_OUTPOST;

	setbdir(buf, currboard);
	postfile.filetime = time(NULL);
	if (append_record(buf, &postfile, sizeof(postfile)) == -1) {
		unlink(postfile.filename);      // monster: remove file on failure
		if (!mode) {
			report("recommend_posting '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		} else {
			report("Recommend '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		}
		pressreturn();
		clear();
		return -1;
	}

	if (!mode) {
		report("recommend_posted '%s' on '%s'", postfile.title,
			currboard);
	}

	update_lastpost(currboard);
	update_total_today(currboard);	
	return 1;
}


int do_recommend(int ent, struct fileheader *fileinfo, char *direct)
{
	char dbname[STRLEN];
	char isrecommend[10];	
	int old_digestmode;
	int origin_ent;
	char origin_bname[STRLEN];
	char origin_fname[FNAMELEN];
	struct fileheader origin_fileinfo;
	struct boardheader *bp, *recommend_bp;

	if (digestmode > 1)
		return DONOTHING;
	old_digestmode = digestmode;
	digestmode = 0;

	set_safe_record();

#ifdef AUTHHOST
	if ( !HAS_ORGPERM(PERM_WELCOME) && !HAS_ORGPERM(PERM_SYSOP) &&
             !HAS_ORGPERM(PERM_OBOARDS) && !HAS_ORGPERM(PERM_ACBOARD) &&
             !HAS_ORGPERM(PERM_ACHATROOM) )
		return DONOTHING;
#endif

	/* 检查当前用户模式 */
	if (uinfo.mode != RMAIL) {
		snprintf(genbuf, sizeof(genbuf), "boards/%s/%s", currboard, fileinfo->filename);
	} else {
		move(7, 0);
		outs("只能推荐版面上的文章");
		pressreturn();
		return FULLUPDATE;
	}
	strlcpy(quote_file, genbuf, sizeof(quote_file));
	strlcpy(quote_title, fileinfo->title, sizeof(quote_title));

	if ((recommend_bp = getbcache(DEFAULTRECOMMENDBOARD)) == NULL)
	    return FULLUPDATE;

        if (strcmp(currboard, DEFAULTRECOMMENDBOARD) != 0 && !current_bm && 
	    !check_bm(currentuser.userid, recommend_bp->BM)) {
		clear();
                move(7,22);
	        outs("对不起，你没有权限进行推荐。");
	        pressreturn();
	        return FULLUPDATE;
        }
    

	if (!strcmp(currboard, DEFAULTRECOMMENDBOARD)) {
        	if(strncmp(quote_title, "[推荐]", 6)){
			return DONOTHING;
		}
		if(get_origin_info(fileinfo, &origin_ent, 
							origin_bname, origin_fname, &origin_fileinfo) == 0){
			presskeyfor("对不起，原文已丢失或没有原文。\n");
			return FULLUPDATE;
		}

		if ((bp = getbcache(origin_bname)) == NULL)
			return FULLUPDATE;
		if (bp->flag & (BRD_RESTRICT | BRD_NOPOSTVOTE)) /* 防止跳进本身不能看到的版面  */
			return FULLUPDATE;

		return do_select2(ent, fileinfo, direct, origin_bname, origin_ent);
	}

	clear();
        outs("\n\n");
	outs("\033[1;35m          欢迎使用首页推荐功能！\033[m\n\n");
	outs("\033[1;37m    您的操作将把文章推荐到\033[m\033[1;33mRecommend\033[m\033[1;37m版\n\n");
	outs("    在\033[m\033[1;33mRecommend\033[m版被收入文摘区的文章将显示在web首页。\n\n");
	outs("    请大家\033[m\033[1;31m慎用\033[m\033[1;37m推荐权，扰乱版面秩序的将被封禁取消推荐权。\n\n");
	outs("    是否继续？\033[m\n\n");

	if ((bp = getbcache(currboard)) == NULL)
		return DONOTHING;
	if (bp->flag & (BRD_RESTRICT | BRD_NOPOSTVOTE)){
	        clear();
		move(12,21);
		outs("对不起，不能推荐限制版面文章\n");
		pressreturn();
		return FULLUPDATE;
	}
        if (bp->flag & BRD_INTERN) {
	        clear();
	        move(12,22);
	        outs("对不起，不能推荐对外屏蔽版面文章\n");
	        pressreturn();
	        return FULLUPDATE;
        }

	if (!check_max_post(DEFAULTRECOMMENDBOARD))
		return FULLUPDATE;

	#ifdef FILTER
	if (!has_filter_inited()) {
		init_filter();
	}
	if (!regex_strstr(currentuser.username))
	{
		move(12,0); 
		outs("对不起，你的昵称中含有不合适的内容，不能推荐，请先进行修改\n");
		pressreturn();
		return FULLUPDATE;
	}
	#endif

	move(12, 0);
	if (fileinfo->flag & FILE_RECOMMENDED) 
		prints("\033[1;32m  本文已被推荐到 %s 版, 是否继续推荐 \033[m",  DEFAULTRECOMMENDBOARD);
	else
		prints("\033[1m    推荐 ' %s ' 到 %s 版 \033[m",  quote_title, DEFAULTRECOMMENDBOARD);
	getdata(14, 0, "\033[1m    (S)确认  (A)取消? [S]\033[m: ", isrecommend, 9, DOECHO, YEA);
	if (isrecommend[0] == 'a' || isrecommend[0] == 'A') {
		outs("取消");
	} else {
		strlcpy(quote_board, currboard, sizeof(quote_board));
		strlcpy(dbname, currboard, sizeof(dbname));
		strlcpy(currboard, DEFAULTRECOMMENDBOARD, sizeof(currboard));
		if (post_recommend(fileinfo, 0) == -1) {
			pressreturn();
			strlcpy(currboard, dbname, sizeof(currboard));
			return FULLUPDATE;
		}
		strlcpy(currboard, dbname, sizeof(currboard));
		fileinfo->flag |= FILE_RECOMMENDED;
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 2) ? NA : YEA );
		prints("\n已把文章 \'%s\' 推荐到 %s 版\n", quote_title, DEFAULTRECOMMENDBOARD);
	}

	digestmode = old_digestmode;
	pressreturn();
	return FULLUPDATE;
}
#endif

/* Add by SmallPig */
int
do_cross(int ent, struct fileheader *fileinfo, char *direct)
{
	char bname[STRLEN];
	char dbname[STRLEN];
	char ispost[10];
	int old_digestmode;

#ifdef BM_CROSSPOST
	if (uinfo.mode != RMAIL) {
		if (!HAS_PERM(PERM_BOARDS) && strcmp(currboard, "Post"))
#ifdef INTERNET_EMAIL
			return forward_post(ent, fileinfo, direct);
#else
			return DONOTHING;
#endif
	}
#endif

	if (digestmode > 7)
		return DONOTHING;
	old_digestmode = digestmode;
	digestmode = 0;

	set_safe_record();

#ifdef AUTHHOST
	if ( !HAS_ORGPERM(PERM_WELCOME) && !HAS_ORGPERM(PERM_SYSOP) &&
             !HAS_ORGPERM(PERM_OBOARDS) && !HAS_ORGPERM(PERM_ACBOARD) &&
             !HAS_ORGPERM(PERM_ACHATROOM) )
		return DONOTHING;
#endif

	if (uinfo.mode != RMAIL) {
		snprintf(genbuf, sizeof(genbuf), "boards/%s/%s", currboard, fileinfo->filename);
	} else {
		snprintf(genbuf, sizeof(genbuf), "mail/%c/%s/%s",
			mytoupper(currentuser.userid[0]), currentuser.userid,
			fileinfo->filename);
	}
	strlcpy(quote_file, genbuf, sizeof(quote_file));
	strlcpy(quote_title, fileinfo->title, sizeof(quote_title));

	clear();

	outs("请珍惜资源，同样内容的文章请勿张贴（包括转载）\033[1;31m4\033[m篇或以上。\n");
	outs("如确有需要，请到\033[1;33msuggest\033[m版发文申请，经同意后才能一文多发。\n");
	outs("\033[1;32m违者将受到封禁全站发文权的处罚，情节严重者将会被删除帐号。\033[m\n");
	outs("详细规定请参照相关站规。 谢谢合作!\n\n");

	if (!get_a_boardname(bname, "您如果确定要转载的话，请输入要转贴的讨论区名称(取消转载请按回车): "))
		return FULLUPDATE;
	
	if (YEA == check_readonly(bname)) {
		move(7, 0);
		outs("对不起，您不能转贴文章到只读版面上。");
		pressreturn();
		return FULLUPDATE;
	}
	
	if (!strcmp(bname, DEFAULTRECOMMENDBOARD)) {
		move(7, 0);
		outs("对不起，你不能转贴文章到推荐版面。");
		pressreturn();
		return FULLUPDATE;
	}

	if (!strcmp(bname, currboard) && uinfo.mode != RMAIL) {
		move(7, 0);
		outs("对不起，本文就在您要转载的版面上，所以无需转载。");
		pressreturn();
		return FULLUPDATE;
	}

	if (!check_max_post(bname))
		return FULLUPDATE;

	#ifdef FILTER
	if (!has_filter_inited()) {
		init_filter();
	}
	if (!regex_strstr(currentuser.username))
	{
		move(7,0);
		outs("对不起，你的昵称中含有不合适的内容，不能发文，请先进行修改\n");
		pressreturn();
		return FULLUPDATE;
	}
	#endif

	move(7, 0);
	prints("转载 ' %s ' 到 %s 版 ", quote_title, bname);
	getdata(9, 0, "(S)转信 (L)本站 (A)取消? [L]: ", ispost, 9, DOECHO, YEA);
	if (ispost[0] == 'a' || ispost[0] == 'A') {
		outs("取消");
	} else {
		quote_fh = fileinfo;
		strlcpy(quote_board, currboard, sizeof(quote_board));
		strlcpy(dbname, currboard, sizeof(dbname));
		strlcpy(currboard, bname, sizeof(currboard));
		if (ispost[0] != 's' && ispost[0] != 'S')
			ispost[0] = 'L';
		if (post_cross(ispost[0], 0) == -1) {
			pressreturn();
			strlcpy(currboard, dbname, sizeof(currboard));
			return FULLUPDATE;
		}
		strlcpy(currboard, dbname, sizeof(currboard));
		prints("\n已把文章 \'%s\' 转贴到 %s 版\n", quote_title, bname);
        
	}
	digestmode = old_digestmode;
	pressreturn();
	return FULLUPDATE;
}

/* show the first 3 lines */
void
readtitle(void)
{
	struct boardheader *bp;
	int i, j, bnum, tuid;
	struct user_info uin;
	char *currBM;
	char tmp[40], bmlists[3][IDLEN + 2];
	char header[STRLEN], title[STRLEN];
	char readmode[11];

	bp = getbcache(currboard);
	current_bm = (HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
		      check_bm(currentuser.userid, bp->BM)) ||
		      HAS_PERM(PERM_BLEVELS);

	currBM = bp->BM;
	for (i = 0, j = 0, bnum = 0; currBM[i] != '\0' && bnum < 3; i++) {
		if (currBM[i] == ' ') {
			bmlists[bnum][j] = '\0';
			bnum++;
			j = 0;
		} else {
			bmlists[bnum][j++] = currBM[i];
		}
	}
	bmlists[bnum][j] = '\0';
	if (currBM[0] == '\0' || currBM[0] == ' ') {
		strcpy(header, "诚征版主中");
	} else {
		strcpy(header, "版主: ");
		for (i = 0; i <= bnum; i++) {
			tmp[0] = '\0';
			tuid = getuser(bmlists[i], NULL);
			search_ulist(&uin, t_cmpuids, tuid);			
			if (uin.active && uin.pid && !uin.invisible) {
				snprintf(tmp, sizeof(tmp), "\033[32m%s\033[33m ", bmlists[i]); /* green bm */
			} else if (uin.active && uin.pid && uin.invisible && (HAS_PERM(PERM_SEECLOAK) || usernum == uin.uid)) {
				snprintf(tmp, sizeof(tmp), "\033[36m%s\033[33m ", bmlists[i]); /* blue bm */
			} else {
				snprintf(tmp, sizeof(tmp), "%s ", bmlists[i]);
			}
			strlcat(header, tmp, sizeof(header));
		}
	}

	if (chkmail(NA)) {
		strcpy(title, "[您有信件，按 M 看新信]");
	} else if ((bp->flag & VOTE_FLAG)) {
		snprintf(title, sizeof(title), "※投票中,按 v 进入投票※");
	} else {
		strcpy(title, bp->title + 8);
	}

	showtitle(header, title);
	if (digestmode == 8 || digestmode == 9) {
		prints("离开[\033[1;32m←\033[m,\033[1;32mq\033[m] 选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 阅读[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 恢复文章[\033[1;32my\033[m] 清空[\033[1;32mE\033[m] 备忘录[\033[1;32mTAB\033[m] 求助[\033[1;32mh\033[m]     \n");
	} else if (digestmode == 10 || digestmode == 11) {
		prints("离开[\033[1;32m←\033[m,\033[1;32mq\033[m] 选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 阅读[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 封禁[\033[1;32ma\033[m] 修改封禁[\033[1;32mE\033[m] 解除封禁[\033[1;32md\033[m] 求助[\033[1;32mh\033[m] \n");
	} else {
		prints("离开[\033[1;32m←\033[m,\033[1;32mq\033[m] 选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 阅读[\033[1;32m→\033[m,\033[1;32mRtn\033[m] 发表文章[\033[1;32mCtrl-P\033[m] 砍信[\033[1;32md\033[m] 备忘录[\033[1;32mTAB\033[m] 求助[\033[1;32mh\033[m]\n");
	}

	if (digestmode == 0) {
		if (DEFINE(DEF_THESIS)) { /* youzi 1997.7.8 */
			strcpy(readmode, "主题");
		} else {
			strcpy(readmode, "一般");
		}
	} else if (digestmode == 1) {
		strcpy(readmode, "文摘");
	} else if (digestmode == 2) {
		strcpy(readmode, "主题");
	} else if (digestmode == 3) {
		strcpy(readmode, "MARK");
	} else if (digestmode == 4) {
		strcpy(readmode, "原作");
	} else if (digestmode == 7) {
		strcpy(readmode, "标题关键字");
	}


#ifdef INBOARDCOUNT
	int idx = getbnum(currboard);
	int inboard = board_setcurrentuser(idx-1, 0);

	if (DEFINE(DEF_THESIS) && digestmode == 0) {
		prints("\033[1;37;44m 编号   %-12s %6s %-18s %14s%4d [%4s式看版] \033[m\n",
		       "刊 登 者", "日  期", " 标  题", "在线:", inboard, readmode);
	} else if (digestmode == 5 || digestmode == 6) { /* 模糊 or 精确同作者阅读 */
		prints("\033[1;37;44m 编号   %-12s %6s %-10s (关键字: \033[32m%-12s\033[37m) [\033[33m%s\033[37m同作者阅读] \033[m\n", "刊 登 者", "日  期", " 标  题", someoneID, (digestmode == 5) ? "模糊" : "精确");
	} else if (digestmode == 7) {
		prints("\033[1;37;44m 编号   %-12s %6s %-18s %9s%4d [%10s式看版]\033[m\n",
		       "刊 登 者", "日  期", " 标  题", "在线:", inboard, readmode);
	} else if (digestmode == 8) {
		prints("\033[1;37;44m 编号   %-12s %6s %-42s[回收站] \033[m\n",
		       "刊 登 者", "日  期", " 标  题");
	} else if (digestmode == 9) {
		prints("\033[1;37;44m 编号   %-12s %6s %-42s[废纸篓] \033[m\n",
		       "刊 登 者", "日  期", " 标  题");
	} else if (digestmode == 10) {
		prints("\033[1;37;44m 编号   %-12s %6s %-40s[封禁列表] \033[m\n",
		       "执 行 者", "日  期", " 封 禁 原 因");
	} else if (digestmode == 11) {
		prints("\033[1;37;44m 编号   %-12s %6s %-40s[封禁列表] \033[m\n",
		       "封 禁 者", "日  期", " 封 禁 原 因");
	} else {
		prints("\033[1;37;44m 编号   %-12s %6s %-18s %14s%4d [%4s模式]   \033[m\n",
		       "刊 登 者", "日  期", " 标  题", "在线:", inboard, readmode);
	}
#else


	if (DEFINE(DEF_THESIS) && digestmode == 0) {
		prints("\033[1;37;44m 编号   %-12s %6s %-38s[%4s式看版] \033[m\n",
		       "刊 登 者", "日  期", " 标  题", readmode);
	} else if (digestmode == 5 || digestmode == 6) {
		prints("\033[1;37;44m 编号   %-12s %6s %-10s (关键字: \033[32m%-12s\033[37m) [\033[33m%s\033[37m同作者阅读] \033[m\n",
		       "刊 登 者", "日  期", " 标  题", someoneID,
		       (digestmode == 5) ? "模糊" : "精确");
	} else if (digestmode == 7) {
		prints("\033[1;37;44m 编号   %-12s %6s %-32s[%10s式看版] \033[m\n",
		       "刊 登 者", "日  期", " 标  题", readmode);
	} else if (digestmode == 8) {
		prints("\033[1;37;44m 编号   %-12s %6s %-42s[回收站] \033[m\n",
		       "刊 登 者", "日  期", " 标  题");
	} else if (digestmode == 9) {
		prints("\033[1;37;44m 编号   %-12s %6s %-42s[废纸篓] \033[m\n",
		       "刊 登 者", "日  期", " 标  题");
	} else if (digestmode == 10) {
		prints("\033[1;37;44m 编号   %-12s %6s %-40s[封禁列表] \033[m\n",
		       "执 行 者", "日  期", " 封 禁 原 因");
	} else if (digestmode == 11) {
		prints("\033[1;37;44m 编号   %-12s %6s %-40s[封禁列表] \033[m\n",
		       "封 禁 者", "日  期", " 封 禁 原 因");
	} else {
		prints("\033[1;37;44m 编号   %-12s %6s %-40s[%4s模式] \033[m\n",
		       "刊 登 者", "日  期", " 标  题", readmode);
	}
#endif
	clrtobot();
}

/* monster: 文章列表 (同主题模式） */
char *
readdoent_thread(int num, struct fileheader *ent, char mark2[], char *date, time_t filetime, int type)
{
	static char buf[128];
	const char tchar[3][3] = { "●", "└", "├" };
	char *title = NULL;

#ifdef COLOR_POST_DATE
	struct tm *mytm;
	char color[8] = "\033[1;30m";
#endif

#ifdef COLOR_POST_DATE
	mytm = localtime(&filetime);
	color[5] = mytm->tm_wday + 49;

	if (ent->title[0] == '\0') {
		snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s● %-.45s\033[m ",
			num, type, BBSID, color, date, mark2, "< 此文已缺失，请删除 >");

		return buf;
	}

	if (FFLL == 0) {
		if (!strncmp("Re: ", ent->title, 4)) {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%s %-.46s\033[m ",
				num, type, ent->owner, color, date, mark2,
				tchar[(int)ent->reserved[0]], ent->title + 4);
		} else {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%s %-.43s\033[m ",
				num, type, ent->owner, color, date, mark2,
				tchar[(int)ent->reserved[0]], ent->title);
		}
	} else {
		if (ReadID == ent->id) {
			if (ent->reserved[0] == THREAD_BEGIN) {
				snprintf(buf, sizeof(buf), " \033[1;32m%4d\033[m %c %-12.12s %s%6.6s.%s\033[1;32m%s %-.44s\033[m ",
					num, type, ent->owner, color, date, mark2,
					tchar[(int)ent->reserved[0]], ent->title);
			} else {
				if (!strncmp(ent->title, "Re: ", 4)) {
					title = ent->title + 4;
				} else {
					title = ent->title;
				}

				snprintf(buf, sizeof(buf), " \033[1;36m%4d\033[m %c %-12.12s %s%6.6s.%s\033[1;36m%s %-.44s\033[m ",
					num, type, ent->owner, color, date, mark2,
					tchar[(int)ent->reserved[0]], title);
			}
		} else {
			if (strncmp(ent->title, "Re: ", 4) || ent->reserved[0] == THREAD_BEGIN) {
				title = ent->title;
			} else {
				title = ent->title + 4;
			}

			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%s %-.44s ",
				num, type, ent->owner, color, date, mark2,
				tchar[(int)ent->reserved[0]], title);
		}
	}
#else
	if (ent->title[0] == '\0') {
		snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s● %-.45s\033[m ",
			num, type, BBSID, date, mark2, "< 此文已缺失，请删除 >");

		return buf;
	}

	if (FFLL == 0) {
		if (!strncmp("Re: ", ent->title, 4)) {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%s %-.46s\033[m ",
				num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]], ent->title + 4);
		} else {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%s %-.43s\033[m ",
				num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]], ent->title);
		}
	} else {
		if (ReadID == ent->id) {
			if (ent->reserved[0] == THREAD_BEGIN) {
				snprintf(buf, sizeof(buf), " \033[1;32m%4d\033[m %c %-12.12s %6.6s.%s\033[1;32m%s %-.44s\033[m ",
					num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]],
					ent->title);
			} else {
				if (!strncmp(ent->title, "Re: ", 4)) {
					title = ent->title + 4;
				} else {
					title = ent->title;
				}

				snprintf(buf, sizeof(buf), " \033[1;36m%4d\033[m %c %-12.12s %6.6s.%s\033[1;36m%s %-.44s\033[m ",
					num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]], title);
			}
		} else {
			if (strncmp(ent->title, "Re: ", 4) || ent->reserved[0] == THREAD_BEGIN) {
				title = ent->title;
			} else {
				title = ent->title + 4;
			}

			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%s %-.44s ",
				num, type, ent->owner, date, mark2, tchar[(int)ent->reserved[0]], title);
		}
	}
#endif

	return buf;
}

char *
readdoent(int num, void *ent_ptr)             // 文章列表
{
	static char buf[128], mark2[16];
	struct boardheader *bp;
	char *date, *owner;
	int type;
	struct fileheader *ent = (struct fileheader *)ent_ptr;

#ifdef COLOR_POST_DATE
	struct tm *mytm;
	char color[8] = "\033[1;30m";
#endif

	type = brc_unread(ent->filetime) ? 'N' : ' ';
	if ((ent->flag & FILE_SELECTED) && (current_bm || HAS_PERM(PERM_ANNOUNCE))) {
		type = '$';
		goto skip;
	}
	if ((ent->flag & FILE_DIGEST) /* && HAS_PERM(PERM_MARKPOST) */ ) {
		if (type == ' ')
			type = 'g';
		else
			type = 'G';
	}
	if (ent->flag & FILE_MARKED) {
		switch (type) {
		case ' ':
			type = 'm';
			break;
		case 'N':
			type = 'M';
			break;
		case 'g':
			type = 'b';
			break;
		case 'G':
			type = 'B';
			break;
		}
	}

	/* monster: disable x-mark display
	 *  if(ent->flag & FILE_DELETED  && current_bm)
	 *          type = (brc_unread(ent->filename) ? 'X' ? 'x';
	 */

      skip:
	date = ctime(&ent->filetime) + 4;

	/* monster: 日期后标题前的标记优先权：@ (附件），x （不可回复） */
	if (ent->flag & FILE_ATTACHED) {
		strcpy(mark2, "\033[1;33m@\033[m");
	} else {
		bp = getbcache(currboard);

		if (ent->flag & FILE_NOREPLY || bp->flag & NOREPLY_FLAG) {
			strcpy(mark2, "\033[0;1;4;33mx\033[m");
		} else {
			mark2[0] = ' ';
			mark2[1] = 0;
		}
	}

	if (digestmode == 2) { /* 主题模式 */
		return readdoent_thread(num, ent, mark2, date, ent->filetime, type);
	}

	/* monster: here 'owner' means 'blacklist', 'realowner' means 'executive' when in denylist */
	owner = (digestmode != 11) ? ent->owner : ent->realowner;

#ifdef COLOR_POST_DATE
	mytm = localtime(&ent->filetime);
	color[5] = mytm->tm_wday + 49;

	if (ent->title[0] == '\0') {
		snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s● %-.45s\033[m ",
			num, type, BBSID, color, date, mark2, "< 此文已缺失，请删除 >");

		return buf;
	}

	if (FFLL == 0) {
		if (!strncmp("Re: ", ent->title, 4)) {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%-.48s\033[m ",
				num, type, owner, color, date, mark2, ent->title);
		} else {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s● %-.45s\033[m ",
				num, type, owner, color, date, mark2, ent->title);
		}
	} else {
		if (!strncmp("Re: ", ent->title, 4)) {
			if (ReadID == ent->id) { /* 当前在读主题 */
				snprintf(buf, sizeof(buf), " \033[1;36m%4d\033[m %c %-12.12s %s%6.6s.%s\033[1;36m%-.48s\033[m ",
					num, type, owner, color, date, mark2, ent->title);
			} else {
				snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s%-.48s ",
					num, type, owner, color, date, mark2, ent->title);
			}
		} else {
			if (ReadID == ent->id) {
				snprintf(buf, sizeof(buf), " \033[1;32m%4d\033[m %c %-12.12s %s%6.6s.%s\033[1;32m● %-.45s\033[m ",
					num, type, owner, color, date, mark2, ent->title);
			} else {
				snprintf(buf, sizeof(buf), " %4d %c %-12.12s %s%6.6s\033[m %s● %-.45s\033[m ",
					num, type, owner, color, date, mark2, ent->title);
			}
		}
	}
#else
	if (ent->title[0] == '\0') {
		snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s● %-.45s\033[m ",
			num, type, BBSID, date, mark2, "< 此文已缺失，请删除 >");

		return buf;
	}

	if (FFLL == 0) {
		if (!strncmp("Re: ", ent->title, 4)) {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%-.48s\033[m ",
				num, type, owner, date, mark2, ent->title);
		} else {
			snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s● %-.45s\033[m ",
				num, type, owner, date, mark2, ent->title);
		}
	} else {
		if (!strncmp("Re: ", ent->title, 4)) {
			if (ReadID == ent->id) {
				snprintf(buf, sizeof(buf), " \033[1;36m%4d\033[m %c %-12.12s %6.6s.%s\033[1;36m%-.48s\033[m ",
					num, type, owner, date, mark2, ent->title);
			} else {
				snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s%-.48s ",
					num, type, owner, date, mark2, ent->title);
			}
		} else {
			if (ReadID == ent->id) {
				snprintf(buf, sizeof(buf), " \033[1;32m%4d\033[m %c %-12.12s %6.6s.%s\033[1;32m● %-.45s\033[m ",
					num, type, owner, date, mark2, ent->title);
			} else {
				snprintf(buf, sizeof(buf), " %4d %c %-12.12s %6.6s\033[m %s● %-.45s\033[m ",
					num, type, owner, date, mark2, ent->title);
			}
		}
	}
#endif

	return buf;
}

int
cmpfilename(struct fileheader *fhdr, char *filename)
{
	return (!strncmp(fhdr->filename, filename, sizeof(fhdr->filename))) ? YEA : NA;
}

int
cmpfilename2(void *filename, void *fhdr_ptr)
{
	struct fileheader *fhdr = (struct fileheader *)fhdr_ptr;

	return (!strcmp(fhdr->filename, (char *)filename)) ? YEA : NA;
}

/* freestyler:  获取附件信息到全局变量 attach_info, attach_link
 * return 0 如果没附件 */
int
getattachinfo(struct fileheader *fileinfo)
{
	struct boardheader* bh = getbcache(quote_board);
	if (bh && (bh->flag & BRD_ATTACH))  { /* 版面可上传附件 */
		if (fileinfo->flag & FILE_ATTACHED) { /* 有附件 */
			struct attacheader ah;
			char	afname[BFNAMELEN];
			strcpy(afname, fileinfo->filename);
			afname[0] = 'A';
			if (getattach(quote_board, afname, &ah)) {
				char 	buf[40];
				struct  stat st;
				snprintf(buf, sizeof(buf), "attach/%s/%s", quote_board, ah.filename);
				if (lstat(buf, &st) == -1)  return 0;
				snprintf(attach_info, sizeof(attach_info),
					"附件: %s (%d KB)", ah.origname, (int)st.st_size/1024);
				
				snprintf(attach_link, sizeof(attach_link), 
					 "http://%s/attach/%s/%d.%s", BBSHOST, quote_board, atoi(ah.filename + 2), ah.filetype );
				return 1;
			}
		}
	}
	return 0;
}

int
read_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char *t;
	char buf[512], article_link[1024];
	int ch, result;
	struct fileheader header;

	clear();
	brc_addlist(fileinfo->filetime);
	strcpy(buf, direct); /*  buf is like "boards/Joke/.DIR" */
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	snprintf(genbuf, sizeof(genbuf), "%s/%s", buf, fileinfo->filename);
	if (!dashf(genbuf)) {
		clear();
		move(10, 30);
		prints("对不起，本文内容丢失！");
		pressanykey();
		return FULLUPDATE;      //deardragon 0729
	}
	strlcpy(quote_file, genbuf, sizeof(quote_file));
	strcpy(quote_board, currboard);
	strcpy(quote_title, fileinfo->title);
	strcpy(quote_user, fileinfo->owner);


	snprintf(article_link, sizeof(article_link), 	/* 全文链接 */
				"http://%s/bbscon?board=%s&file=%s",
				BBSHOST, currboard, fileinfo->filename);

	if (getattachinfo(fileinfo)) {
#ifndef NOREPLY
		ch = ansimore4(genbuf, attach_info, attach_link, article_link, NA);
#else
		ch = ansimore4(genbuf, attach_info, attach_link, article_link, YEA);
#endif
	} else {
#ifndef NOREPLY
		ch = ansimore4(genbuf, NULL, NULL, article_link, NA);
#else
		ch = ansimore4(genbuf, NULL, NULL, article_link, YEA);
#endif
	}

#ifndef NOREPLY
	clear_line(t_lines - 1);
	if (haspostperm(currboard)) {
		prints("\033[1;44;31m[阅读文章]  \033[33m回信 R │ 结束 Q,← │上一封 ↑│下一封 <Space>,↓│主题阅读 ^X或p \033[m");
	} else {
		prints("\033[1;44;31m[阅读文章]  \033[33m结束 Q,← │上一封 ↑│下一封 <Space>,<Enter>,↓│主题阅读 ^X 或 p \033[m");
	}

	/* Re-Write By Excellent */

	FFLL = 1;
	ReadID = fileinfo->id;

	refresh();
	if (!(ch == KEY_RIGHT || ch == KEY_UP || ch == KEY_PGUP))
		ch = egetch();

	switch (ch) {
	case 'N':
	case 'Q':
	case 'n':
	case 'q':
	case KEY_LEFT:
		break;
	case ' ':
	case 'j':
	case KEY_RIGHT:
		if (DEFINE(DEF_THESIS)) {       /* youzi */
			sread(NA, NA, ent, fileinfo);
			break;
		} else
			return READ_NEXT;
	case KEY_DOWN:
	case KEY_PGDN:
		return READ_NEXT;
	case KEY_UP:
	case KEY_PGUP:
		return READ_PREV;
	case 'Y':
	case 'R':
	case 'y':
	case 'r':
		{
			struct boardheader *bp;

			bp = getbcache(currboard);
			noreply = (fileinfo->flag & FILE_NOREPLY) || (bp->flag & NOREPLY_FLAG);

			if (!noreply || HAS_PERM(PERM_SYSOP) || current_bm || isowner(&currentuser, fileinfo)) {
				local_article = (fileinfo->flag & FILE_OUTPOST) ? NA : YEA;
				postandmail = (fileinfo->flag & FILE_MAIL) ? 1 : 0;
				do_reply(fileinfo->title, fileinfo->owner, fileinfo->id);
			} else {
				clear();
				prints("\n\n    对不起, 该文章有不可 RE 属性, 你不能回复(RE) 这篇文章.    ");
				pressreturn();
				clear();
			}
		}
		break;
	case Ctrl('R'):
		post_reply(ent, fileinfo, direct);
		break;
	case 'g':
		digest_post(ent, fileinfo, direct);
		break;
	case Ctrl('U'):
		sread(YEA, YEA, ent, fileinfo);
		break;
	case Ctrl('N'):
		if ((result = locate_article(direct, &header, ent, LOCATE_THREAD | LOCATE_NEW | LOCATE_NEXT, &fileinfo->id)) != -1)
			sread(YEA, NA, ent, &header);
		break;
	case Ctrl('S'):
	case Ctrl('X'):
	case 'p':               /* Add by SmallPig */
		sread(NA, NA, ent, fileinfo);
		break;
	case Ctrl('A'): /* Add by SmallPig */
		clear();
		show_author(0, fileinfo, '\0');
		return READ_NEXT;
		break;
	case 'S':               /* by youzi */
		if (!HAS_PERM(PERM_MESSAGE))
			break;
		clear();
		s_msg();
		break;
	default:
		break;
	}
#endif
	return FULLUPDATE;
}

int
skip_post(int ent, struct fileheader *fileinfo, char *direct)
{
	brc_addlist(fileinfo->filetime);
	return GOTO_NEXT;
}

int
do_select(int ent, struct fileheader *fileinfo, char *direct)
{
	char bname[BFNAMELEN];
	struct boardheader *bp;
	int page, tmp, oldlevel;

	clear_line(0);
	prints("选择一个讨论区 (英文字母大小写皆可)\n");
	prints("输入讨论区名 (按空白键自动搜寻): ");
	clrtoeol();

	make_blist();
	namecomplete(NULL, bname);
	if (*bname == '\0')
		return FULLUPDATE;

	if ((bp = getbcache(bname)) == NULL)
		goto error;

	if (bp->flag & BRD_GROUP) {
		if ((oldlevel = setboardlevel(bname)) == -1)
			goto error;
#ifdef INBOARDCOUNT
		/* inboard user count  by freestyler */
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
#endif
		brdnum = -1;
		choose_board(YEA);	/* monster: bug here, we did not use original value of newflag here */
		boardlevel = oldlevel;
		brdnum = -1;		/* monster: refresh board list */
		return DOQUIT;
	} else {
		if (digestmode > 1) 
			digestmode = 0;
#ifdef INBOARDCOUNT
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
#endif
		brc_initial(bname);

#ifdef INBOARDCOUNT
		idx = getbnum(currboard);
		board_setcurrentuser(idx-1, 1);
#endif
		setbdir(direct, currboard);
		current_bm = (HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
			      check_bm(currentuser.userid, bp->BM)) ||
			      HAS_PERM(PERM_BLEVELS);

		if (DEFINE(DEF_FIRSTNEW)) {
			tmp = unread_position(direct, bp);
			page = tmp - t_lines / 2;
			getkeep(direct, page > 1 ? page : 1, tmp + 1);
		}

		return NEWDIRECT;
	}

error:
	clear_line(2);
	outs("不正确的讨论区.");
	pressreturn();
	return FULLUPDATE;
}

/* Add by betterman 07/06/07 */
int
do_select2(int ent, struct fileheader *fileinfo, char *direct, char *bname, int newent)
{
	struct boardheader *bp;
	int page, tmp, oldlevel;

	if (*bname == '\0')
		return FULLUPDATE;

	if ((bp = getbcache(bname)) == NULL)
		goto error;

	if (bp->flag & BRD_GROUP) {
		if ((oldlevel = setboardlevel(bname)) == -1)
			goto error;
#ifdef INBOARDCOUNT
		/* inboard user count  by freestyler */
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
#endif
		brdnum = -1;
		choose_board(YEA);	/* monster: bug here, we did not use original value of newflag here */
		boardlevel = oldlevel;
		brdnum = -1;		/* monster: refresh board list */
		return DOQUIT;
	} else {
		if (digestmode > 1) 
			digestmode = 0;

#ifdef INBOARDCOUNT
		int idx = getbnum(currboard);
		board_setcurrentuser(idx-1, -1);
#endif
		brc_initial(bname);

#ifdef INBOARDCOUNT
		idx = getbnum(currboard);
		board_setcurrentuser(idx-1, 1);
#endif

		setbdir(direct, currboard);
		current_bm = (HAS_PERM(PERM_BOARDS | PERM_PERSONAL) &&
			      check_bm(currentuser.userid, bp->BM)) ||
			      HAS_PERM(PERM_BLEVELS);

		if (DEFINE(DEF_FIRSTNEW)) {
			tmp = unread_position(direct, bp);
			page = tmp - t_lines / 2;
			getkeep(direct, page > 1 ? page : 1, tmp + 1);
		}
		
		sprintf(genbuf, "%d", newent);
		return NEWDIRECT2;
	}

error:
	return FULLUPDATE;
}


/*
int
read_letter(int ent, struct fileheader *fileinfo, char *direct)
{
	setmaildir(direct,currentuser.userid);
	return NEWDIRECT;
}
*/

int
do_acction(int type)
{
	clear_line(t_lines - 1);
	outs("\033[1;5m系统处理标题中, 请稍候...\033[m");
	refresh();
	
	switch (type) {
	case 2:
		return make_thread(currboard, NA);
	case 3:		/* marked */
	case 4:		/* 原作 */
	case 5:         /* 同作者 */
	case 6:         /* 同作者  精确 */
	case 7:         /* 标题关键字 */
		return marked_all(type - 3);
	}

	return 0;
}

int
acction_mode(int ent, struct fileheader *fileinfo, char *direct)
{
	int type;
	char ch[4] = { '\0' };

	if (digestmode != NA) {
		if (digestmode == 5 || digestmode == 6) {
			snprintf(genbuf, sizeof(genbuf), "boards/%s/SOMEONE.%s.DIR.%d", currboard, someoneID, digestmode - 5);
			unlink(genbuf);
		} else if (digestmode == 7) {
			snprintf(genbuf, sizeof(genbuf), "boards/%s/KEY.%s.DIR", currboard, currentuser.userid);
			unlink(genbuf);
		}
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		saveline(t_lines - 1, 0);
		clear_line(t_lines - 1);
		getdata(t_lines - 1, 0,
			"切换模式到: 1)文摘 2)同主题 3)被 m 文章 4)原作 5)同作者 6)标题关键字 [2]: ",
			ch, 3, DOECHO, YEA);
		if (ch[0] == '\0')
			ch[0] = '2';
		type = atoi(ch);

		if (type < 1 || type > 6) {
			saveline(t_lines - 1, 1);
			return PARTUPDATE;
		} else if (type == 6) {
			getdata(t_lines - 1, 0, "您想查找的文章标题关键字: ",
				someoneID, 30, DOECHO, YEA);
			if (someoneID[0] == '\0') {
				saveline(t_lines - 1, 1);
				return PARTUPDATE;
			}
			type = 7;
		} else if (type == 5) {
			strlcpy(someoneID, fileinfo->owner, sizeof(someoneID));
			getdata(t_lines - 1, 0, "您想查找哪位网友的文章? ",
				someoneID, 13, DOECHO, NA);
			if (someoneID[0] == '\0') {
				saveline(t_lines - 1, 1);
				return PARTUPDATE;
			}
			getdata(t_lines - 1, 37,
				"精确查找按 Y， 模糊查找请回车[Enter]", ch, 2,
				DOECHO, YEA);
			if (ch[0] == 'y' || ch[0] == 'Y')
				type = 6;
		}

		if (do_acction(type) != -1) {
			digestmode = type;
			setbdir(currdirect, currboard);
			if (!dashf(currdirect)) {
				digestmode = NA;
				setbdir(currdirect, currboard);
				return PARTUPDATE;
			}
		}
	}
	return NEWDIRECT;
}

int
pure_mode(void)
{
	if (do_acction(4) != -1) {
		digestmode = 4;
		setbdir(currdirect, currboard);
		do_acction(4);

		if (!dashf(currdirect)) {
			digestmode = NA;
			setbdir(currdirect, currboard);
			return PARTUPDATE;
		}
	}
	return NEWDIRECT;
}

int
deny_mode(void)
{
	if (!current_bm)
		return DONOTHING;

	if (digestmode) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		struct boardheader *bp;

		bp = getbcache(currboard);  //Added by cancel At 02.05.20
		digestmode = (bp->flag & ANONY_FLAG) ? 10 : 11;
		setbdir(currdirect, currboard);
		if (get_num_records(currdirect, sizeof(struct denyheader)) == 0) {
			denyuser();
			digestmode = NA;
			setbdir(currdirect, currboard);
		}
	}
	return NEWDIRECT;
}

int
digest_mode(void)
{                               /* 文摘模式 切换 */
	if (digestmode == YEA) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		digestmode = YEA;
		setbdir(currdirect, currboard);
		if (!dashf(currdirect)) { /* 无文摘列表文件 */
			digestmode = NA;
			setbdir(currdirect, currboard);
			return DONOTHING;
		}
	}
	return NEWDIRECT;
}

int
deleted_mode(void)
{
	if (!current_bm && !HAS_PERM(PERM_JUDGE))
		return DONOTHING;

	if (digestmode == 8) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		digestmode = 8;
		setbdir(currdirect, currboard);
		if (!dashf(currdirect)) {
			digestmode = NA;
			setbdir(currdirect, currboard);
			return DONOTHING;
		}
	}
	return NEWDIRECT;
}

int
junk_mode(void)
{

	if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_JUDGE))
		return DONOTHING;

	if (digestmode == 9) {
		digestmode = NA;
		setbdir(currdirect, currboard);
	} else {
		digestmode = 9;
		setbdir(currdirect, currboard);
		if (!dashf(currdirect)) {
			digestmode = NA;
			setbdir(currdirect, currboard);
			return DONOTHING;
		}
	}
	return NEWDIRECT;
}

int
dodigest(int ent, struct fileheader *fileinfo, char *direct, int delete, int update)
{
	char ndirect[PATH_MAX + 1]; // normal direct
	char ddirect[PATH_MAX + 1]; // digest direct
	char filename[PATH_MAX + 1], filename2[PATH_MAX + 1];
	char path[PATH_MAX + 1], *ptr;
	struct fileheader header;
	int pos;

	strlcpy(path, direct, sizeof(path));
	if ((ptr = strrchr(path, '/')) == NULL)
		return DONOTHING;
	*ptr = '\0';

	snprintf(ndirect, sizeof(ndirect), "%s/.DIR", path);
	snprintf(ddirect, sizeof(ddirect), "%s/.DIGEST", path);

	if (digestmode == 1) {
		if (!delete)
			return DONOTHING;

		snprintf(filename, sizeof(filename), "%s/%s", path, fileinfo->filename);
		if (delete_record(ddirect, sizeof(struct fileheader), ent) == -1)
			return PARTUPDATE;
		unlink(filename);

		snprintf(filename, sizeof(filename), "M%s", fileinfo->filename + 1);
		if ((pos = search_record(ndirect, &header, sizeof(struct fileheader), cmpfilename2, filename)) > 0) {
			header.flag &= ~FILE_DIGEST;
			safe_substitute_record(ndirect, &header, pos, NA);

		}

		return DIRCHANGED;
	}

	if (delete) {
		snprintf(filename, sizeof(filename), "G%s", fileinfo->filename + 1);
		delete_file(ddirect, 1, filename, YEA);
		fileinfo->flag &= ~FILE_DIGEST;
	} else {
		if (fileinfo->filename[0] != 'G') {
			snprintf(filename, sizeof(filename), "%s/%s", path, fileinfo->filename);
			snprintf(filename2, sizeof(filename2), "%s/G%s", path, fileinfo->filename + 1);
			if (link(filename, filename2) == -1) {
				if (errno != EEXIST) {
					return DONOTHING;
				} else {
					char gfilename[PATH_MAX + 1];

					// monster: 解决 g 的同步问题
					snprintf(gfilename, sizeof(gfilename), "G%s", fileinfo->filename + 1);
					if (search_record(ddirect, &header, sizeof(struct fileheader), 
					    cmpfilename2, gfilename) > 0) {
						fileinfo->flag |= FILE_DIGEST;
						return DONOTHING;
					}
				}
			}
		}

		memcpy(&header, fileinfo, sizeof(header));
		header.flag = fileinfo->flag & FILE_ATTACHED;
		snprintf(header.filename, sizeof(header.filename), "G%s", fileinfo->filename + 1);
		if (append_record(ddirect, &header, sizeof(struct fileheader)) == -1) {
			unlink(filename2);
			return DONOTHING;
		}

		fileinfo->flag |= FILE_DIGEST;
	}

	if (update) {
		safe_substitute_record(ndirect, fileinfo, ent, YEA /* digestmode == 0 */);
		if ( digestmode > 1 )  
			safe_substitute_record(direct, fileinfo, ent, (digestmode == 2) ? NA:YEA );

	}

	return DIRCHANGED;
}

int
digest_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode > 7 || !current_bm)
		return DONOTHING;

	dodigest(ent, fileinfo, direct, (fileinfo->flag & FILE_DIGEST), YEA);
	return PARTUPDATE;
}

#ifndef NOREPLY
int
do_reply(char *title, char *replyid, int id)
{
	strcpy(replytitle, title);
	post_article(currboard, replyid, id);
	replytitle[0] = '\0';
	return FULLUPDATE;
}
#endif

int
garbage_line(char *str)
{
	int qlevel = 0;

	while (*str == ':' || *str == '>') {
		str++;
		if (*str == ' ')
			str++;
		if (qlevel++ >= 1)
			return 1;
	}
	while (*str == ' ' || *str == '\t')
		str++;
	if (qlevel >= 1)
		if (strstr(str, "提到:\n") || strstr(str, ": 】\n") ||
		    strncmp(str, "==>", 3) == 0 || strstr(str, "的文章 □"))
			return 1;
	return (*str == '\n');
}

/* this is a 陷阱 for bad people to cover my program to his */
int
Origin2(char *text)
{
	char tmp[STRLEN];

	snprintf(tmp, sizeof(tmp), ":·%s %s·[FROM:", BoardName, BBSHOST);
	return (strstr(text, tmp) != NULL) ? YEA : NA;
}

void
do_quote(char *filepath, char quote_mode)
{
	FILE *inf, *outf;
	char *qfile, *quser;
	char buf[256], *ptr;
	char op;
	int bflag, i;

	qfile = quote_file;
	quser = quote_user;
	bflag = strncmp(qfile, "mail", 4);

	if ((outf = fopen(filepath, "w")) == NULL)
		return;

	if (quote_mode != '\0' && *qfile != '\0' && (inf = fopen(qfile, "r")) != NULL) {
		op = quote_mode;
		if (op != 'N' && fgets(buf, sizeof(buf), inf) != NULL) {
			if ((ptr = strrchr(buf, ')')) != NULL) {
				ptr[1] = '\0';
				if ((ptr = strchr(buf, ':')) != NULL) {
					quser = ptr + 1;
					while (*quser == ' ')
						quser++;
				}
			}

			if (bflag) {
				fprintf(outf, "\n【 在 %-.55s 的大作中提到: 】\n", quser);
			} else {
				fprintf(outf, "\n【 在 %-.55s 的来信中提到: 】\n", quser);
			}

			if (op == 'A') {
				while (fgets(buf, 256, inf) != NULL)
					fprintf(outf, ": %s", buf);
			} else if (op == 'R') {
				while (fgets(buf, 256, inf) != NULL)
					if (buf[0] == '\n')
						break;

				while (fgets(buf, 256, inf) != NULL) {
					if (Origin2(buf))
						continue;
					fputs(buf, outf);
				}
			} else {
				while (fgets(buf, 256, inf) != NULL)
					if (buf[0] == '\n')
						break;

				i = 0;
				while (fgets(buf, 256, inf) != NULL) {
					if (strcmp(buf, "--\n") == 0)
						break;
					if (buf[250] != '\0')
						strcpy(buf + 250, "\n");
					if (!garbage_line(buf)) {
						if (op == 'S' && i >= 10) {
							fprintf(outf, ": .................（以下省略）");
							break;
						}
						i++;
						fprintf(outf, ": %s", buf);
					}
				}
			}
		}
		fputc('\n', outf);
		fclose(inf);
	}
	*quote_file = '\0';
	*quote_user = '\0';
	if (currentuser.signature == 0 || header.chk_anony) {
		fputs("\n--", outf);
	} else {
		addsignature(outf, 1);
	}
	fclose(outf);
}

/* Add by SmallPig */
void
getcross(char *filepath, int mode)
{
	FILE *inf, *of;
	char buf[8192];
	char owner[248], *ptr;
	int count, owner_found = 0, inmail = INMAIL(uinfo.mode);

	modify_user_mode(POSTING);
	if ((inf = fopen(quote_file, "r")) == NULL) {
		report("getcross: cannot open %s for reading", quote_file);
		return;
	}

	if ((of = fopen(filepath, "w")) == NULL) {
		report("getcross: cannot open %s for writing", filepath);
		fclose(inf);
		return;
	}
	if (mode == 0) {
		write_header(of, 0 /* 写入 .post */, NA); /* freestyler: 转载写入 .post, 十大作者问题 */ 
		if (fgets(buf, 256, inf) != NULL) {
			if ((ptr = strstr(buf, "信人: ")) == NULL) {
				owner_found = 0;
				strcpy(owner, "Unkown User");
			} else {
				ptr += 6;
				for (count = 0; ptr[count] != 0 && ptr[count] != ' ' && ptr[count] != '\n'; count++);

				if (count <= 1) {
					owner_found = 0;
					strcpy(owner, "Unkown User");
				} else {
					if (buf[0] == 27) {
						strcpy(owner, "自动发信系统");
					} else {
						strlcpy(owner, ptr, count + 1);
					}
					owner_found = 1;
				}
			}
		}

		if (inmail) {
			fprintf(of, "\033[1;37m【 以下文字转载自 \033[32m%s \033[37m的信箱 】\n", currentuser.userid);
		} else {
			fprintf(of, "\033[1;37m【 以下文字转载自 \033[32m%s \033[37m讨论区 】\n", quote_board);
		}

		if (owner_found) {
			/* skip file header */
			while (fgets(buf, 256, inf) != NULL && buf[0] != '\n');

			fgets(buf, 256, inf);
			if ((strstr(buf, "【 以下文字转载自 ") && strstr(buf, "讨论区 】"))) {
				fgets(buf, 256, inf);
				if (strstr(buf, "【 原文由") && strstr(buf, "所发表 】")) {
					fputs(buf, of);
				} else {
					fprintf(of, "【 原文由\033[32m %s\033[37m 所发表 】\033[m\n\n", owner);
				}
			} else {
				fprintf(of, "【 原文由\033[32m %s\033[37m 所发表 】\033[m\n\n", owner);
				fputs(buf, of);
			}
		} else {
			fprintf(of, "\033[m");
			fseek(inf, 0, SEEK_SET);
		}
	} else if (mode == 1) {
		add_sysheader(of, quote_board, quote_title);
	} else if (mode == 2) {
		write_header(of, 0 /* 写入 .posts */, NA);
	} else if (mode == 3) {
		/* 精华区转载至版面 */
		write_header(of, 0 /* 写入 .posts */, NA);
		fprintf(of, "\033[1;37m【 以下文字转载自 \033[32m%s\033[37m 】\n", ainfo.title);
		if (quote_user[0] != '\0') {
			fprintf(of, "【 原文由\033[32m %s\033[37m 所发表 】\033[m\n\n", quote_user);
			quote_user[0] = '\0';
		}
	}

	while ((count = fread(buf, 1, sizeof(buf), inf)) > 0)
		fwrite(buf, 1, count, of);


	if (!mode) {			// 转载记录
		if(getattachinfo(quote_fh)) {	/* freestyler: 附件链接信息　*/
			fprintf(of, "\033[m\n%s 链接:\n", attach_info);
			fprintf(of, "\033[4m%s\033[m\n", attach_link);
		}
		fprintf(of, "--\n\033[m\033[1;%2dm※ 转载:.%s %s.[FROM: %s]\033[m\n",
			(currentuser.numlogins % 7) + 31, BoardName, BBSHOST, fromhost);
	}

	fclose(of);
	fclose(inf);
	quote_file[0] = '\0';
}

int
do_post(void)
{
	struct boardheader *bp;
	bp = getbcache(currboard);
	noreply = bp->flag & NOREPLY_FLAG;
	*quote_file = '\0';
	*quote_user = '\0';
	local_article = YEA;
	return post_article(currboard, NULL, 0);
}

int
post_reply(int ent, struct fileheader *fileinfo, char *direct)
{
	char uid[STRLEN];
	char title[TITLELEN], buf[STRLEN];
	char *t;
	FILE *fp;

	if (guestuser || !HAS_PERM(PERM_LOGINOK))
		return DONOTHING;

	clear();
	if (check_maxmail()) {
		pressreturn();
		return FULLUPDATE;
	}
	modify_user_mode(SMAIL);

	/* indicate the quote file/user */
	setboardfile(quote_file, currboard, fileinfo->filename);
	strlcpy(quote_user, fileinfo->owner, sizeof(quote_user));
	
	/* find the author */
	if (getuser(quote_user, NULL) == 0) {
		genbuf[0] = '\0';
		if ((fp = fopen(quote_file, "r")) != NULL) {
			fgets(genbuf, 255, fp);
			fclose(fp);
		}
		t = strtok(genbuf, ":");
		if (strncmp(t, "发信人", 6) == 0 ||
		    strncmp(t, "寄信人", 6) == 0 ||
		    strncmp(t, "Posted By", 9) == 0 ||
		    strncmp(t, "作  者", 6) == 0) {
			while (t != NULL) {
				t = (char *) strtok(NULL, " \r\t\n<>");
				if (t == NULL)
					break;
				if (!invalidaddr(t))
					break;
			}
			if (t != NULL) {
				strlcpy(uid, t, sizeof(uid));
			}
		}
		
		if (t == NULL || (strchr(t, '@') == NULL && getuser(t, NULL) == 0)) {
			prints("对不起，该帐号已经不存在。\n");
			pressreturn();
			return FULLUPDATE;
		}
	} else {
		strlcpy(uid, quote_user, sizeof(uid));
	}

	/* make the title */
	if ((fileinfo->title[0] != 'R' && fileinfo->title[0] != 'r') || fileinfo->title[1] != 'e' || fileinfo->title[2] != ':') {
		snprintf(title, sizeof(title), "Re: %s", fileinfo->title);
	} else {
		strlcpy(title, fileinfo->title, sizeof(title));
	}

	/* edit, then send the mail */
	snprintf(buf, sizeof(buf), "信件已成功地寄给原作者 %s\n", uid);
	m_feedback(do_send(uid, title, YEA, 0), uid, buf);
	return FULLUPDATE;
}

/* Add by SmallPig */
int
post_cross(char islocal, int mode)
{
	struct fileheader postfile;
	struct boardheader *bp;
	char bdir[STRLEN], filepath[STRLEN];
	char buf[256], whopost[IDLEN + 2];

	if (!haspostperm(currboard) && !mode) {
		prints("\n\n您尚无权限在 %s 版发表文章，取消转载\n", currboard);
		return -1;
	}

	memset(&postfile, 0, sizeof (postfile));

	if (!mode && strncmp(quote_title, "[转载]", 6)) {
		snprintf(save_title, sizeof(save_title), "[转载] %s", quote_title);
	} else {
		strlcpy(save_title, quote_title, sizeof(save_title));
	}

	snprintf(bdir, sizeof(bdir), "boards/%s", currboard);
	if ((getfilename(bdir, filepath, GFN_FILE | GFN_UPDATEID, &postfile.id)) == -1)
		return -1;
	strcpy(postfile.filename, strrchr(filepath, '/') + 1);

	if (mode == 1) {
		strcpy(whopost, BBSID);
		postfile.flag |= FILE_MARKED;
	} else {
		strcpy(whopost, currentuser.userid);
	}

	strlcpy(postfile.owner, whopost, sizeof(postfile.owner));
	strlcpy(postfile.title, save_title, sizeof(postfile.title));
//      setboardfile(filepath, currboard, postfile.filename);

	bp = getbcache(currboard);
	if ((islocal == 'S' || islocal == 's') && (bp->flag & OUT_FLAG))
		local_article = NA;
	else
		local_article = YEA;

	posts_article_id = postfile.id;
	getcross(filepath, mode);

	if (local_article == YEA || !(bp->flag & OUT_FLAG)) {
		postfile.flag &= ~FILE_OUTPOST;
	} else {
		postfile.flag |= FILE_OUTPOST;
		outgo_post(&postfile, currboard);
	}

	setbdir(buf, currboard);
	postfile.filetime = time(NULL);
	if (append_record(buf, &postfile, sizeof(postfile)) == -1) {
		unlink(postfile.filename);      // monster: remove file on failure
		if (!mode) {
			report("cross_posting '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		} else {
			report("Posting '%s' on '%s': append_record failed!",
				postfile.title, quote_board);
		}
		pressreturn();
		clear();
		return -1;
	}

	if (!mode) {
		report("cross_posted '%s' on '%s'", postfile.title,
			currboard);
	}

    BBS_SINGAL("/post/cross",
               "f", postfile.filename,
               "b", currboard,
               "h", fromhost,
               NULL);

	update_lastpost(currboard);
	update_total_today(currboard);
	return 1;
}

int
show_board_notes(char *bname, int promptend)
{
	char buf[PATH_MAX + 1];

	snprintf(buf, sizeof(buf), "vote/%s/notes", bname);
	if (dashf(buf)) {
		ansimore2(buf, promptend, 0, (promptend == YEA) ? 0 : t_lines - 5);
		return 1;
	} else if (dashf("vote/notes")) {
		ansimore2("vote/notes", promptend, 0, (promptend == YEA) ? 0 : t_lines - 5);
		return 1;
	}
	return -1;
}

int
show_user_notes(void)
{
	char buf[256];

	setuserfile(buf, "notes");
	if (dashf(buf)) {
		ansimore(buf, YEA);
		return FULLUPDATE;
	}
	clear();
	move(10, 15);
	prints("您尚未在 InfoEdit->WriteFile 编辑个人备忘录。\n");
	pressanykey();
	return FULLUPDATE;
}

int
outgo_post(struct fileheader *fh, char *board)
{
	char buf[PATH_MAX + 1];

	snprintf(buf, sizeof(buf), "%s\t%s\t%s\t%s\t%s\n", board,
		fh->filename, header.chk_anony ? board : currentuser.userid,
		header.chk_anony ? "我是匿名天使" : currentuser.username,
		save_title);

	return file_append("innd/out.bntp", buf);
}

void
punish(void)
{
	char keybuf[3];
	static int count;

	if (count == 3) {
		clear();
		prints("真拿你没办法，还是踢你下去好了。。。");
		refresh();
		sleep(3);
		abort_bbs();
	}

	++count;
	clear();
	outc('\n');
	prints("\033[1;37;40m \033[0;37;40m▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄ \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                     \033[0;37;40m  \033[1m                   \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[0;37;47m  \033[40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[0;37;40m \033[1m\n");
	prints("\033[1;37;40m \033[0;33;47m  \033[1;37;40m                                                          \033[0;33;47m  \033[1;37;40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[47m  \033[40m                                                          \033[47m  \033[40m\n");
	prints("\033[1;37;40m \033[0;33;47m  \033[1;37;40m                                        \033[0;1;37;40m                  \033[0;33;47m  \033[1;37;40m\n");
	prints("\033[1;37;40m \033[0;33;47m  \033[1;37;40m                                  \033[0;37;40m  \033[1m                      \033[0;33;47m  \033[37;40m \033[1m\n");
	prints("\033[1;37;40m \033[0;33;47m                              \033[1;37m    \033[0;33;47m                            \033[1;37;40m\n");
	prints("\033[1;37;40m   \033[34m█████████████████████████████\033[37m\n");
	prints("\033[1;37;40m                                                                 \033[0;1m\n");
	prints("\033[m\n");
	refresh();
	sleep(1);
	move(4, 6);
	prints("\033[1;37m系统怀疑你用灌水机。。。\033[m");
	refresh();
	sleep(1);
	move(6, 6);
	prints("\033[1;37m快说，你是不是用了灌水机？\033[m");
	sleep(1);
	refresh();
	move(8, 6);
	prints("\033[1;37m还不认，怎么可能在 xx 秒内连发 yy 篇文章？\033[m");
	sleep(1);
	refresh();
	move(10, 6);
	prints("\033[1;37m什么？手动的？你骗谁，哪有这么快？\033[m");
	sleep(3);
	refresh();
	move(12, 6);
	sleep(4);
	refresh();
	prints("\033[1;37m哦？真的没用灌水机？那可能是系统搞错了，不阻你了^_^\033[m");
	refresh();
	sleep(15);
	move(19, 6);
	prints("\033[1;37;5m                (press return)\033[m");
	move(t_lines - 1, 0);
	getdata(t_lines - 1, 0, "", keybuf, 2, NOECHO, YEA);
	clear();
}

static unsigned  int 
ip_str2int(const char *ip)
{
	/* Assume ip is ipv4 : a.b.c.d */
	int dotcnt = 0, i;
	for (i = 0; ip[i]; i++) {
		if ( ip[i] == '.' ) dotcnt++;
	}
	if ( dotcnt != 3 ) return 0;
	unsigned int ret = 0;
	char buf[256];
	strlcpy(buf, ip, sizeof( buf ));
	char *tk = strtok(buf, ".");
	while ( tk != NULL ) {
		ret = ret * 256 + atoi(tk);
		tk = strtok(NULL, ".");
	}
	return ret;
}

int 
check_outcampus_ip()
{
	if (access(SYSU_IP_LIST, F_OK) == -1) {
		return NA;
	}
	
	FILE *fp = fopen(SYSU_IP_LIST, "r");
	char ip_start[64], ip_end[64];
	unsigned int ip_s, ip_e; 
	unsigned int ip_now = ip_str2int(raw_fromhost);

	if ( fp == NULL ) return NA;
	while ( fscanf( fp, "%s%s", ip_start, ip_end ) != EOF ) {
		ip_s = ip_str2int(ip_start);
		ip_e = ip_str2int(ip_end);
		if ( ip_now >= ip_s && ip_now <= ip_e ) {
			fclose(fp);
			return NA;
		}
	}
	fclose(fp);
	return YEA;
}
int
post_article(char *postboard, char *mailid, unsigned int id)
{
	struct fileheader postfile;
	struct boardheader *bp;
	char bdir[STRLEN], filepath[STRLEN], buf[STRLEN], replyfilename[STRLEN];
	int interval;
	static time_t lastposttime = 0;
	static int postcount = 0;
	
	int bm = (current_bm || HAS_PERM(PERM_SYSOP));

    /* LTaoist : backup the quote_file name for siganl the replyfilename */
    if(*quote_file)
    {
        strcpy(replyfilename, basename(quote_file));
    }
	
	if ( check_board_attr(currboard, BRD_RESTRICT) == NA 
			//&& !bm
			&& YEA == check_outcampus_ip() ) { 
		/* 
		 * Cypress 2012.10.17 学校通知：十八大期间校外ip暂时取消发贴功能 
		 * 编辑 SYSU_IP_LIST 文件(consts.h)，记录校内ip段
		 * 恢复发贴时只需将该文件改名或者删除即可。
		 */
		clear();
		move(8, 1);
		prints("   对不起, 您不能在该讨论区上发表文章, 可能的原因如下:\n\n");
		prints("       1. 逸仙时空将于2012年10月25日至11月30日之间进行维护，期间将暂停校外IP发贴功能.\n");
		prints("       2. 在校学生若在校外可以通过学校VPN通道连接发贴，具体参见：\n");
	    prints("            a. 信息技术服务帮助台 http://helpdesk.sysu.edu.cn \n");
	    prints("            b. 点击 IT服务FAQ\n");
	    prints("            c. 点击 VPN隧道 查看相关说明\n");
		prints("       3. 带来的不便请站友们见谅!\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	if (YEA == check_readonly(currboard)) { /* Leeward 98.03.28 */
		clear();
		move(8, 1);
		prints("   对不起, 您不能在该讨论区上发表文章, 可能的原因如下:\n\n");
		prints("       1. 您尚未获得发表文章的权限;\n");
		prints("       2. 您被管理人员取消了该讨论区的发文权限;\n");
		prints("       3. 该讨论区只读.\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	if (!strcmp(currboard, DEFAULTRECOMMENDBOARD) && !current_bm) {
		clear();
		move(8, 1);
		prints("                对不起, 您不能在推荐版面上发表文章。\n\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	if (!check_max_post(currboard))
		return FULLUPDATE;
	#ifdef FILTER
	if (!has_filter_inited()) {
		init_filter();
	}
	if (!regex_strstr(currentuser.username))
	{
		clear();
		move(8,1);
		prints("       对不起，你的昵称中含有不合适的内容，不能发文，请先进行修改\n");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	#endif
	if ( digestmode > 7 ) {
		clear();
		move(5, 10);
		prints("目前是回收站或封禁列表, 不能发表文章\n          (按 ENTER 后再按 LEFT 键可返回一般模式)。");
		pressreturn();
		clear();
		return FULLUPDATE;
	} 

	if (!haspostperm(postboard)) {
		clear();
		move(5, 10);
		prints("此讨论区是唯读的, 或是您尚无权限在此发表文章。");
		pressreturn();
		clear();
		return FULLUPDATE;
	}
	
	modify_user_mode(POSTING);
	interval = abs(time(NULL) - lastposttime);
	if (interval < 80) {
		if (postcount >= 20) {
			report("%s 在 %s 灌水，已被系统阻止", currentuser.userid, currboard);
			punish();
			postcount = 8;
			return FULLUPDATE;
		} else {
			postcount++;
		}
	} else {
		lastposttime = time(NULL);
		postcount = 0;
	}

	memset(&postfile, 0, sizeof (postfile));
	clear();
	show_board_notes(postboard, NA);
	bp = getbcache(postboard);
	if (bp->flag & OUT_FLAG && replytitle[0] == '\0')
		local_article = NA;
#ifndef NOREPLY
	if (replytitle[0] != '\0') {
		if (strncmp(replytitle, "Re: ", 4) == 0) {
			strlcpy(header.title, replytitle, TITLELEN);
		} else {
			snprintf(header.title, sizeof(header.title), "Re: %s", replytitle);
			header.title[TITLELEN - 1] = '\0';
		}
		header.reply_mode = 1;
	} else
#endif
	{
		header.title[0] = '\0';
		header.reply_mode = 0;

	}
	strcpy(header.ds, postboard);
	header.postboard = YEA;
	if (post_header(&header) == YEA) {
		strlcpy(postfile.title, header.title, sizeof(postfile.title));
		strlcpy(save_title, postfile.title, sizeof(save_title));
	} else {
		return FULLUPDATE;
	}

	snprintf(bdir, sizeof(bdir), "boards/%s", postboard);
	if ((getfilename(bdir, filepath, GFN_FILE | ((mailid == NULL) ? GFN_UPDATEID : 0), &id)) == -1)
		return DONOTHING;
	postfile.id = id;
	strcpy(postfile.filename, strrchr(filepath, '/') + 1);

	strlcpy(postfile.owner, (header.chk_anony) ? postboard : currentuser.userid, sizeof(postfile.owner));
	setboardfile(filepath, postboard, postfile.filename);
	modify_user_mode(POSTING);
	do_quote(filepath, header.include_mode);

	posts_article_id = id;
	if (vedit(filepath, EDIT_SAVEHEADER | EDIT_MODIFYHEADER | EDIT_ADDLOGINFO) == -1) {
		unlink(filepath);
		clear();
		return FULLUPDATE;
	}

	strlcpy(postfile.title, save_title, sizeof(postfile.title));

	/* monster: 重新指定文件名 */
	#ifdef RENAME_AFTERPOST
	if ((getfilename(bdir, filepath, GFN_LINK | ((mailid == NULL) ? GFN_UPDATEID : 0), &postfile.id)) == 0)
		strcpy(postfile.filename, strrchr(filepath, '/') + 1);
	#endif

	if ((local_article == YEA) || !(bp->flag & OUT_FLAG)) {
		postfile.flag &= ~FILE_OUTPOST;
	} else {
		postfile.flag |= FILE_OUTPOST;
		outgo_post(&postfile, postboard);
	}

	sprintf(buf, "boards/%s/%s", postboard, DOT_DIR);

	if (noreply) {
		postfile.flag |= FILE_NOREPLY;
		noreply = NA;
	}
#ifdef MARK_X_FLAG
	if (markXflag) {
		postfile.flag |= FILE_DELETED;
		markXflag = 0;
	} else {
		postfile.flag &= ~FILE_DELETED;
	}
#endif
	if (mailtoauthor) {
		//Added by cancel At 02.05.22: 修正重定义文件名引起的bug
		setboardfile(filepath, postboard, postfile.filename);
		if (header.chk_anony)
			prints("对不起，您不能在匿名版使用寄信给原作者功能。");
		else if (!mail_file(filepath, mailid, postfile.title))
			prints("信件已成功地寄给原作者 %s", mailid);
		else
			prints("信件邮寄失败，%s 无法收信。", mailid);
		pressanykey();
	}
	else if (postandmail) {
		if (header.chk_anony) anonymousmail = YEA;
		mail_file(filepath, mailid, postfile.title);
		anonymousmail = NA;
	}
	mailtoauthor = 0;
	postandmail = 0;

	/* monster: 记录匿名文章的真实ID */
	if (bp->flag & ANONY_FLAG) {
		strlcpy(postfile.realowner, currentuser.userid, sizeof(postfile.realowner));
	}

	postfile.filetime = time(NULL);
	if (append_record(buf, &postfile, sizeof(postfile)) == -1) {
		unlink(postfile.filename);
		report("posting '%s' on '%s': append_record failed!",
			postfile.title, currboard);
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	update_lastpost(currboard);
	update_total_today(currboard);
	brc_addlist(postfile.filetime);

    if(replytitle[0] == '\0')
    {
        BBS_SINGAL("/post/newtopic",
                   "b", postboard,
                   "f", postfile.filename,
                   "h", fromhost,
                   NULL);
    }
    else
    {
        BBS_SINGAL("/post/reply",
                   "b", postboard,
                   "f", postfile.filename,
                   "f0", replyfilename,
                   "h", fromhost,
                   NULL);
    }

	report("posted '%s' on '%s'", postfile.title, currboard);

	if (!junkboard()) {
		set_safe_record();
		currentuser.numposts++;
		substitute_record(PASSFILE, &currentuser, sizeof (currentuser),
				  usernum);
	}

	if( digestmode == 2 ||  /* 同主题 */
	    digestmode == 4 ||  /* 原作 */
	    digestmode == 5 ||  /* 同作者*/
	    digestmode == 6 ||  /* 同作者 精确 */
	    digestmode == 7 )  	/* 标题关键字 */
		do_acction(digestmode);

	return FULLUPDATE;
}

int
change_title(char *fname, char *title)
{
	FILE *fp, *out;
	char buf[256], outname[PATH_MAX + 1];
	int newtitle = 0;

	if ((fp = fopen(fname, "r")) == NULL)
		return -1;
	snprintf(outname, sizeof(outname), "%s.%s.%05d", fname, currentuser.userid, uinfo.pid);
	if ((out = fopen(outname, "w")) == NULL) {
		fclose(fp);     /* add by quickmouse 01/03/09 */
		return -1;
	}
	while ((fgets(buf, sizeof(buf), fp)) != NULL) {
		if (!strncmp(buf, "标  题: ", 8) && newtitle == 0) {
			fprintf(out, "标  题: %s\033[m\n", title);
			newtitle = 1;
			continue;
		}
		fputs(buf, out);
	}
	fclose(fp);
	fclose(out);
	f_mv(outname, fname);
	return 0;
}

int
edit_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[PATH_MAX + 1];
	struct boardheader *bp;

	if( !INMAIL(uinfo.mode) && YEA == check_readonly(currboard)) { /* Leeward 98.03.28 */
		clear();
		move(8, 8);
		prints("对不起，您不能在只读版面上编辑文章。");
		pressreturn();
		clear();
		return FULLUPDATE;
	}

	/* Pudding: 没发文权也不能修改文章 */
	if (!INMAIL(uinfo.mode) && !haspostperm(currboard))
		return DONOTHING;

	if (!INMAIL(uinfo.mode) && !current_bm && !isowner(&currentuser, fileinfo))
		return DONOTHING;


	if (INMAIL(uinfo.mode)) {
		setmailfile(buf, fileinfo->filename);
	} else {
		setboardfile(buf, currboard, fileinfo->filename);
	}

	modify_user_mode(EDIT);
	posts_article_id = fileinfo->id;
	if (vedit(buf, EDIT_NONE) == -1) {
		return FULLUPDATE;
	} else {
		bp = getbcache(currboard);
		if ((bp->flag & ANONY_FLAG) && !(strcmp(fileinfo->owner, currboard))) {
			add_edit_mark(buf, 4, NULL);
		} else {
			add_edit_mark(buf, 1, NULL);
		}
	}

#ifdef MARK_X_FLAG
	if (markXflag) {
		fileinfo->flag |= FILE_DELETED;
		markXflag = 0;
	} else {
		fileinfo->flag &= ~FILE_DELETED;
	}

	safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
#endif

	if (!INMAIL(uinfo.mode))
    {
		report("edited post '%s' on %s", fileinfo->title, currboard);
        BBS_SINGAL("/post/updatepost",
                   "f", fileinfo->filename,
                   "b", currboard,
                   "h", fromhost,
                   NULL);
    }
        
	return FULLUPDATE;
}

int
edit_title(int ent, struct fileheader *fileinfo, char *direct)
{
	struct boardheader *bp;
	char buf[TITLELEN], tmp[PATH_MAX + 1];
	char *ptr;

	if (!INMAIL(uinfo.mode) && YEA == check_readonly(currboard))   /* Leeward 98.03.28 */
		return DONOTHING;

	if (!INMAIL(uinfo.mode) && digestmode > 7 )
		return DONOTHING;
	/* Pudding: 没发文权也不能修改文章 */
	if (!INMAIL(uinfo.mode) && !haspostperm(currboard))
		return DONOTHING;
	
	if (!INMAIL(uinfo.mode) && !current_bm && !isowner(&currentuser, fileinfo))
		return DONOTHING;

	strlcpy(buf, fileinfo->title, sizeof(buf));
	my_ansi_filter(buf);
	getdata(t_lines - 1, 0, "新文章标题: ", buf, 55, DOECHO, NA);

	strlcpy(save_title, buf, sizeof(save_title));
	if (check_text() == 0) {
		return PARTUPDATE;
	}
	
	if (!strcmp(buf, fileinfo->title))
		return PARTUPDATE;
	if (!INMAIL(uinfo.mode))
		check_title(buf);

	int savedigestmode = digestmode;
	if (!INMAIL(uinfo.mode)) {
		if ( digestmode != 1) {
			digestmode = 0;
			setbdir(direct, currboard);
		}
	}
	if (buf[0] != '\0') {
		strlcpy(fileinfo->title, buf, sizeof(fileinfo->title));
		strlcpy(tmp, direct, sizeof(tmp));
		if ((ptr = strrchr(tmp, '/')) != NULL)
			*ptr = '\0';
		snprintf(genbuf, sizeof(genbuf), "%s/%s", tmp, fileinfo->filename);

		bp = getbcache(currboard);
		if ((bp->flag & ANONY_FLAG) && !(strcmp(fileinfo->owner, currboard))) {
			add_edit_mark(genbuf, 5, buf);
		} else {
			add_edit_mark(genbuf, 2, buf);
		}
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
        if(!INMAIL(uinfo.mode))
        {
            BBS_SINGAL("/post/changetitle",
                       "f", fileinfo->filename,
                       "b", currboard,
                       "h", fromhost,
                       NULL);
        }
	}
	if ( INMAIL(uinfo.mode) || savedigestmode == 0 || digestmode == 1 )
		return PARTUPDATE;
	else {
		digestmode = savedigestmode;
		setbdir(direct, currboard);
		do_acction(digestmode);
		return FULLUPDATE;
	}
}

int
underline_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode == 1 || digestmode > 7)
		return DONOTHING;

	if (!current_bm && !isowner(&currentuser, fileinfo))
		return DONOTHING;

	if (fileinfo->flag & FILE_NOREPLY) {
		fileinfo->flag &= ~FILE_NOREPLY;
	} else {
		fileinfo->flag |= FILE_NOREPLY;
	}
	/* freestyler:非标准阅读模式置不可回复标记 */
	int savedigestmode = digestmode;
	digestmode = 0;
	setbdir(direct, currboard);

	safe_substitute_record(direct, fileinfo, ent, YEA);

	if( savedigestmode != 0 ) {
		digestmode = savedigestmode;
		setbdir(direct, currboard);
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 2) ? NA: YEA);
	}
	return PARTUPDATE;
}

int
mark_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if (!current_bm || digestmode==1 || digestmode > 7)
		return DONOTHING;

	int flag;
	if (fileinfo->flag & FILE_MARKED) {
		fileinfo->flag &= ~FILE_MARKED;
		flag = 0;
	} else {
		fileinfo->flag |= FILE_MARKED;
		flag = 1;
	/*	fileinfo->flag &= ~FILE_DELETED; */
	}
	/* freestyler: 非标准阅读模式 mark post */
	int savedigestmode = digestmode;
	digestmode = 0;
	setbdir(direct, currboard);
	safe_substitute_record(direct, fileinfo, ent, YEA );
	if ( savedigestmode != 0 ) { 
		digestmode = savedigestmode;
		setbdir(direct, currboard); 
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 2 ) ? NA : YEA );
	}
    
	return PARTUPDATE;
}

static int
del_rangecheck(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;

	/* 保留m和g标记文章 */
	if (fileinfo->flag & (FILE_MARKED | FILE_DIGEST))
		return KEEPRECORD;

	cancelpost(currboard, currentuser.userid, fileinfo, 0, YEA);
	return REMOVERECORD;
}

int
del_range(int ent, struct fileheader *fileinfo, char *direct)
{
	char num[8];
	int inum1, inum2, result;

	if (!INMAIL(uinfo.mode) && !current_bm)
		return DONOTHING;

	if (digestmode || !strcmp(currboard, "syssecurity"))
		return DONOTHING;

	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "首篇文章编号: ", num, 7, DOECHO, YEA);
	if ((inum1 = atoi(num)) <= 0) {
		move(t_lines - 1, 50);
		prints("错误编号...");
		egetch();
		return PARTUPDATE;
	}
	getdata(t_lines - 1, 25, "末篇文章编号: ", num, 7, DOECHO, YEA);
	if ((inum2 = atoi(num)) <= inum1) {
		move(t_lines - 1, 50);
		prints("错误区间...");
		egetch();
		return PARTUPDATE;
	}
	move(t_lines - 1, 50);
	if (askyn("确定删除", NA, NA) == YEA) {
		result = process_records(direct, sizeof(struct fileheader), inum1, inum2, del_rangecheck, NULL);
		fixkeep(direct, inum1, inum2);

		if (INMAIL(uinfo.mode)) {
			snprintf(genbuf, sizeof(genbuf), "Range delete %d-%d in mailbox", inum1, inum2);
		} else {
			update_lastpost(currboard);
			snprintf(genbuf, sizeof(genbuf), "Range delete %d-%d on %s", inum1, inum2, currboard);
			securityreport(genbuf);
		}
		report("%s", genbuf);
		return DIRCHANGED;
	}
	move(t_lines - 1, 50);
	clrtoeol();
	prints("放弃删除...");
	egetch();
	return PARTUPDATE;
}

/*
 *   undelete 一篇文章 Leeward 98.05.18
 *   modified by ylsdd, monster
 */

static int
cmp_fname(const void *hdr1_ptr, const void *hdr2_ptr)
{
	const struct fileheader *hdr1 = (const struct fileheader *)hdr1_ptr;
	const struct fileheader *hdr2 = (const struct fileheader *)hdr2_ptr;

	return (hdr1->filetime - hdr2->filetime);
}

int
undelete_article(int ent, struct fileheader *fileinfo, char *direct, int update)
{
	char *ptr, *ptr2, buf[LINELEN];
	struct fileheader header;
	FILE *fp;
	int valid = NA;

	setboardfile(buf, currboard, fileinfo->filename);
	if ((fp = fopen(buf, "r")) == NULL)
		return -1;

	while (fgets(buf, sizeof(buf), fp)) {
		if (!strncmp(buf, "标  题: ", 8)) {
			valid = YEA;
			break;
		}
	}
	fclose(fp);

	memcpy(&header, fileinfo, sizeof(header));
	header.flag = 0;
	if (valid == YEA) {
		strlcpy(header.title, buf + 8, sizeof(header.title));
		if ((ptr = strrchr(header.title, '\n')) != NULL)
			*ptr = '\0';
	} else {
		/* article header is not valid, use default title */
		strlcpy(header.title, fileinfo->title, sizeof(header.title));
		if ((ptr = strrchr(header.title, '-')) != NULL) {
			*ptr = '\0';
			for (ptr2 = ptr - 1; ptr >= header.title; ptr2--) {
				if (*ptr2 != ' ') {
					*(ptr2 + 1) = '\0';
					break;
				}
			}
		}
	}

	snprintf(buf, sizeof(buf), "boards/%s/.DIR", currboard);
	if (append_record(buf, &header, sizeof(struct fileheader)) == -1)
		return -1;

	fileinfo->flag |= FILE_FORWARDED;
	snprintf(fileinfo->title, sizeof(fileinfo->title), "<< 本文已被 %s 恢复至版面 >>", currentuser.userid);

	if (update == YEA) {
		if (safe_substitute_record(direct, fileinfo, ent, NA) == -1)
			return -1;
		report("undeleted %s's '%s' on %s", header.owner, header.title, currboard);
	}
	return 0;
}

int
undel_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[PATH_MAX + 1];

	if ((digestmode != 8 && digestmode != 9) || !current_bm)
		return DONOTHING;

	if (fileinfo->flag & FILE_FORWARDED) {
		presskeyfor("本文已恢复过了");
	} else if (undelete_article(ent, fileinfo, direct, YEA) == -1) {
		presskeyfor("该文章不存在，已被恢复, 删除或列表出错");
	} else {
		snprintf(buf, sizeof(buf), "boards/%s/.DIR", currboard);
		sort_records(buf, sizeof(struct fileheader), cmp_fname);
		update_lastpost(currboard);
	}

	return PARTUPDATE;
}

static int
undel_rangecheck(void *rptr, void *extrarg)
{
	struct fileheader *fileinfo = (struct fileheader *)rptr;

	if (!(fileinfo->flag & FILE_FORWARDED))
		undelete_article(0, fileinfo, currdirect, NA);
	return KEEPRECORD;
}

int
undel_range(int ent, struct fileheader *fileinfo, char *direct)
{
	int inum1, inum2, old_digestmode;
	char num[8], buf[PATH_MAX + 1];

	if ((digestmode != 8 && digestmode != 9) || !current_bm)
		return DONOTHING;

	saveline(t_lines - 1, 0);
	clear_line(t_lines - 1);
	getdata(t_lines - 1, 0, "首篇文章编号: ", num, 7, DOECHO, YEA);
	if ((inum1 = atoi(num)) <= 0) {
		move(t_lines - 1, 50);
		prints("错误编号...");
		egetch();
		saveline(t_lines - 1, 1);
		return DONOTHING;
	}

	getdata(t_lines - 1, 25, "末篇文章编号: ", num, 7, DOECHO, YEA);
	if ((inum2 = atoi(num)) <= inum1) {
		move(t_lines - 1, 50);
		prints("错误区间...");
		egetch();
		saveline(t_lines - 1, 1);
		return DONOTHING;
	}

	move(t_lines - 1, 50);
	if (askyn("恢复文章", NA, NA) == YEA) {
		process_records(direct, sizeof(struct fileheader), inum1, inum2, undel_rangecheck, NULL);

		snprintf(buf, sizeof(buf), "boards/%s/.DIR", currboard);
		sort_records(buf, sizeof(struct fileheader), cmp_fname);
		update_lastpost(currboard);

		old_digestmode = digestmode;
		digestmode = NA;
		snprintf(genbuf, sizeof(genbuf), "Range undelete %d-%d on %s", inum1, inum2, currboard);
		securityreport(genbuf);
		digestmode = old_digestmode;

		return DIRCHANGED;
	}

	move(t_lines - 1, 50);
	clrtoeol();
	prints("放弃恢复...");
	egetch();
	return PARTUPDATE;
}

int
empty_recyclebin(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[STRLEN];

	if (!HAS_PERM(PERM_SYSOP))
		return DONOTHING;
//      if (digestmode != 8 && digestmode != 9)
//              return DONOTHING;

	snprintf(buf, sizeof(buf), "确定要清空%s吗", (digestmode == 8) ? "回收站" : "废纸篓");
	if (askyn(buf, NA, YEA) == NA)
		return PARTUPDATE;

	snprintf(buf, sizeof(buf), "boards/%s/.removing.%d", currboard, getpid());
	f_mv(direct, buf);

	snprintf(buf, sizeof(buf), "%s版%s被清空", currboard, (digestmode == 8) ? "回收站" : "废纸篓");
	digestmode = NA;
	securityreport(buf);
	setbdir(currdirect, currboard);
	return NEWDIRECT;
}

int
del_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char buf[PATH_MAX + 1], *ptr;
	int owned, IScombine;
	struct userec lookupuser;

	if (digestmode > 7 || !strcmp(currboard, "syssecurity"))
		return DONOTHING;

	/* freestyler: 非标准模式删文 */
	if (digestmode >= 2 ) {
		int savedigestmode = digestmode ;
		digestmode = NA;
		setbdir(direct, currboard);
		ent = get_dir_index(direct, fileinfo) + 1;
		if (del_post(ent, fileinfo, direct) == DIRCHANGED) { /* 递归,删除成功 */
			digestmode = savedigestmode;
			setbdir(direct, currboard);
			do_acction(digestmode);
			return DIRCHANGED;
		} else {
			digestmode = savedigestmode;
			setbdir(direct, currboard);
			return PARTUPDATE;
		}
	}

	if (guestuser)
		return DONOTHING;       /* monster: guest 不能砍文章, 就算是自己发的 ... */

	owned = isowner(&currentuser, fileinfo);
	if (!current_bm && !owned)
		return DONOTHING;

	snprintf(genbuf, sizeof(genbuf), "删除文章 [%-.55s]", fileinfo->title);
	if (askyn(genbuf, NA, YEA) == NA) {
		presskeyfor("放弃删除文章...");
		return PARTUPDATE;
	}

	if (digestmode == YEA) {
		dodigest(ent, fileinfo, direct, YEA, YEA);
		return DIRCHANGED;
	}

	if (delete_file(direct, ent, fileinfo->filename, NA) == 0) {
		struct boardheader *bp;

		bp = getbcache(currboard);

		strlcpy(buf, direct, sizeof(buf));
		if ((ptr = strrchr(buf, '/')) != NULL)
			*ptr = '\0';

		report("Del '%s' on '%s'", fileinfo->title, currboard);

        BBS_SINGAL("/post/del",
                   "b", currboard,
                   "f", fileinfo->filename,
                   NULL);
        
		IScombine = (!strncmp(fileinfo->title, "【合集】", 8) || 
			     !strncmp(fileinfo->title, "[合集]", 6) ) ;
		cancelpost(currboard, currentuser.userid, fileinfo, owned && (!IScombine), ((bp->flag & ANONY_FLAG) && owned) ? NA : YEA);

		if (!junkboard() && !digestmode && !IScombine) {
			if (owned) {
				set_safe_record();
				if (currentuser.numposts > 0) {
					--currentuser.numposts;
					substitute_record(PASSFILE, &currentuser, sizeof (currentuser), usernum);
				}
			} else {
				if ((owned = getuser(fileinfo->owner, &lookupuser))) {
					if (lookupuser.numposts > 0) {
						--lookupuser.numposts;
						substitute_record(PASSFILE, &lookupuser, sizeof (struct userec), owned);
					}
				}
			}
		}

		update_lastpost(currboard);
		return DIRCHANGED;
	}

	update_lastpost(currboard);
	presskeyfor("删除失败...");
	return PARTUPDATE;
}

#ifdef QCLEARNEWFLAG
int
flag_clearto(int ent, char *direct, int clearall)
{
	int i, fd;
	struct fileheader f_info;

	if (uinfo.mode != READING)
		return DONOTHING;

	if ((fd = open(direct, O_RDONLY, 0)) == -1)
		return DONOTHING;

	if (clearall) {
		lseek(fd, (off_t) ((-sizeof(struct fileheader)) * (BRC_MAXNUM + 1)), SEEK_END);
		while (read(fd, &f_info, sizeof(struct fileheader)) == sizeof(struct fileheader))
			brc_addlist(f_info.filetime);
	} else {
		lseek(fd, (off_t) ((ent - 1) * sizeof(struct fileheader)), SEEK_SET);
		read(fd, &f_info, sizeof(struct fileheader));
		for (i = f_info.filetime - BRC_MAXNUM; i <= f_info.filetime; i++)
			brc_addlist(i);
	}

	close(fd);
	return PARTUPDATE;
}
#else
int
flag_clearto(int ent, char *direct, int clearall)
{
	int fd, i;
	struct fileheader f_info;

	if (uinfo.mode != READING)
		return DONOTHING;
	if ((fd = open(direct, O_RDONLY, 0)) == -1)
		return DONOTHING;
	for (i = 0; clearall || i < ent; i++) {
		if (read(fd, &f_info, sizeof(struct fileheader)) != (struct fileheader))
			break;
		brc_addlist(f_info.filetime);
	}
	close(fd);
	return PARTUPDATE;
}
#endif

int
new_flag_clearto(int ent, struct fileheader *fileinfo, char *direct)
{
	return flag_clearto(ent, direct, NA);
}

int
new_flag_clear(int ent, struct fileheader *fileinfo, char *direct)
{
	extern int brc_num, brc_cur;

	brc_num = 0;
	brc_cur = 0;
	brc_insert(time(NULL));

	return PARTUPDATE;      /* return flag_clearto(ent, direct, YEA); */
}

int
save_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if (!current_bm && !HAS_PERM(PERM_ANNOUNCE))
		return DONOTHING;

	return ann_savepost(currboard, fileinfo, NA);
}

/* monster: 
 *
 *	更新ainfo.title, 使atrace能记录正确的操作位置 
 *
 *	注意，update_ainfo_title(YEA) 和 update_ainfo_title(NA)必须配对，
 *	且次序不能颠倒，不能连续调用update_ainfo_title(YEA)或者update_ainfo_title(NA)。
 */
void
update_ainfo_title(int import)
{
	static char title[TITLELEN];

	if (import == YEA && INMAIL(uinfo.mode)) {
		strlcpy(title, ainfo.title, sizeof(title));
		snprintf(ainfo.title, sizeof(ainfo.title), "%s的邮箱", currentuser.userid);
	} else {
		strlcpy(ainfo.title, title, sizeof(ainfo.title));
	}
}

int
import_post(int ent, struct fileheader *fileinfo, char *direct)
{
	char fname[PATH_MAX + 1];
	char *ptr;

	if (!current_bm && !HAS_PERM(PERM_ANNOUNCE) && !HAS_PERM(PERM_PERSONAL))
		return DONOTHING;

	if (fileinfo->flag & FILE_VISIT) {
		if (askyn("文章曾经放入精华区, 现在还要再放入吗", YEA, YEA) == NA)
			return PARTUPDATE;
	}

	strlcpy(fname, direct, sizeof(fname));
	if ((ptr = strrchr(fname, '/')) == NULL)
		return DONOTHING;
	strcpy(ptr + 1, fileinfo->filename);

	update_ainfo_title(YEA);
	if (ann_import_article(fname, fileinfo->title, fileinfo->owner, fileinfo->flag & FILE_ATTACHED, NA) == 0) {
		fileinfo->flag |= FILE_VISIT;    /* 将标志置位 */
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
		presskeyfor("文章已放入精华区, 按任意键继续...");
	} else {
		presskeyfor("对不起, 您没有设定丝路或丝路设定有误. 请用 f 设定丝路.");
	}
	update_ainfo_title(NA);

	return PARTUPDATE;
}

int
select_post(int ent, struct fileheader *fileinfo, char *direct)
{
	if ((!current_bm && !HAS_PERM(PERM_ANNOUNCE)) || digestmode == 1 || digestmode > 7)
		return DONOTHING;

	if (fileinfo->flag & FILE_SELECTED) {
		fileinfo->flag &= ~FILE_SELECTED;
	} else {
		fileinfo->flag |= FILE_SELECTED;
	}

	/* freestyler: 非标准阅读模式选择文章 */
	int savedigestmode = digestmode;
	digestmode = 0;
	setbdir(direct, currboard);
	safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
	if ( savedigestmode != 0 ) {
		digestmode = savedigestmode;
		setbdir(direct, currboard);
		safe_substitute_record(direct, fileinfo, ent, (digestmode==2) ? NA : YEA);
	}
	return PARTUPDATE;
}

int
process_select_post(int ent, struct fileheader *fileinfo, char *direct)
{
	return bmfunc(ent, fileinfo, direct, 4);
}

int
author_operate(int ent, struct fileheader *fileinfo, char *direct)
{
	int bm, anony, action;
	char ans[3], msgbuf[1024], repbuf[STRLEN], fname[PATH_MAX + 1];
	int deny_attach;
	struct user_info *uin;
	struct boardheader *bp;

	if (guestuser || !strcmp(currentuser.userid, fileinfo->owner) ||
	    strchr(fileinfo->owner, '.'))
		return DONOTHING;

	bm = (current_bm || HAS_PERM(PERM_SYSOP));

	/* monster: 检查文章作者是否为匿名账号 */
	bp = getbcache(currboard);
	anony = (bp->flag & ANONY_FLAG);
	if (anony && !strcmp(fileinfo->owner, currboard)) {
		if (bm) {
			getdata(t_lines - 1, 0, "文章作者: 1) 封禁 0) 取消 [0]: ", ans, 2, DOECHO, YEA);
			if (ans[0] == '1') {
				action = 4;
				goto process_actions;
			}
		}
		return PARTUPDATE;
	}

	getdata(t_lines - 1, 0, bm ?
		"文章作者: 1) 发送信息  2) 设为好友 3) 设为坏人 4) 封禁 0) 取消 [0]: "
		:
		"文章作者: 1) 发送信息  2) 设为好友 3) 设为坏人 0) 取消 [0]: ",
		ans, 2, DOECHO, YEA);

	action = ans[0] - '0';
	if (action < 1 || (bm && action > 4) || (!bm && action > 3))
		return PARTUPDATE;

      process_actions:

	switch (action) {
	case 1:         /* 发送信息 */
		uin = (struct user_info *) t_search(fileinfo->owner, NA);
		if (uinfo.invisible && !HAS_PERM(PERM_SYSOP)) {
			presskeyfor("抱歉, 此功能在隐身状态下不能执行，请按<Enter>继续...");
			break;
		}
		if (!HAS_PERM(PERM_PAGE) || !uin || !canmsg(uin) || uin->mode == BBSNET || INBBSGAME(uin->mode)) {
			presskeyfor("暂无法发送信息给文章作者, 请按<Enter>继续...");
			break;
		}
		/*
		getdata(t_lines - 1, 0, "音信 : ", msgbuf, 55, DOECHO, YEA);
		*/
		multi_getdata(0, 0, "音信 : ", msgbuf, MSGLEN, 80, MSGLINE, YEA); /* Pudding: 与多行讯息兼容 */
		if (msgbuf[0] != '\0') {
			do_sendmsg(uin, msgbuf, 2, uin->pid);
		}
		break;
	case 2:         /* 加为好友 */
		friendflag = 1;
		addtooverride(fileinfo->owner);
		break;
	case 3:         /* 加为坏人 */
		friendflag = 0;
		addtooverride(fileinfo->owner);
		break;
	case 4:         /* 封禁 */
		if (!(anony && !strcmp(fileinfo->owner, currboard)) && !getuser(fileinfo->owner, NULL)) {
			presskeyfor("无法封禁文章作者, 请按<Enter>继续...");
			break;
		}
		/* Pudding: 让版主选择是否公开附文 */
		/*
		deny_attach = askyn("是否公开附文(匿名版无效)", YEA, YEA);
		*/
		deny_attach = YEA;
		stand_title("封禁文章作者");
		if (anony) {
			if (addtodeny(strcmp(fileinfo->owner, currboard) ? fileinfo->owner : fileinfo->realowner, msgbuf, 0, D_ANONYMOUS, fileinfo) == 1) {
				snprintf(repbuf, sizeof(repbuf), "%s 被取消在 %s 版的发文权利", fileinfo->realowner, currboard);
				securityreport(repbuf);
				if (msgbuf[0] != '\0') {
//					strlcat(msgbuf, "附文：\n\n", sizeof(msgbuf));
					setboardfile(fname, currboard, fileinfo->filename);
					autoreport(repbuf, msgbuf, YEA, fileinfo->realowner, NULL);
				}
			}
		} else {
			if (addtodeny(fileinfo->owner, msgbuf, 0, 0, fileinfo) == 1) {
				snprintf(repbuf, sizeof(repbuf), "%s 被取消在 %s 版的发文权利", fileinfo->owner, currboard);
				securityreport(repbuf);
				if (msgbuf[0] != '\0') {
					if (deny_attach) strlcat(msgbuf, "附文：\n\n", sizeof(msgbuf));
					setboardfile(fname, currboard, fileinfo->filename);
					autoreport(repbuf, msgbuf, YEA, fileinfo->owner, deny_attach ? fname : NULL);
				}
			}
		}
		break;
	}
	return FULLUPDATE;
}

int
denyuser(void)
{
	char msgbuf[1024], repbuf[STRLEN], uident[IDLEN + 2];

	stand_title("封禁使用者");
	move(2, 0);
	usercomplete("输入准备加入封禁名单的使用者ID: ", uident);

	if (uident[0] && getuser(uident, NULL) && addtodeny(uident, msgbuf, 0, D_NOATTACH, NULL) == 1) {
		snprintf(repbuf, sizeof(repbuf), "%s 被取消在 %s 版的发文权利", uident, currboard);
		securityreport(repbuf);
		if (msgbuf[0] != '\0') {
			autoreport(repbuf, msgbuf, YEA, uident, NULL);
		}
	}
	return FULLUPDATE;
}

int
A_action(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode == 10 || digestmode == 11) {
		return denyuser();
	} else {
		return auth_search_up(ent, fileinfo, direct);
	}
}

int
a_action(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode == 10 || digestmode == 11) {
		return denyuser();
	} else {
		return auth_search_down(ent, fileinfo, direct);
	}
}

int
del_deny(int ent, struct denyheader *fileinfo, char *direct)
{
	char fname[STRLEN];

	move(t_lines - 1, 0);
	if (askyn("确定恢复封禁者的发文权利", NA, NA) == YEA &&
		 (delfromdeny(fileinfo->blacklist, (digestmode == 10) ?
		  D_ANONYMOUS | D_IGNORENOUSER : D_IGNORENOUSER) == 1)) {
		setboardfile(fname, currboard, fileinfo->filename);
		unlink(fname);
		delete_record(currdirect, sizeof (struct denyheader), ent);

		if (get_num_records(currdirect, sizeof (struct denyheader)) == 0) {
			digestmode = NA;
			setbdir(currdirect, currboard);
		}
	}
	return NEWDIRECT;
}

int
d_action(int ent, struct fileheader *fileinfo, char *direct)
{
	return (digestmode == 10 || digestmode == 11) ?
		del_deny(ent, (struct denyheader *)fileinfo, direct) :
		del_post(ent, fileinfo, direct);
}

int
change_deny(int ent, struct denyheader *fileinfo, char *direct)
{
	char repbuf[STRLEN], msgbuf[1024];

	stand_title("封禁文章作者 (修改)");
	if (addtodeny(fileinfo->blacklist, msgbuf, 1, (digestmode == 10) ? D_ANONYMOUS : 0, (struct fileheader *)fileinfo) == 1) {
		delete_record(currdirect, sizeof (struct denyheader), ent);
		snprintf(repbuf, sizeof(repbuf), "修改对 %s 被取消 %s 版发文权利的处理",
			fileinfo->blacklist, currboard);
		securityreport(repbuf);
		if (msgbuf[0] != '\0') {
			autoreport(repbuf, msgbuf, (digestmode == 10) ? NA : YEA, fileinfo->blacklist, NULL);
		}
	}
	return NEWDIRECT;
}

int
E_action(int ent, struct fileheader *fileinfo, char *direct)
{
	switch (digestmode) {
	case 0:
		return edit_post(ent, fileinfo, direct);
		break;
		/* freestyler: 非标准阅读模式编辑文章, 
		 * 没启用MARK_X_FLAG, edit_post将不会用到ent, direct参数和digestmode全局变量 
		 * 否则要相应处理ent, direct, digestmode */
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		return edit_post(ent, fileinfo, direct);
		break;
	case 8:
	case 9:
		return empty_recyclebin(ent, fileinfo, direct);
		break;
	case 10:
	case 11:
		return change_deny(ent, (struct denyheader *)fileinfo, direct);
		break;
	}
	return DONOTHING;
}

/* gcc:编辑文章属性 */
unsigned int
showfileinfo(unsigned int pbits, int i, int flag)
{
	move (i + 6, 0);
	prints("%s%c. %-30s %2s\033[m",
	       1 ? "\033[m" : "\033[1;30m",
	       'A' + i,
	       fileinfo_strings[i],
	       ((pbits >> i) & 1 ? "是" : "否"));
	refresh();
	return YEA;
}

/* gcc：编辑文章属性，增加新项时注意权限 */
int
edit_property(int ent, struct fileheader *fileinfo, char *direct)
{
	if (digestmode == 1 || digestmode > 7)
		return DONOTHING;
	/* if (!isowner(&currentuser, fileinfo)) */
	if (strcmp(fileinfo->owner, currentuser.userid) ||
	    fileinfo->filetime < currentuser.firstlogin
	    )
		return DONOTHING;
    
	unsigned int oldflag = 0;
	unsigned int newflag = 0;
	move(1, 0);
	clrtobot();
	move(2, 0);

	if (fileinfo->flag & FILE_MAIL)
		oldflag |= 1;

	newflag = setperms(oldflag, "文章属性", NUMFILEINFO, showfileinfo);
	move(2, 0);
	if (newflag == oldflag)
		outs("参数没有修改...\n");
	else {
		if ((newflag ^ oldflag) & 1) {
			if (fileinfo->flag & FILE_MAIL)
				fileinfo->flag &= ~FILE_MAIL;
			else
				fileinfo->flag |= FILE_MAIL;
		}
		int savedigestmode = digestmode;
		digestmode = 0;
		setbdir(direct, currboard);
		safe_substitute_record(direct, fileinfo, ent, (digestmode == 0) ? YEA : NA);
		if ( savedigestmode != 0 ) {
			digestmode = savedigestmode;
			setbdir(direct, currboard);
			safe_substitute_record(direct, fileinfo, ent, (digestmode == 2) ? NA : YEA);
		}

		outs("新的参数设定完成...\n\n");
	}

	pressreturn();
	return FULLUPDATE;
}


int
toggle_previewmode(int ent, struct fileheader *fileinfo, char *direct)
{
	extern int screen_len;
	if (uinfo.mode == READING) {
		screen_len = (screen_len == t_lines - 4) ? (t_lines - 4) / 2 : t_lines - 4;
		previewmode = !previewmode;
		return MODECHANGED;
	}
	return DONOTHING;
	
}
#ifdef ZMODEM

int
zmodem_transferfile(char *localfile, char *remotefile)
{
	char fname[STRLEN];

	move(2, 0);
	clrtobot();

	outs("请直接按 Enter 接受括号内提示的文件名, 或者输入其他文件名;\n按 * 中止当前文章的传输.\n\n");
	prints("把文章保存为 [%s]\n", remotefile);
	getdata(6, 0, "==> ", fname, sizeof(fname), YEA, YEA);

	if (fname[0] == '*')
		return -2;

	if (fname[0] == '\0')
		strlcpy(fname, remotefile, sizeof(fname));
	fixstr(fname, "\\/:*?\"<>|", ' ');

	if (bbs_zsendfile(localfile, fname) == -1) {
		clear_line(7);
		outs("传输失败：请确认终端是否支持 Zmodem 传输协议");
		redoscr();
		pressanykey();
		return -1;
	}

	return 0;
}

int
zmodem_transfer(int ent, struct fileheader *fileinfo, char *direct)
{
	char localfile[PATH_MAX + 1], remotefile[PATH_MAX + 1];

	stand_title("文章下载 (Zmodem)");

	setboardfile(localfile, currboard, fileinfo->filename);
	snprintf(remotefile, sizeof(remotefile), "%s.txt", fileinfo->title);

	do {
		if (zmodem_transferfile(localfile, remotefile) == -1)
			return FULLUPDATE;
	} while (0); /* monster: todo - attachment support */

	pressanykey();
	return FULLUPDATE;
}

int
Y_action(int ent, struct fileheader *fileinfo, char *direct)
{
	switch(digestmode) {
	case 8:
	case 9:
		return undel_range(ent, fileinfo, direct);
		break;
	case 10:
	case 11:
		break;
	default:
		return zmodem_transfer(ent, fileinfo, direct);
		break;
	}
	return DONOTHING;
}
#endif

int
show_b_secnote(void)
{
	char buf[256];

	clear();
	setvotefile(buf, currboard, "secnotes");
	if (dashf(buf)) {
		if (!check_notespasswd())
			return FULLUPDATE;
		clear();
		ansimore(buf, NA);
	} else {
		move(3, 25);
		prints("此讨论区尚无「秘密备忘录」。");
	}
	pressanykey();
	return FULLUPDATE;
}

int
show_b_note(void)
{
	clear();
	if (show_board_notes(currboard, YEA) == -1) {
		move(3, 30);
		prints("此讨论区尚无「备忘录」。");
		pressanykey();
	}
	return FULLUPDATE;
}

#ifdef INTERNET_EMAIL
int
forward_post(int ent, struct fileheader *fileinfo, char *direct)
{
	return (mail_forward(ent, fileinfo, direct));
}

int
forward_u_post(int ent, struct fileheader *fileinfo, char *direct)
{
	return (mail_u_forward(ent, fileinfo, direct));
}
#endif

int
show_fileinfo(int ent, struct fileheader *fileinfo, char *direct)
{
	/* get file size */
	struct stat st;
	char buf[512], filepath[512];
	char *t;
	
	strlcpy(buf, direct, sizeof(buf));
	if ((t = strrchr(buf, '/')) != NULL)
		*t = '\0';
	
	snprintf(filepath, sizeof(filepath), "%s/%s", buf, fileinfo->filename);
	if (stat(filepath, &st) != -1)
		fileinfo->size = st.st_size;
	
	clear();
	move(5, 0);
	prints("本文链接:\n");
	prints("http://%s/bbscon?board=%s&file=%s\n\n",
	       BBSHOST, currboard, fileinfo->filename);
	if (HAS_PERM(PERM_SYSOP))
		prints("发表时间: %d\n\n", fileinfo->filetime);
	prints("文章大小: %d 字节\n\n", fileinfo->size);
	strcpy(quote_board, currboard);
	if (getattachinfo(fileinfo))
		prints("%s\n%s\n", attach_info, attach_link);
	pressanykey();
	return FULLUPDATE;
}


struct one_key read_comms[] = {
	{ '_', underline_post },            /* 设置不可回复标记 */
//      { 'w', makeDELETEDflag },           /* 设置水文标记 */
#ifdef ZMODEM
	{ 'Y', Y_action },                  /* 通过Zmodem协议下载文章 (monster) */
#else
	{ 'Y', undel_range },               /* 批量恢复文件 */
#endif
	{ 'y', undel_post },                /* 恢复文章 */
	{ 'r', read_post },                 /* 阅读文章 */
	{ 'K', skip_post },                 /* 跳过文章 */
	{ 'd', d_action },                  /* 删除文章/解除匿名封禁 (monster) */
	{ 'E', E_action },                  /* 编辑文章/清空回收站 (monster) */
	{ ':', edit_property },             /* 编辑文章属性(gcc) */
	{ 'D', del_range },                 /* 区段删除文章 */
	{ 'm', mark_post },                 /* 标记文章 (m) */
	{ 'g', digest_post },               /* 标记文章 (g) */
	{ 'e', select_post },               /* 标记文章 ($) (monster) */
	{ Ctrl('G'), digest_mode },         /* 文摘模式 */
	{ '`', digest_mode },               /* 文摘模式 */
	{ Ctrl('Y'), pure_mode },           /* 原作模式 (monster) */
	{ Ctrl('T'), acction_mode },        /* 多种模式切换 */
	{ 't', thesis_mode },               /* 主题模式 */
	{ '.', deleted_mode },              /* 回收站 */
	{ '>', junk_mode },                 /* 废纸篓 */
	{ 'T', edit_title },                /* 更改文章标题 */
	{ 's', do_select },                 /* 选择版面 */
	{ Ctrl('C'), do_cross },            /* 转载文章 */
	{ Ctrl('P'), do_post },             /* 发表文章 */
	{ 'C', new_flag_clearto },          /* 清除未读标记到当前位置 */
	{ 'c', new_flag_clear },            /* 清全部未读标记 */
#ifdef INTERNET_EMAIL
	{ 'F', mail_forward },              /* 寄回邮箱 */
	{ 'U', mail_u_forward },            /* 寄回邮箱 (UUENCODE) */
	{ Ctrl('R'), post_reply },          /* 回信给原作者 */
#endif
	{ 'i', save_post },            	    /* 把文章存入暂存档 */
	{ 'I', import_post },               /* 把文章放入精华区 */
	{ Ctrl('E'), process_select_post }, /* 处理选定文章 (monster) */
	{ Ctrl('V'), x_lockscreen_silent }, /* 锁屏 (monster) */
	{ Ctrl('O'), author_operate },      /* 对当前文章作者操作 (monster) */
	{ 'R', b_results },                 /* 查看前次投票结果 */
	{ 'v', b_vote },                    /* 投票 */
	{ 'V', b_vote_maintain },           /* 投票管理 */
	{ 'W', b_notes_edit },              /* 编辑备忘录 */
	{ Ctrl('W'), b_notes_passwd },      /* 设定秘密备忘录密码 */
	{ 'h', mainreadhelp },              /* 显示帮助 */
	{ Ctrl('J'), mainreadhelp },        /* 显示帮助 */
	{ KEY_TAB, show_b_note },           /* 查看备忘录 */
	{ 'z', show_b_secnote },            /* 查看秘密备忘录 */
	{ 'x', currboard_announce },	    /* 查看精华区 */
	{ 'X', author_announce },           /* 查看个人精华区 */
	{ Ctrl('X'), pannounce },           /* 查看指定用户的个人精华区 */
	{ 'a', a_action },                  /* 向后搜索作者/封禁 */
	{ 'A', A_action },                  /* 向前搜索作者/封禁 */
	{ '/', title_search_down },         /* 向后搜索标题 */
	{ '?', title_search_up },           /* 向前搜索标题 */
	{ '\'', post_search_down },         /* 向后搜索内容 */
	{ '\"', post_search_up },           /* 向前搜索内容 */
	{ ']', thread_search_down },        /* 向后搜索主题 */
	{ '[', thread_search_up },          /* 向前搜索主题 */
	{ Ctrl('D'), deny_mode },           /* 封禁用户 */
	{ Ctrl('K'), control_user },        /* 编辑限制版名单 (monster) */
	{ Ctrl('A'), show_author },         /* 查询文章作者 */
	{ Ctrl('N'), SR_first_new },
	{ 'n', SR_first_new },
	{ '\\', SR_last },
	{ '=', SR_first },
	{ '%', jump_to_reply },	 	    /* 跳到回复 (freestyler) */
	{ Ctrl('S'), SR_read },
	{ 'p', SR_read },
	{ Ctrl('U'), SR_author },
	{ 'b', bmfuncs },                   /* 版主特殊功能 */
	{ '!', Q_Goodbye },                 /* 快速离站 */
	{ 'S', s_msg },                     /* 发送讯息 */
	{ 'f', t_friends },                 /* 寻找好友/环顾四方 */
	{ 'o', fast_cloak },                /* 版面快速隐身切换 */
	{ 'L', show_allmsgs },		    /* 显示所有讯息 */
	{ ',', toggle_previewmode },	    /* 切换预览模式 */
#ifdef RECOMMEND
	{ '<', do_recommend },		    /* 推荐文章 */
#endif
	{ '*', show_fileinfo },		    /* 显示全文链接等 */
	{ '\0', NULL }
};

int
Read(void)
{
	char buf[STRLEN];
	char notename[STRLEN];
	time_t usetime;
	struct stat st;

	if (currboard[0] == 0) {
		move(2, 0);
		prints("请先选择讨论区\n");
		pressreturn();
		clear_line(2);
		return -1;
	}
	brc_initial(currboard);
	setbdir(buf, currboard);
	setvotefile(notename, currboard, "notes");
	if (stat(notename, &st) != -1) {
		if (st.st_mtime < (time(NULL) - 7 * 86400)) {
			utimes(notename, NULL);
			setvotefile(genbuf, currboard, "noterec");
			unlink(genbuf);
		}
	}
#ifdef ALWAYS_SHOW_BRDNOTE
	if (dashf(notename))
		ansimore3(notename, YEA);
#else
	if (vote_flag(currboard, '\0', 1 /* 检查读过新的备忘录没 */ ) ==
	    0) {
		if (dashf(notename)) {
			ansimore3(notename, YEA);
			vote_flag(currboard, 'R', 1 /* 写入读过新的备忘录 */ );
		}
	}
#endif

	usetime = time(NULL);
#ifdef INBOARDCOUNT
	int idx = getbnum(currboard);
	board_setcurrentuser(idx-1, 1);
	is_inboard = 1;
#endif 
	
	i_read(READING, buf, NULL, NULL, readtitle, readdoent, update_endline,
	       &read_comms[0], get_records, get_num_records, sizeof(struct fileheader));
	board_usage(currboard, time(NULL) - usetime);
	brc_update();

#ifdef 	INBOARDCOUNT  
	idx = getbnum(currboard);
	board_setcurrentuser(idx-1, -1);
	is_inboard = 0;
#endif 

	return 0;
}

/*Add by SmallPig*/
void
notepad(void)
{
	char tmpname[PATH_MAX + 1], note1[4];
	char note[3][STRLEN - 4]; /* 3行内容 */
	char tmp[STRLEN];
	FILE *in;
	int i, n;
	time_t thetime = time(NULL);

	clear();
	move(0, 0);
	prints("开始你的留言吧！大家正拭目以待....\n");
	prints("请勿在留言版发表违反站规的内容(如粗口, 商业广告), 违者依法处罚\n");
	modify_user_mode(WNOTEPAD);
	snprintf(tmpname, sizeof(tmpname), "tmp/notepad.%s.%05d", currentuser.userid, uinfo.pid);
	if ((in = fopen(tmpname, "w")) != NULL) {
		for (i = 0; i < 3; i++)
			memset(note[i], 0, STRLEN - 4);
		while (1) {
			for (i = 0; i < 3; i++) {
				getdata(2 + i, 0, ": ", note[i],
					STRLEN - 5, DOECHO, NA);
				if (note[i][0] == '\0')
					break;
			}
			if (i == 0) {
				fclose(in);
				unlink(tmpname);
				return;
			}
			getdata(5, 0,
				"是否把你的大作放入留言板 (Y)是的 (N)不要 (E)再编辑 [Y]: ",
				note1, 3, DOECHO, YEA);
			if (note1[0] == 'e' || note1[0] == 'E')
				continue;
			else
				break;
		}
		if (note1[0] != 'N' && note1[0] != 'n') {
			snprintf(tmp, sizeof(tmp), "\033[1;32m%s\033[37m（%.18s）",
				currentuser.userid, currentuser.username);
			fprintf(in,
				"\033[1;34m▕\033[44m▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔\033[36m酸\033[32m甜\033[33m苦\033[31m辣\033[37m版\033[34m▔▔▔▔▔▔▔▔▔▔▔▔▔▔▔\033[44m◣\033[m\n");
			getdatestring(thetime);
			fprintf(in,
				"\033[1;34m▕\033[32;44m %-44s\033[32m在 \033[36m%23.23s\033[32m 离开时留下的话  \033[m\n",
				tmp, datestring + 6);
			for (n = 0; n < i; n++) {
				if (note[n][0] == '\0')
					break;
				fprintf(in,
					"\033[1;34m▕\033[33;44m %-75.75s\033[1;34m\033[m \n",
					note[n]);
			}
			fprintf(in,
				"\033[1;34m▕\033[44m ───────────────────────────────────── \033[m \n");
			catnotepad(in, "etc/notepad"); /* 将'etc/notepad'第2行开始的内容添加到 in 后面 */

			fclose(in);
			f_mv(tmpname, "etc/notepad"); 
		} else {
			fclose(in);
			unlink(tmpname);
		}
	}
	if (talkrequest) {
		talkreply();
	}
	clear();
	return;
}

/* youzi quick goodbye */
int
Q_Goodbye(void)
{
	extern int started;
	extern int *zapbuf;
	char fname[STRLEN];
	int logouts;

	/* freestyler: 保存阅读标记 */
	brc_update();
	
	free(zapbuf);
	setuserfile(fname, "msgfile");
#ifdef LOG_MY_MESG
	if (count_self() == 1) {
		unlink(fname);
		setuserfile(fname, "allmsgfile");
	}
#endif

	/* edwardc.990423 讯息浏览器 */
	if (dashf(fname) && DEFINE(DEF_MAILMSG) && count_self() == 1)
		mesgmore(fname);

	clear();
	prints("\n\n\n\n");
	setuserfile(fname, "notes");
	if (dashf(fname))
		ansimore(fname, YEA);
	setuserfile(fname, "logout");
	if (dashf(fname)) {
		logouts = countlogouts(fname);
		if (logouts >= 1) {
			user_display(fname,
				     (logouts ==
				      1) ? 1 : (currentuser.numlogins %
						(logouts)) + 1, YEA);
		}
	} else {
		if (fill_shmfile(2, "etc/logout", "GOODBYE_SHMKEY"))
			show_goodbyeshm();
	}
	pressreturn();          // sunner 的建议
	clear();
	refresh();
	report("exit");
	if (started) {
		time_t stay;

		stay = time(NULL) - login_start_time;
		snprintf(genbuf, sizeof(genbuf), "Stay:%3d (%s)", stay / 60, currentuser.username);
		log_usies("EXIT ", genbuf);
		u_exit();
	}


#ifdef CHK_FRIEND_BOOK
	if (num_user_logins(currentuser.userid) == 0 && !guestuser) {
		FILE *fp;
		char buf[STRLEN], *ptr;

		if ((fp = fopen("friendbook", "r")) != NULL) {
			while (fgets(buf, sizeof (buf), fp) != NULL) {
				char uid[14];

				ptr = strstr(buf, "@");
				if (ptr == NULL) {
					del_from_file("friendbook", buf);
					continue;
				}
				ptr++;
				strcpy(uid, ptr);
				ptr = strstr(uid, "\n");
				*ptr = '\0';
				if (!strcmp(uid, currentuser.userid))
					del_from_file("friendbook", buf);
			}
			fclose(fp);
		}
	}
#endif
	deattach_shm();
	sleep(1);
	exit(0);
	return -1;
}

int
Goodbye(void)
{
	char sysoplist[20][41], syswork[20][41], buf[STRLEN];
	int i, num_sysop, choose;
	FILE *sysops;
	char *ptr;

	*quote_file = '\0';
	i = 0;
	if ((sysops = fopen("etc/sysops", "r")) != NULL) {
		while (fgets(buf, STRLEN, sysops) != NULL && i <= 19) {
			if (buf[0] == '#')
				continue;
			ptr = strtok(buf, " \n\r\t");
			if (ptr) {
				strlcpy(sysoplist[i], ptr, sizeof(sysoplist[i]));
				ptr = strtok(NULL, " \n\r\t");
				strlcpy(syswork[i], (ptr == NULL) ?  "[职务不明]" : ptr, sizeof(syswork[i]));
				i++;
			}
		}
		fclose(sysops);
	}

	num_sysop = i;
	move(1, 0);
	alarm(0);
	clear();
	move(0, 0);
	prints("你就要离开 %s ，可有什么建议吗？\n", BoardName);
	prints("[\033[1;33m1\033[m] 寄信给管理人员\n");
	prints("[\033[1;33m2\033[m] 按错了啦，我还要玩\n");
#ifdef USE_NOTEPAD
#ifdef AUTHHOST
	if (HAS_PERM(PERM_WELCOME)) {
#endif
	if (!guestuser) {
		prints("[\033[1;33m3\033[m] 写写\033[1;32m留\033[33m言\033[35m版\033[m罗\n");
	}
#ifdef AUTHHOST
	}
#endif
#endif
	if (!guestuser && HAS_PERM(PERM_MESSAGE)) {
		prints("[\033[1;33m4\033[m]\033[1;32m 给朋友发个消息 :)\033[m\n");
	}

	prints("[\033[1;33m5\033[m] 不寄罗，要离开啦\n");
	strlcpy(buf, "你的选择是 [\033[1;32m5\033[m]：", sizeof(buf));
	getdata(8, 0, buf, genbuf, 4, DOECHO, YEA);
	clear();
	choose = genbuf[0] - '0';

	switch (choose) {
	case 1:
		outs("     站长的 ID    负 责 的 职 务\n");
		outs("     ============ =====================\n");
		for (i = 1; i <= num_sysop; i++) {
			prints("[\033[1;33m%2d\033[m] %-12s %s\n", i,
			       sysoplist[i - 1], syswork[i - 1]);
		}
		prints("[\033[1;33m%2d\033[m] 还是走了罗！\n", num_sysop + 1);
		snprintf(buf, sizeof(buf), "你的选择是 [\033[1;32m%d\033[m]：", num_sysop + 1);
		getdata(num_sysop + 5, 0, buf, genbuf, 4, DOECHO, YEA);
		choose = atoi(genbuf);
		if (choose >= 1 && choose <= num_sysop)
			do_send(sysoplist[choose - 1], "使用者寄来的建议信", NA, 0);
		break;
	case 2:
		return FULLUPDATE;
#ifdef USE_NOTEPAD
	case 3:
		if (!guestuser && HAS_PERM(PERM_WELCOME) && choose == 3)
			notepad();
		break;
#endif
	case 4:
		if (HAS_PERM(PERM_MESSAGE) && choose == 4)
			friend_wall();
		break;
	}
	return Q_Goodbye();
}

void
board_usage(char *mode, time_t usetime)
{
	time_t now;
	char buf[256];

	now = time(NULL);
	snprintf(buf, sizeof(buf), "%24.24s USE %-20.20s Stay: %5d (%s)\n",
		ctime(&now), mode, usetime, currentuser.userid);
	do_report(LOG_BOARD, buf);
}

int
Info(void)
{
	modify_user_mode(XMENU);
	ansimore("Version.Info", YEA);
	clear();
	return 0;
}

int
Conditions(void)
{
	modify_user_mode(XMENU);
	ansimore("COPYING", YEA);
	clear();
	return 0;
}

int
Welcome(void)
{
	char ans[3];

	modify_user_mode(XMENU);
	if (!dashf("etc/Welcome2"))
		ansimore("etc/Welcome", YEA);
	else {
		clear();
		stand_title("观看进站画面");
		for (;;) {
			getdata(1, 0,
				"(1)特殊进站公布栏  (2)本站进站画面 ? : ", ans,
				2, DOECHO, YEA);
			/* skyo.990427 modify  按 Enter 跳出  */
			if (ans[0] == '\0') {
				clear();
				return 0;
			}
			if (ans[0] == '1' || ans[0] == '2')
				break;
		}
		if (ans[0] == '1')
			ansimore("etc/Welcome", YEA);
		else
			ansimore("etc/Welcome2", YEA);
	}
	clear();
	return 0;
}

int
cmpbnames(void *bname_ptr, void *brec_ptr)
{
	char *bname = (char *)bname_ptr;
	struct fileheader *brec = (struct fileheader *)brec_ptr;

	return (!strncasecmp(bname, brec->filename, sizeof(brec->filename))) ? YEA : NA;
}

/*
 *  by ylsdd, modified by monster
 *
 *  unlink action is taked within cancelpost if in mail mode,
 *  otherwise this item is added to the file '.DELETED' under
 *  the board's directory, the filename is not changed.
 *  Unlike the fb code which moves the file to the deleted
 *  board.
 */
void
cancelpost(char *board, char *userid, struct fileheader *fh, int owned, int keepid)
{
	struct fileheader postfile;
	char oldpath[PATH_MAX + 1];
	int tmpdigestmode;

	if (uinfo.mode == RMAIL) {
		snprintf(oldpath, sizeof(oldpath), "mail/%c/%s/%s",
			mytoupper(currentuser.userid[0]),
			currentuser.userid, fh->filename);
		unlink(oldpath);
		return;
	}

/*
 *	memset(&postfile, 0, sizeof (postfile));
 *	strcpy(postfile.filename, fh->filename);
 *	strlcpy(postfile.owner, fh->owner, IDLEN + 2);
 *	postfile.owner[IDLEN + 1] = 0;
 */

	memcpy(&postfile, fh, sizeof(postfile));
	if (keepid == YEA)
		snprintf(postfile.title, sizeof(postfile.title), "%-32.32s - %s", fh->title, userid);
	postfile.flag = 0;
	tmpdigestmode = digestmode;
	digestmode = (owned) ? 9 : 8;
	setbdir(genbuf, board);
	append_record(genbuf, &postfile, sizeof(postfile));

	digestmode = tmpdigestmode;
}

int
thesis_mode(void)
{
	int id;
	struct userec lookupuser;

	id = getuser(currentuser.userid, &lookupuser);
	lookupuser.userdefine ^= DEF_THESIS;
	currentuser.userdefine ^= DEF_THESIS;
	substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), id);
	update_utmp();
	return FULLUPDATE;
}

int
marked_all(int type)
{
	struct fileheader post;
	int fd, fd2;
	char fname[PATH_MAX + 1], tname[PATH_MAX + 1];
	char tempname1[TITLELEN + 1], tempname2[51];

	snprintf(fname, sizeof(fname), "boards/%s/%s", currboard, DOT_DIR);
	switch (type) {
	case 0:
		snprintf(tname, sizeof(tname), "boards/%s/%s", currboard, MARKED_DIR);
		break;
	case 1:
		snprintf(tname, sizeof(tname), "boards/%s/%s", currboard, AUTHOR_DIR);
		break;
	case 2:
	case 3:
		snprintf(tname, sizeof(tname), "boards/%s/SOMEONE.%s.DIR.%d", currboard,
			someoneID, type - 2);
		break;
	case 4:
		snprintf(tname, sizeof(tname), "boards/%s/KEY.%s.DIR", currboard,
			currentuser.userid);
		break;
	}

	if ((fd = open(fname, O_RDONLY, 0)) == -1)
		return -1;

	if ((fd2 = open(tname, O_CREAT | O_WRONLY | O_TRUNC, 0644)) == -1) {
		close(fd);
		return -1;
	}

	while (read(fd, &post, sizeof (struct fileheader)) == sizeof(struct fileheader)) {
		switch (type) {
		case 0:
			if (post.flag & FILE_MARKED) {
				safewrite(fd2, &post, sizeof(post));
			}
			break;
		case 1:
			if (strncmp(post.title, "Re: ", 4) || post.flag & (FILE_MARKED | FILE_DIGEST)) {
				safewrite(fd2, &post, sizeof (post));
			}
			break;
		case 2:
			strtolower(tempname1, post.owner);
			strtolower(tempname2, someoneID);
			if (strstr(tempname1, tempname2)) {
				safewrite(fd2, &post, sizeof (post));
			}
			break;
		case 3:
			if (!strcasecmp(post.owner, someoneID)) {
				safewrite(fd2, &post, sizeof (post));
			}
			break;
		case 4:
			strtolower(tempname1, post.title);
			strtolower(tempname2, someoneID);
			if (strstr(tempname1, tempname2)) {
				safewrite(fd2, &post, sizeof (post));
			}
			break;
		}
	}
	close(fd);
	close(fd2);

	return 0;
}

void
add_edit_mark(char *fname, int mode, char *title)
{
	FILE *fp, *out;
	time_t now;
	char buf[LINELEN], outname[PATH_MAX + 1];
	int step = 0, signature = 0, anonymous = NA, color;


	if ((fp = fopen(fname, "r")) == NULL)
		return;

	/* monster: 修正转寄文章时不能正确添加标记和匿名编辑文章泄漏ID的错误 */
	switch (mode) {
	case 3:
		mode = 1;
		snprintf(outname, sizeof(outname), "mail/.tmp/editpost.%s.%d", currentuser.userid, uinfo.pid);
		break;
	case 4:
	case 5:
		mode -= 3;
		anonymous = YEA;
	default:
		if (INMAIL(uinfo.mode)) {
			snprintf(outname, sizeof(outname), "mail/.tmp/editpost.%s.%d", currentuser.userid, uinfo.pid);
		} else {
			snprintf(outname, sizeof(outname), "boards/.tmp/editpost.%s.%d", currentuser.userid, uinfo.pid);
		}
	}

	if ((out = fopen(outname, "w")) == NULL)
		return;

	color = (currentuser.numlogins % 7) + 31;
	while ((fgets(buf, sizeof(buf), fp)) != NULL) {
		if (!strncmp(buf, "--\n", 3))
			signature = 1;
		if (mode == 1) {
			if (buf[0] == 27 && buf[1] == '[' &&
			    buf[2] == '1' && buf[3] == ';' &&
			    !strncmp(buf + 6, "m※ 修改:．", 11))
				continue;
			if (Origin2(buf) && (step != 3)) {
				now = time(NULL);
				fprintf(out,
					"%s\033[1;%2dm※ 修改:．%s 于 %15.15s 修改本文．[FROM: %s]\033[m\n",
					(signature) ? "" : "--\n", color,
					(anonymous == YEA) ? currboard : currentuser.userid, ctime(&now) + 4,
					(anonymous == YEA) ? "匿名天使的家" : fromhost);
				step = 3;
			}
			fputs(buf, out);
		} else {
			if (!strncmp(buf, "标  题: ", 8)) {
				fprintf(out, "标  题: %s\n", title);
				mode = 1;
				continue;
			}
			fputs(buf, out);
		}
	}
	if ((step != 3) && (mode == 1)) {
		now = time(NULL);
		fprintf(out, "%s\033[1;%2dm※ 修改:．%s 于 %15.15s 修改本文．[FROM: %s]\033[m\n",
			(signature) ? "" : "--\n", color,
			(anonymous == YEA) ? currboard : currentuser.userid, ctime(&now) + 4,
			(anonymous == YEA) ? "匿名天使的家" : fromhost);
	}
	fclose(fp);
	fclose(out);
	f_mv(outname, fname);
}

/* monster: if the file is delivered as mail, set board to NULL */
void
add_sysheader(FILE *fp, char *board, char *title)
{
	time_t now;
	char buf[STRLEN];

	now = time(NULL);
	if (board != NULL) {
		snprintf(buf, sizeof(buf), "\033[1;41;33m发信人: %s (自动发信系统), 信区: %s", BoardName, board);
	} else {
		snprintf(buf, sizeof(buf), "\033[1;41;33m寄信人: %s (自动发信系统)", BoardName);
	}

	fprintf(fp, "%s%*s\033[m\n", buf, (int)(89 - strlen(buf)), " ");
	fprintf(fp, "标  题: %s\n", title);
	fprintf(fp, "发信站: %s (%24.24s)\n", BoardName, ctime(&now));

	if (board == NULL) {
		fprintf(fp, "来  源: %s\n\n", BBSHOST);
	} else {
		fputc('\n', fp);
	}
}

void
add_syssign(FILE * fp)
{
	fprintf(fp, "\n--\n\033[m\033[1;31m※ 来源:．%s %s．[FROM: %s]\033[m\n",
		BoardName, BBSHOST, BBSHOST);
}
