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
stand_title(char *title)
{
	clear();
	standout();
	outs(title);
	standend();
}

int
kick_user(struct user_info *userinfo)
{
	int id, ind;
	struct user_info uin;
	struct userec kuinfo;
	char kickuser[IDLEN + 2];

	if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE && uinfo.mode != FRIEND) {
		modify_user_mode(ADMIN);
		stand_title("踢使用者下站");

		move(2, 0);
		usercomplete("输入使用者帐号: ", kickuser);
		if (*kickuser == '\0') {
			clear();
			return 0;
		}

		if ((id = getuser(kickuser, NULL)) == 0) {
			move(4, 0);
			outs("无效的用户 ID");
			pressreturn();
			return 0;
		}

		move(3, 0);
		snprintf(genbuf, sizeof(genbuf), "踢掉使用者 : [%s].", kickuser);
		if (askyn(genbuf, NA, NA) == NA) {
			move(4, 0);
			outs("取消踢使用者.");
			pressreturn();
			return 0;
		}

		search_record(PASSFILE, &kuinfo, sizeof (kuinfo), cmpuids, kickuser);
		ind = search_ulist(&uin, t_cmpuids, id);
	} else {
		uin = *userinfo;
		strlcpy(kickuser, uin.userid, sizeof(kickuser));
		ind = YEA;
	}

	if (!ind || !uin.active || (uin.pid && kill(uin.pid, 0) == -1)) {
		if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE && uinfo.mode != FRIEND) {
			move(4, 0);
			prints("用户 [%s] 不在线上", kickuser);
			clrtoeol();
			pressreturn();
			clear();
		}
		return 0;
	}

	safe_kill(uin.pid);
	report("kicked %s", kickuser);
	snprintf(genbuf, sizeof(genbuf), "%s (%s)", kuinfo.userid, kuinfo.username);
	log_usies("KICK ", genbuf);

	if (uinfo.mode != LUSERS && uinfo.mode != OFFLINE && uinfo.mode != FRIEND) {
		if (strcmp(currentuser.userid, uin.userid) != 0) {
			snprintf(genbuf, sizeof(genbuf), "踢出使用者: %s", uin.userid);
			securityreport(genbuf);
		}
		move(5, 0);
		prints("用户 [%s] 已经被踢下站.", kickuser);
		pressreturn();
	}
	return 1;
}

extern char msg_buf[STRLEN];

void
regtitle(void)
{
	stand_title("设定注册单");
	move(1, 0);
	outs("返回[\033[1;32m←\033[m,\033[1;32mq\033[m] 查看[\033[1;32m→\033[m] 选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 通过[\033[1;32my\033[m] 删除[\033[1;32md\033[m] 不通过[\033[1;32m0～9\033[m] 清除[\033[1;32mc\033[m] 求助[\033[1;32mh\033[m]\n");
	outs("\033[1;37;44m 编号    使用者昵称    真实姓名             姓名     上站IP    注册IP        \033[m");
}

char *
regdoent(int num, void *ent_ptr)              // 注册单列表
{
	char cmark[10];
	static char buf[128];
	struct new_reg_rec *ent = (struct new_reg_rec *)ent_ptr;

	switch (ent->mark) {
	case ' ':
		cmark[0] = '\0';
		break;
	case 'Y':
		strlcpy(cmark, "\033[1;32m", sizeof(cmark));
		break;
	default:
		strlcpy(cmark, "\033[1;31m", sizeof(cmark));
	}

	snprintf(buf, sizeof(buf),
		" %4d  %s%c\033[m %-12.12s  %-20.20s  %s%5d\033[m%s%10d\033[m%s%10d\033[m",
		num, cmark, ent->mark, ent->userid, ent->rname, 
		(ent->Sname > 3) ? "\033[1;31m" : "", ent->Sname, 
		(ent->Slog > 3) ? "\033[1;31m" : "",  ent->Slog, 
		(ent->Sip > 3) ? "\033[1;31m" : "", ent->Sip) ; //Modifyed by betterman 06/07/27
	return buf;
}

int
show_reg(struct new_reg_rec *reg)
{
	int usernum;
	struct userec lookupuser;
	stand_title("查看注册者详细资料");
	if ((usernum = getuser(reg->userid, &lookupuser)) == 0) {
		outs("查无此人");
		pressreturn();
		return -1;
	}

	display_userinfo(&lookupuser);
	move(16, 0);
	prints("帐号位置     :%d\n", reg->usernum);
	prints("申请帐号     :%s\n", reg->userid);
	prints("真实姓名     :%s\n", reg->rname);
	prints("毕业年份     :%4d               学号         :%s\n", reg->graduate, reg->account);
	prints("学校专业     :%s\n", reg->dept);
	prints("目前住址     :%s\n", reg->addr);
	prints("连络电话     :%s\n", reg->phone);
	prints("出生年日     :%.4d-%.2d-%.2d\n", reg->birthyear + 1900, reg->birthmonth, reg->birthday); //Modifyed by betterman 06/07/27

	move(1, 0);
	outs("\033[1;36m目前标志：\033[m");
	if (reg->mark == ' ') {
		outs("无    ");
	} else if (reg->mark == 'Y') {
		outs("\033[1;32m通过  \033[m");
	} else {
		outs("\033[1;31m失败  \033[m");
	}

	prints("%s姓名：%4d\033[m    ", (reg->Sname > 3) ? "\033[1;31m" : "\033[1;32m", reg->Sname);
	prints("%s上站IP：%4d\033[m    ", (reg->Slog > 3) ? "\033[1;31m" : "\033[1;32m", reg->Slog);
	prints("%s注册IP：%4d\033[m    ", (reg->Sip > 3) ? "\033[1;31m" : "\033[1;32m", reg->Sip);
	return 0;
}

int
read_register(int ent, struct new_reg_rec *reg, char *direct)
{
	int ch;
	
	while (1) {
		/* monster: 删除非法记录 */
		if (show_reg(reg) == -1) {
			del_register_now(ent, reg, direct);
			return READ_NEXT;
		}

		ch = egetch();

		switch (ch) {
		case 'y':
		case 'd':
			ch = toupper(ch);
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			reg->mark = ch;
			update_reg(ent, reg);
			return READ_NEXT;
		case 'c':
		case 'C':
			reg->mark = ' ';
			update_reg(ent, reg);
			break;
		case '\n':
		case KEY_DOWN:
		case ' ':
		case KEY_RIGHT:
			return READ_NEXT;
		case KEY_UP:
			return READ_PREV;
		case KEY_LEFT:
			return DIRCHANGED;
		case 'D':
			del_register_now(ent, reg, direct);
			return READ_NEXT;
		case 'Y':
			press_register_now(ent, reg, direct);
			return READ_NEXT;
		}
	}
}

int
update_reg(int ent, struct new_reg_rec *reg)
{
	int i;
	struct new_reg_rec tmp;

	for (i = ent; i > 0; i--) {
		if (0 == get_record("new_register.rec", &tmp, sizeof (tmp), i)) {
			if (0 == strcmp(reg->userid, tmp.userid)) {
				ent = i;
				break;
			}
		}
	}

	if (i > 0) {
		substitute_record("new_register.rec", reg,
				  sizeof (struct new_reg_rec), ent);
	}
	return 0;
}

int
del_register_now(int ent, struct new_reg_rec *reg, char *direct)
{
	struct new_reg_rec tmp;
	int i;

	for (i = ent; i > 0; i--) {
		if (0 == get_record("new_register.rec", &tmp, sizeof (tmp), i)) {
			if (0 == strcmp(reg->userid, tmp.userid)) {
				ent = i;
				break;
			}
		}
	}
	if (i == 0)
		return DIRCHANGED;
	i = delete_record("new_register.rec", sizeof (struct new_reg_rec), ent);
	return DIRCHANGED;
}

int
del_register(int ent, struct new_reg_rec *reg, char *direct)
{
	reg->mark = 'D';
	update_reg(ent, reg);
	return DIRCHANGED;
}

int
press_register(int ent, struct new_reg_rec *reg, char *direct)
{
	reg->mark = 'Y';
	update_reg(ent, reg);
	return DIRCHANGED;
}

int
press_register_now(int ent, struct new_reg_rec *reg, char *direct)
{
	int usernum;
	char buf[STRLEN];
	FILE *authfile;
	struct userec lookupuser;
	int fore_user; /* 区分新旧用户 */
	struct denyheader dh;

	if ((usernum = getuser(reg->userid, &lookupuser)) == 0) {
		del_register_now(ent, reg, direct);
		return DIRCHANGED;
	}

	fore_user = (lookupuser.userlevel != PERM_BASIC);

	lookupuser.lastjustify = time(NULL);
	lookupuser.userlevel |= PERM_DEFAULT;
#ifdef AUTHHOST
	lookupuser.userlevel |= PERM_WELCOME;
#endif
   if (search_record("boards/.DENYLIST", &dh, sizeof(struct denyheader), 
							denynames, lookupuser.userid)) 
		lookupuser.userlevel &= ~PERM_POST;
	if (substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), usernum) == -1) {
      presskeyfor("系统错误，请联系系统维护员\n");
		del_register_now(ent, reg, direct);
		return DIRCHANGED;
	}

	sethomefile(buf, lookupuser.userid, "auth");
	if (dashf(buf)) {
		sethomefile(genbuf, lookupuser.userid, "auth.old");
		rename(buf, genbuf);
	}
	if ((authfile = fopen(buf, "w")) != NULL) {
		fprintf(authfile, "usernum: %d, %s", usernum,
			ctime(&(reg->regtime)));
		fprintf(authfile, "userid: %s\n", reg->userid);
		fprintf(authfile, "realname: %s\n", reg->rname);
		fprintf(authfile, "dept: %s\n", reg->dept);
		fprintf(authfile, "addr: %s\n", reg->addr);
		fprintf(authfile, "phone: %s\n", reg->phone);
		fprintf(authfile, "account: %s\n", reg->account);
		//fprintf(authfile, "assoc: %s\n", reg->assoc);	
		fprintf(authfile, "birthday: %d\n",reg->birthyear + 1900);			
		fprintf(authfile, "birthday: %d\n",reg->birthmonth);
		fprintf(authfile, "birthday: %d\n",reg->birthday);
		fprintf(authfile, "graduate: %d\n",reg->graduate);
		fprintf(authfile, "auth: %s\n",reg->auth + 1);
		time_t now = time(NULL);
		fprintf(authfile, "Date: %s", ctime(&now));
		fprintf(authfile, "Approved: %s", currentuser.userid);
		fclose(authfile);
	}

	if(fore_user){ /* 旧用户 */
		mail_sysfile("etc/Activa_fore_users", lookupuser.userid, "恭喜，今后您可在各地畅游Argo");
	}else{ /* 新用户 */
		mail_sysfile("etc/smail", lookupuser.userid, "欢迎加入本站行列");
	}


	sethomefile(buf, lookupuser.userid, "mailcheck");
	unlink(buf);
	snprintf(genbuf, sizeof(genbuf), "让 %s 通过身分确认.", lookupuser.userid);
	do_securityreport(genbuf, &lookupuser, YEA, NULL);  //Henry: 修正形参不一致
	del_register_now(ent, reg, direct);
	return DIRCHANGED;
}

int
clean_register(int ent, struct new_reg_rec *reg, char *direct)
{
	reg->mark = ' ';
	update_reg(ent, reg);
	return DIRCHANGED;
}

int
fail_register(int ent, struct new_reg_rec *reg, char *direct)
{
	static char *reason[] = {
 		"请填写真实姓名.", 		"请如实填写毕业年份.",
		"请如实填写专业.", 		"请如实填写详细出生日期.",
		"请填写完整的住址资料.", 		"请如实填写学号.",
		"请如实填写注册申请表.", 		"请用中文填写注册单.",
		"请用获取激活码方式注册", 		"其它具体原因.",
		NULL
	};
	int fail, usernum;
	struct userec lookupuser;
	char reason_buf[20];  // by rovingcloud
	char show_buf[20];  // by rovingcloud

	if ((usernum = getuser(reg->userid, &lookupuser)) == 0)
		return del_register_now(ent, reg, direct);
	if (reg->mark < '0' || reg->mark > '9')  // by rovingcloud, '8'->'9'
		return DIRCHANGED;
	// by rovingcloud - begin
	// 处理帐管菜单问题：将第9项改成可以填写具体原因
	if (reg->mark == '9') {
		clear();
		sprintf(show_buf, "请为%s输入具体原因：", reg->userid);
		getdata(1, 0, show_buf, reason_buf, 20, DOECHO, YEA);
		snprintf(lookupuser.address, sizeof(lookupuser.address),
			 "<注册失败> %s", reason_buf);
	} else {  	// by rovingcloud
		fail = reg->mark - '0';
		snprintf(lookupuser.address, sizeof(lookupuser.address),
		"<注册失败> %s", reason[fail]);
	}
	// by rovingcloud - end
	substitute_record(PASSFILE, &lookupuser, sizeof(lookupuser), usernum);
	mail_file("etc/f_fill", lookupuser.userid, lookupuser.address);
	del_register_now(ent, reg, direct);
	return DIRCHANGED;
}

int
enable_register(int ent, struct new_reg_rec *reg, char *direct)
{
	if (reg->mark == 'Y')
		return press_register_now(ent, reg, direct);
	if (reg->mark == 'D')
		return del_register_now(ent, reg, direct);
	if (reg->mark == ' ')
		return DIRCHANGED;
	return fail_register(ent, reg, direct);
}

int
enable_register_all(int ent, struct new_reg_rec *reg, char *direct)
{
	int num, i;
	struct new_reg_rec tmp;

	num = get_num_records(direct, sizeof (struct new_reg_rec));
	for (i = 1; i <= num; i++) {
		if (0 == get_record(direct, &tmp, sizeof (tmp), i)) {
			enable_register(i, &tmp, direct);
		}
	}
	return DIRCHANGED;
}

struct one_key reg_comms[] = {
	{ 'r',          read_register },
	{ 'd',          del_register },
	{ 'D',          del_register_now },
	{ 'y',          press_register },
	{ 'Y',          press_register_now },
	{ 'c',          clean_register },
	{ 'C',          clean_register },
	{ 'e',          enable_register },
	{ 'E',          enable_register_all },
	{ 'h',          registerhelp} ,
	{ '\0',         NULL }
};

//Added end

int
m_info(void)
{
	struct userec uinfo;
	int id;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;
	stand_title("修改使用者资料");
	if (!gettheuserid(2, "请输入使用者代号: ", &id, &uinfo))
		return -1;

	move(1, 0);
	clrtobot();
	display_userinfo(&uinfo);
	uinfo_query(&uinfo, 1, id);
	return 0;
}

int
bm_count(char *userid)
{
	int i, count = 0;

	resolve_boards();
	for (i = 0; i < numboards; i++)
		if (check_bm(userid, bcache[i].BM))
			++count;

	return count;
}

int
m_ordainBM(void)
{
	int id, pos, oldbm = 0, i, bm = 1;
	struct boardheader fh;
	char bmfilename[STRLEN], bname[STRLEN];
	char buf[5][STRLEN];
	struct userec lookupuser;

#ifdef MAIL_NEWBMGUIDE
	FILE *bmfp;
	char fname[PATH_MAX + 1];
#endif

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("任命版主\n");
	if (!gettheuserid(2, "输入欲任命的使用者帐号: ", &id, &lookupuser))
		return 0;
	if (!strcmp(lookupuser.userid, "guest")) {
		move(5, 0);
		outs("您不能任命 guest 当版主");
		pressanykey();
		return -1;
	}

	if (!gettheboardname(3, "输入该使用者将管理的讨论区名称: ", &pos, &fh, bname))
		return -1;

	if (fh.BM[0] != '\0') {
		if (!strncmp(fh.BM, "SYSOP", 5)) {
			move(4, 0);
			if (askyn("该讨论区版主是 SYSOP, 您确定该版需要版主吗", NA, NA) == NA)
				return -1;
			fh.BM[0] = '\0';
		} else {
			for (i = 0, oldbm = 1; fh.BM[i] != '\0'; i++)
				if (fh.BM[i] == ' ')
					oldbm++;
			if (oldbm == 3) {
				move(5, 0);
				prints("%s 讨论区已有三名版主", bname);
				pressreturn();
				return -1;
			}
			bm = 0;
		}
	}

	if (check_bm(lookupuser.userid, fh.BM)) {
		move(5, 0);
		prints(" %s 已经是该版的版主了", lookupuser.userid);
		pressanykey();
		return -1;
	}

	prints("\n您将任命 %s 为 %s 版版%s.\n", lookupuser.userid, bname, bm ? "主" : "副");
	if (askyn("您确定要任命吗?", NA, NA) == NA) {
		prints("取消任命版主");
		pressanykey();
		return -1;
	}

	for (i = 0; i < 5; i++)
		buf[i][0] = '\0';
	
	move(8, 0);
	prints("请输入任命附言(最多五行，按 Enter 结束)");
	for (i = 0; i < 5; i++) {
		getdata(i + 9, 0, ": ", buf[i], STRLEN - 5, DOECHO, YEA);
		if (buf[i][0] == '\0')
			break;
	}

	if (!(lookupuser.userlevel & PERM_BOARDS)) {
		lookupuser.userlevel |= PERM_BOARDS;
		substitute_record(PASSFILE, &lookupuser, sizeof(struct userec),  id);
		snprintf(genbuf, sizeof(genbuf), "版主任命, 给予 %s 的版主权限", lookupuser.userid);
		securityreport(genbuf);
		move(15, 0);
		outs(genbuf);
	}

	if (fh.BM[0] == '\0') {
		strlcpy(genbuf, lookupuser.userid, sizeof(genbuf));
	} else {
		snprintf(genbuf, sizeof(genbuf), "%s %s", fh.BM, lookupuser.userid);
	}

	strlcpy(fh.BM, genbuf, sizeof(fh.BM));
	substitute_record(BOARDS, &fh, sizeof(fh), pos);
	refresh_bcache();

	snprintf(bmfilename, sizeof(bmfilename), "任命 %s 为 %s 讨论区%s",
		lookupuser.userid, fh.filename, bm ? "版主" : "版副");
	securityreport(bmfilename);

	move(16, 0);
	outs(bmfilename);

#ifdef MAIL_NEWBMGUIDE
	snprintf(fname, sizeof(fname), "tmp/bmfile.%d", getpid());
	if ((bmfp = fopen(fname, "w")) != NULL) {
		if (bm) {
			fprintf(bmfp, "    现任命 %s 为 %s 版版主，希望他能履行在版主申请中作\n\n", lookupuser.userid, bname);
			fprintf(bmfp, "出的承诺繁荣该版，为广大站友服务。\n\n");
		} else {
			fprintf(bmfp, "    申请和资料都齐备了，在版内表现也不错，希望上任后能履行自己在\n\n");
			fprintf(bmfp, "申请书中所说的责任，协助版主，搞好 %s 版。\n\n", bname);
		}

		fprintf(bmfp, "    请在三天内到区管理中心报到。并把任命公告收进所申请版的精华区。\n\n");
		fprintf(bmfp, "    《%s版主管理条例》：\n\n", BBSNAME);
		fprintf(bmfp, "第04条  新任命版主有一个月试用期。一个月期满后须在所在区的管理中心版\n\n");
		fprintf(bmfp, "        提交一份转正申请，转正后方可成为正式版主；未能转正的版主作撤\n\n");
		fprintf(bmfp, "        职处理；逾期两周未提交转正申请的版主作撤职处理。\n\n");
		fprintf(bmfp, "        版主转正的具体方式参见《逸仙时空分区管理条例》。\n\n");
		add_syssign(bmfp);
		fclose(bmfp);
#ifdef ORDAINBM_POST_BOARDNAME
		postfile(fname, ORDAINBM_POST_BOARDNAME, bmfilename, 1);
#endif
		postfile(fname, bname, bmfilename, 1);
		mail_sysfile(fname, lookupuser.userid, bmfilename);
		unlink(fname);
	}
	mail_sysfile("etc/bm_1", lookupuser.userid, "☆新版主必读☆");
#endif
	refresh_bcache();
	pressanykey();
	return 0;
}

int
m_retireBM(void)
{
	int id, pos, right = 0, oldbm = 0, i, j, bmnum;
	int bm = 1;
	struct boardheader fh;
	FILE *bmfp;
	char bmfilename[STRLEN];
	char bname[STRLEN], usernames[3][STRLEN];
	char fname[STRLEN], buf1[STRLEN], buf2[STRLEN];
	struct userec lookupuser;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("版主离职\n");
	if (!gettheuserid(2, "输入欲离职的版主帐号: ", &id, &lookupuser))
		return -1;
	if (!gettheboardname(3, "请输入该版主要辞去的版名: ", &pos, &fh, bname))
		return -1;
	if (!check_bm(lookupuser.userid, fh.BM)) {
		move(5, 0);
		prints(" %s %s版版主，如有错误，请通知系统维护.", lookupuser.userid, (oldbm) ? "不是该" : "没有担任任何");
		pressanykey();
		return -1;
	}

	bmnum = 0;
	for (i = 0, j = 0; fh.BM[i] != '\0'; i++) {
		if (fh.BM[i] == ' ') {
			usernames[bmnum][j] = '\0';
			bmnum++;
			j = 0;
		} else {
			usernames[bmnum][j++] = fh.BM[i];
		}
	}

	usernames[bmnum++][j] = '\0';
	for (i = 0, right = 0; i < bmnum; i++) {
		if (!strcmp(usernames[i], lookupuser.userid)) {
			right = 1;
			if (i) bm = 0;
		}
		if (right && i != bmnum - 1)
			strcpy(usernames[i], usernames[i + 1]);
	}

	if (!right) {
		move(5, 0);
		prints("对不起， %s 版版主名单中没有 %s ，如有错误，请通知系统维护.", bname, lookupuser.userid);
		pressanykey();
		return -1;
	}

	prints("\n您将取消 %s 的 %s 版版%s职务.\n", lookupuser.userid, bname, bm ? "主" : "副");
	if (askyn("您确定要取消他的该版版主职务吗?", NA, NA) == NA) {
		prints("\n呵呵，您改变心意了？ %s 继续留任 %s 版版主职务！", lookupuser.userid, bname);
		pressanykey();
		return -1;
	}

	if (bmnum - 1) {
		snprintf(genbuf, sizeof(genbuf), "%s", usernames[0]);
		strlcpy(genbuf, usernames[0], sizeof(genbuf));
		for (i = 1; i < bmnum - 1; i++) {
			snprintf(genbuf, sizeof(genbuf), "%s %s", genbuf, usernames[i]);
		}
	} else {
		genbuf[0] = '\0';
	}

	strlcpy(fh.BM, genbuf, sizeof(fh.BM));
	substitute_record(BOARDS, &fh, sizeof(fh), pos);
	refresh_bcache();

	snprintf(genbuf, sizeof(genbuf), "取消 %s 的 %s 讨论区版主职务", lookupuser.userid, fh.filename);
	securityreport(genbuf);

	move(8, 0);
	outs(genbuf);

	if (!bm_count(lookupuser.userid)) {
		if (!(lookupuser.userlevel & PERM_OBOARDS) && !(lookupuser.userlevel & PERM_SYSOP)) {
			lookupuser.userlevel &= ~PERM_BOARDS;
			substitute_record(PASSFILE, &lookupuser, sizeof(struct userec), id);
			snprintf(genbuf, sizeof(genbuf), "版主卸职, 取消 %s 的版主权限", lookupuser.userid);
			securityreport(genbuf);
			move(9, 0);
			outs(genbuf);
		}
	}

	outs("\n\n");
	if (askyn("需要在相关版面发送通告吗?", YEA, NA) == NA) {
		pressanykey();
		return 0;
	}
	outc('\n');
	if (askyn("正常离任请按 Enter 键确认，撤职惩罚按 N 键", YEA, NA) == YEA) {
		right = 1;
		snprintf(bmfilename, sizeof(bmfilename), "%s 版%s %s 离任通告", bname,
			bm ? "版主" : "版副", lookupuser.userid);
	} else {
		right = 0;
		snprintf(bmfilename, sizeof(bmfilename), "[通告]撤除 %s 版%s %s ", bname,
			bm ? "版主" : "版副", lookupuser.userid);
	}
	strlcpy(currboard, bname, sizeof(currboard));

	//rewrite by cancel
	snprintf(buf2, sizeof(buf2), "\033[%s", (right) ? "1;32;46m" : "m");
	snprintf(fname, sizeof(fname), "bmfile.%d", getpid());
	if ((bmfp = fopen(fname, "w+")) == NULL) {
		outs("无法生成公告");
		pressreturn();
		return 0;
	}

	fprintf(bmfp, "\n    \033[1;33m%s╔════════════════════════════╗\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s║                                                        ║\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s║☆                       通告                         ☆║\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s║                                                        ║\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s║", buf2);

	if (right) {
		fprintf(bmfp, "☆ 同意 \033[1;33m%-12s\033[1;32m 辞去 "
			"\033[1;33m%-16s\033[1;32m 版版%s之职 ☆",
			lookupuser.userid, bname, (bm) ? "主" : "副");
	} else {
		fprintf(bmfp, "☆ 现撤去 %-16s 版的 %-12s 版%s之职 ☆",
			bname, lookupuser.userid, (bm) ? "主" : "副");
	}
	fprintf(bmfp, "║\033[m");

	fprintf(bmfp, "\n   \033[1;33m %s║                                                        ║\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s║☆            感谢该网友在职期间的辛勤劳动            ☆║\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s║                                                        ║\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s║☆                 并祝以后生活愉快                   ☆║\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s║                                                        ║\033[m", buf2);

	prints("请输入%s附言(最多五行，按 Enter 结束)", right ? "版主离任" : "版主撤职");

	for (i = 0; i < 5; i++) {
		getdata(i + 15, 0, ": ", buf1, 50, DOECHO, YEA);
		if (buf1[0] == '\0')
			break;
		if (i == 0)
			fprintf(bmfp, "\n    \033[1;33m%s║%s                                              ║\033[m",
				buf2, right ? "离任附言：" : "撤职说明：");
		fprintf(bmfp, "\n    \033[1;33m%s║    %s", buf2, buf1);
		for (j = 52 - strlen(buf1); j > 0; j--)
			fprintf(bmfp, " ");
		fprintf(bmfp, "║\033[m");
	}
	fprintf(bmfp, "\n    \033[1;33m%s║                                                        ║\033[m", buf2);
	fprintf(bmfp, "\n    \033[1;33m%s╚════════════════════════════╝\033[m", buf2);

	add_syssign(bmfp);
	fclose(bmfp);
//      autoreport(bmfilename, genbuf, YEA, NULL, NULL);
	postfile(fname, bname, bmfilename, 1);
	postfile(fname, "BoardManager", bmfilename, 1);
	mail_sysfile(fname, lookupuser.userid, bmfilename);
	unlink(fname);
	prints("\n执行完毕！");
	pressanykey();
	return 0;
}

//Added by betterman
int
count_same_auth(char *username, char type, int myecho)
{
	FILE *fp;
	int count = 0, x, y;
	char buf[256], buf2[STRLEN];
	char *ptr, *ptr2;
	int year = 1995;

	count = 0;
	x = 0;
	y = 5;
	while(1){
		sprintf(genbuf, "auth/%d/%d", year, year);
		year++;
		fp = fopen(genbuf, "r");
		if (!fp) {
			break;
		}

		while(fgets(buf, 256, fp) != NULL)
		{
			ptr = ptr2 = buf;
			if(*ptr == '\0' || *ptr == '\n' || *ptr == '\r' || *ptr == '#' )
				continue;
			switch (type) {
				case '1':
					if((ptr = strchr(ptr,';')) == NULL || ptr - ptr2 - 1 > STRLEN){
						buf2[0] = '\0';
						break;
					}
					strlcpy(buf2, ptr2, ptr - ptr2 +1);
					break;
				case '2':
					if((ptr = strchr(ptr,';')) == NULL || 
						(ptr2 = strchr(ptr+1,';')) == NULL ||
						(ptr = strchr(ptr2+1,';')) == NULL ||	
						ptr - ptr2 - 1 > STRLEN ){
						buf2[0] = '\0';
						break;
					}
					strlcpy(buf2, ptr2+1, ptr - ptr2 );
					break;
			}

			if (!strcmp(buf2, username)) {
				count++;
				if (y >= t_lines - 2) {
					x += 16;
					y = 5;
				}
				if (myecho) {
					move(y++, x);
					outs(buf);
				}
			}
		}
		if(fp){
			fclose(fp);
		}
	}
	return count;
}

int
my_searchauth(void)
{
	char username[STRLEN], ans[3];
	int count;
	const char titles[][20] = {
		"查找真实姓名: ", "查找指定学号: "
	};

	ans[0] = '\0';
	while (1) {
		username[0] = '\0';
		stand_title("在数据库中查询相同资料用户");
		getdata(2, 0, "(0)退出  (1)查找真实姓名  (2)查找指定学号 [0]: ",
			ans, 2, DOECHO, NA);
		if (ans[0] < '1' || ans[0] > '2')
			return 0;
		getdata(3, 0, (char *)titles[ans[0] - '1'], username, NICKNAMELEN, DOECHO, NA);
		if (username[0] == '\0')
			return 0;
		if ((count = count_same_auth(username, ans[0], YEA)) >= 0) {
			move(4, 0);
			prints("共查找到 %d 个匹配资料", count);
		}
		if (askyn("是否继续查询", YEA, YEA) == NA)
			return 0;
	}
}

int
my_search_same_auth(void)
{
	char ans[3];
	int count , x, y, echo;	
	int usernum, mail_auth;
	struct userec lookupuser, trec;
	int fd;

	const char titles[][30] = {
		"欲查找的使用者帐号: "
	};

	while (1) {
		count = 0;
		x = 0;
		y = 5;
		echo = 0;
		ans[0] = '\0';
		stand_title("查找相同激活资料的账号");
		getdata(2, 0, "(0)退出  (1)查找相同激活资料的账号  [0]: ",
			ans, 2, DOECHO, NA);
		if (ans[0] < '1' || ans[0] > '1')
			return 0;
		if (!gettheuserid(3, (char *)titles[ans[0] - '1'],  &usernum, &lookupuser))
			return 0;
	
		if(!(lookupuser.userlevel & PERM_WELCOME)){
			presskeyfor("该帐号还没激活");
			return 0;
		}

		mail_auth = (lookupuser.reginfo[0] != '\0');

		if ((fd = open(PASSFILE, O_RDONLY, 0)) == -1){
			presskeyfor("系统错误，请联络系统管理员");
			return -1;
		}

		while(read(fd,&trec,sizeof(trec)) == sizeof(trec))
		{
			if(trec.userid[0] == '\0')
				continue;
			if((trec.userlevel & PERM_WELCOME) == 0)
				continue;
			if(mail_auth){
				echo = !strcmp(trec.reginfo,lookupuser.reginfo);
			}else{
				echo = !memcmp(trec.reginfo, lookupuser.reginfo, MD5_PASSLEN);
			}
			
			if(echo){
				count++;
				if (y >= t_lines - 2) {
					x += 16;
					y = 5;
				}
				move(y++, x);
				outs(trec.userid);				
			}
			
		}
	
		if (count >= 0) {
			move(4, 0);
			prints("共查找到 %d 个匹配资料", count);
		}
		if (askyn("是否继续查询", YEA, YEA) == NA)
			return 0;
	}
}


int
m_register(void)
{
	int fd = 0;
	char ans[3], cmd[3];

	if (!HAS_PERM(PERM_ACCOUNTS | PERM_SYSOP))
		return 0;
	
	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("设定使用者注册资料");
	move(2,0);
	outs("(0)离开  (1)审查新注册资料  (2)查询相同资料用户  (3)查询使用者注册资料 \n");
	getdata(3, 0, "(4)查询资料数据库 (5)查询相同激活资料用户  [0]: ",
		ans, 2, DOECHO, YEA);

	if (ans[0] == '1') {
		cmd[0] = '\0';
		if ((fd = filelock("register.lock", NA)) == -1) {
			getdata(1, 0, "可能有另外的帐号管理员在处理注册单，是否继续？", cmd, 3, DOECHO, YEA);
			if (cmd[0] == 'N' || cmd[0] == 'n')
				return 0;
		}
		if (get_num_records("new_register.rec", sizeof (struct new_reg_rec)) == 0) {
			if (fd > 0) close(fd);
			outs("现在没有注册单");
			pressreturn();
			return 0;
		}
		i_read(ADMIN, "new_register.rec", NULL, NULL, regtitle, regdoent, update_endline, &reg_comms[0],
		       get_records, get_num_records, sizeof(struct new_reg_rec));
		if (fd > 0) close(fd);
	}
	if (ans[0] == '2')
		my_searchreg();

	if (ans[0] == '3')
		my_queryreg();
	if(ans[0] == '4')
		my_searchauth();
	if(ans[0] == '5')
		my_search_same_auth();

	clear();
	return 0;
}

int
d_board(void)
{
	struct boardheader binfo;
	char bname[STRLEN];
	int id;

	if (!HAS_PERM(PERM_BLEVELS))
		return 0;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("删除讨论区");
	refresh_bcache();
	make_blist();
	move(2, 0);
	namecomplete("请输入讨论区: ", bname);
	if (bname[0] == '\0')
		return 0;

	id = getbnum(bname);
	if (get_record(BOARDS, &binfo, sizeof(binfo), id) == -1 || strcmp(binfo.filename, bname)) {
		move(3, 0);
		outs("不正确的讨论区\n");
		pressreturn();
		return 0;
	}

	if (binfo.BM[0] != '\0' && strcmp(binfo.BM, "SYSOP")) {
		move(6, 0);
		outs("该版还有版主，在删除本版前，请先取消版主的任命。");
		pressanykey();
		return 0;
	}

	if (askyn("您确定要删除这个讨论区", NA, NA) != YEA) {
		move(3, 0);
		prints("取消删除行动");
		pressreturn();
		return 0;
	}

	if (askyn("移除精华区", NA, NA) == YEA) {
		snprintf(genbuf, sizeof(genbuf), "0Announce/boards/%s", binfo.filename);
		f_rm(genbuf);
	}

	snprintf(genbuf, sizeof(genbuf), "vote/%s", binfo.filename);
	f_rm(genbuf);

	snprintf(genbuf, sizeof(genbuf), "boards/%s", binfo.filename);
	f_rm(genbuf);

	snprintf(genbuf, sizeof(genbuf), "删除讨论区: %s", binfo.filename);
	securityreport(genbuf);

	snprintf(genbuf, sizeof(genbuf), " << '%s' 被 %s 删除 >>", binfo.filename, currentuser.userid);
	memset(&binfo, 0, sizeof (binfo));
	strlcpy(binfo.title, genbuf, sizeof(binfo.title));
	binfo.level = PERM_SYSOP;
	substitute_record(BOARDS, &binfo, sizeof(binfo), id);
	refresh_bcache();

	move(5, 0);
	prints("\n本讨论区已经删除...");
	pressreturn();
	numboards = -1;
	return 0;
}

int
d_user(char *cid)
{
	int id, num;
	char secu[STRLEN];
	struct user_info uin;
	FILE *fp;     //Added by cancel at 02.04.25
	struct userec lookupuser;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("删除使用者帐号");
	if (!gettheuserid(2, "请输入欲删除的使用者代号: ", &id, &lookupuser))
		return 0;

	if (t_search_ulist(&uin, t_cmpuids, id, NA, NA) > 0) {
		outs("\n对不起，您不可以删除在线账号!");
		pressreturn();
		return 0;
	}

	if (!strcmp(lookupuser.userid, "SYSOP")) {
		outs("\n对不起，您不可以删除 SYSOP 帐号!");
		pressreturn();
		return 0;
	}

	prints("\n\n以下是 [%s] 的部分资料:\n", lookupuser.userid);
	prints("    User ID:  [%s]\n", lookupuser.userid);
	prints("    昵   称:  [%s]\n", lookupuser.username);
	prints("    姓   名:  [%s]\n", lookupuser.realname);

	strlcpy(secu, "bTCPRD#@XWBA#VS-DOM-F0s2345678", sizeof(secu));
	for (num = 0; num < 30; num++) {
		if (!(lookupuser.userlevel & (1 << num)))
			secu[num] = '-';
	}
	prints("    权   限: %s\n\n", secu);

	if (lookupuser.userlevel & PERM_BOARDS) {
		prints("[%s] 目前尚担任版主职务(或具有版主权限)\n\n",
		       lookupuser.userid);
	}

	snprintf(genbuf, sizeof(genbuf), "您确认要删除 [%s] 这个 ID 吗", lookupuser.userid);
	if (askyn(genbuf, NA, NA) == NA) {
		prints("\n取消删除使用者...\n");
		pressreturn();
		clear();
		return 0;
	}

	//Added by cancel at 02.04.25
	//删除id时询问是不设为不可注册
	if (askyn("是否设为不可注册", NA, NA) == YEA) {
		if ((fp = fopen("etc/bad_id", "a+")) != NULL) {
			fprintf(fp, "%s\n", lookupuser.userid);
			fclose(fp);
		}
	}
	move(14, 0);
	snprintf(secu, sizeof(secu), "删除使用者：%s", lookupuser.userid);
	do_securityreport(secu, &lookupuser, YEA, NULL);
	clear_userdir(lookupuser.userid);
	lookupuser.userlevel = 0;
	strcpy(lookupuser.address, "");
	strcpy(lookupuser.username, "");
	strcpy(lookupuser.realname, "");
	strcpy(lookupuser.termtype, "");
	prints("\n%s 已经被灭绝了...\n", lookupuser.userid);
	lookupuser.userid[0] = '\0';
	substitute_record(PASSFILE, &lookupuser, sizeof (lookupuser), id);
	setuserid(id, lookupuser.userid);
	pressreturn();
	return 1;
}

int
x_level(void)
{
	int id;
	unsigned int newlevel;
	unsigned int oldlevel;  /* Added by cancel at 01/09/13 */
	struct userec lookupuser;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("更改使用者权限\n");
	move(2, 0);
	usercomplete("输入欲更改的使用者帐号: ", genbuf);
	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}
	if ((id = getuser(genbuf, &lookupuser)) == 0) {
		move(3, 0);
		prints("该账号不存在");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	if (!strcmp("SYSOP", genbuf) /* || !strcmp("guest", genbuf) */ ) {
		move(3, 0);
		prints("您不能修改 %s 的权限\n", genbuf);
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}
	move(1, 0);
	clrtobot();
	move(2, 0);
	prints("设定使用者 '%s' 的权限 \n", genbuf);

	newlevel = setperms(lookupuser.userlevel, "权限", NUMPERMS, showperminfo);
	if (!HAS_PERM(PERM_SYSOP)) {
		/* monster: 如自身没有SYSOP权限，则不能赋予他人SYSOP权限 */
		newlevel &= ~PERM_SYSOP;
	}

	move(2, 0);
	if (newlevel == lookupuser.userlevel)
		prints("使用者 '%s' 权限没有变更\n", lookupuser.userid);
	else {
		oldlevel = lookupuser.userlevel;
		lookupuser.userlevel = newlevel;
		sec_report_level(lookupuser.userid, oldlevel, newlevel);
		substitute_record(PASSFILE, &lookupuser, sizeof (struct userec), id);
		if (!(lookupuser.userlevel & PERM_LOGINOK)) {
			char src[PATH_MAX + 1], dst[PATH_MAX + 1];

			sethomefile(dst, lookupuser.userid, "register.old");
			sethomefile(src, lookupuser.userid, "register");
			rename(src, dst);
		}
		prints("使用者 '%s' 权限已经更改完毕.\n", lookupuser.userid);
	}
	pressreturn();
	clear();
	return 0;
}

int
x_denylevel(void)
{
	int id, oldlevel;
	char ans[3];
	struct userec lookupuser;

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("更改使用者基本权限");
	move(2, 0);
	usercomplete("输入欲更改的使用者帐号: ", genbuf);

	if (genbuf[0] == '\0') {
		clear();
		return 0;
	}

	if ((id = getuser(genbuf, &lookupuser)) == 0) {
		move(3, 0);
		prints("Invalid User Id");
		clrtoeol();
		pressreturn();
		clear();
		return 0;
	}

	move(1, 0);
	clrtobot();
	move(2, 0);
	prints("设定使用者 '%s' 的基本权限 \n\n", genbuf);
	prints("(1) 封禁发表文章权力            (A) 恢复发表文章权力\n");
	prints("(2) 取消基本上站权力            (B) 恢复基本上站权力\n");
	prints("(3) 禁止进入聊天室              (C) 恢复进入聊天室权力\n");
	prints("(4) 禁止呼叫他人聊天            (D) 恢复呼叫他人聊天权力\n");
	prints("(5) 禁止给他人发消息            (E) 恢复给他人发消息权力\n");
	prints("(6) 禁止给他人发信件            (F) 恢复给他人发信件权力\n");

	oldlevel = lookupuser.userlevel;
	getdata(13, 0, "请输入您的处理: ", ans, 2, DOECHO, YEA);
	switch (ans[0]) {
	case '1':
		lookupuser.userlevel &= ~PERM_POST;
		break;
	case 'a':
	case 'A':
		lookupuser.userlevel |= PERM_POST;
		break;
	case '2':
		lookupuser.userlevel &= ~PERM_BASIC;
		break;
	case 'b':
	case 'B':
		lookupuser.userlevel |= PERM_BASIC;
		break;
	case '3':
		lookupuser.userlevel &= ~PERM_CHAT;
		break;
	case 'c':
	case 'C':
		lookupuser.userlevel |= PERM_CHAT;
		break;
	case '4':
		lookupuser.userlevel &= ~PERM_PAGE;
		break;
	case 'd':
	case 'D':
		lookupuser.userlevel |= PERM_PAGE;
		break;
	case '5':
		lookupuser.userlevel &= ~PERM_MESSAGE;
		break;
	case 'e':
	case 'E':
		lookupuser.userlevel |= PERM_MESSAGE;
		break;
	case '6':
		lookupuser.userlevel &= ~PERM_SENDMAIL;
		break;
	case 'f':
	case 'F':
		lookupuser.userlevel |= PERM_SENDMAIL;
		break;
	default:
		break;
	}

	if (oldlevel == lookupuser.userlevel) {
		prints("使用者 '%s' 权限没有变更\n", lookupuser.userid);
	} else {
		sec_report_level(lookupuser.userid, oldlevel, lookupuser.userlevel);
		substitute_record(PASSFILE, &lookupuser, sizeof (struct userec), id);
		prints("\n使用者 '%s' 基本权限已经更改完毕.\n", lookupuser.userid);
	}

	pressreturn();
	clear();
	return 0;
}

int
a_edits(void)
{
	int aborted;
	char ans[4], buf[STRLEN], buf2[STRLEN], secu[STRLEN];
	int ch, num, confirm;

	static char *e_file[] =
	{
		"Welcome", "Welcome2", "issue", "logout", "../vote/notes",
		"menu.ini", "endline_msg", "bad_id", "bad_email", "bad_host",
		"autopost", "sysops",
		"NOLOGIN", "NOREGISTER", "blockmail", "s_fill", "smail", "f_fill",
		"register", "firstlogin", "bbsnet", // "bbsnet_iplimit",
		"ipcount",
		#ifdef FILTER
		"filter_words",
		#endif
		"sysgoodbrd",
		"Activation",
		"Activa_fore_users",
		"top10keyword",
		"bm_1",
		NULL
	};

	static char *explain_file[] =
	{
		"特殊进站公布栏", "进站画面", "进站欢迎档", "离站画面",
		"公用备忘录", "系统菜单 (menu.ini)", "底部流动信息",
		"不可注册的 ID", "不可确认之E-Mail", "不可上站之地址",
		"每日自动送信档", "管理者名单",
		"暂停登陆(NOLOGIN)", "暂停注册(NOREGISTER)", "转信黑名单",
		"注册成功信件", "激活成功信件", "注册失败信件", "新用户注册范例",
		"用户第一次登陆公告",
		"BBSNET 转站清单", // "BBSNET IP 限制",
		"特殊IP连接数",
		#ifdef FILTER
		"过滤关键词",
		#endif
		"系统推荐版面",
		"帐号激活说明",
		"旧用户激活信件",
		"十大关键词",
		"新版主必读",
		NULL
	};

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	stand_title("编修系统档案");
	move(2, 0);

	/* monster: 避免美工编辑menu.ini时断线 */
	if (HAS_PERM(PERM_SYSOP)) {
		for (num = 0; e_file[num] != NULL && explain_file[num] != NULL;
		     num++) {
			prints("[\033[1;32m%2d\033[m] %s", num + 1,
			       explain_file[num]);
			if (num < 17)
				move(3 + num, 0);
			else
				move(num - 15, 50);
		}
	} else {
		for (num = 0; num < 7; num++) {
			prints("[\033[1;32m%2d\033[m] %s", num + 1,
			       explain_file[num]);
			move(3 + num, 0);
		}
	}

	prints("[\033[1;32m%2d\033[m] 都不想改\n", num + 1);

	getdata(t_lines - 1, 0, "您要编修哪一项系统档案: ", ans, 3, DOECHO,
		YEA);
	ch = atoi(ans);
	if (!isdigit(ans[0]) || ch <= 0 || ch > num || ans[0] == '\n' ||
	    ans[0] == '\0')
		return -1;
	ch -= 1;

	if (!strcmp(e_file[ch], "bbsnet")) {
		move(2, 0);
		clrtobot();
		getdata(2, 0, "您要编修哪一页BBSNET转站清单 (1~9) [1]:", ans, 2,
			DOECHO, YEA);
		if (ans[0] == '\0' || ans[0] == '1') {
			strlcpy(buf2, "etc/bbsnet.ini", sizeof(buf2));
		} else if (ans[0] >= '2' && ans[0] <= '9') {
			snprintf(buf2, sizeof(buf2), "etc/bbsnet%c.ini", ans[0]);
		} else {
			clear();
			return -1;
		}
	} else {
		snprintf(buf2, sizeof(buf2), "etc/%s", e_file[ch]);
	}

	move(2, 0);
	clrtobot();
	snprintf(buf, sizeof(buf), "(E)编辑 (D)删除 %s? [E]: ", explain_file[ch]);
	getdata(2, 0, buf, ans, 2, DOECHO, YEA);
	if (ans[0] == 'D' || ans[0] == 'd') {
		snprintf(buf, sizeof(buf), "您确定要删除 %s 这个系统档", explain_file[ch]);
		confirm = askyn(buf, NA, NA);
		if (confirm != 1) {
			move(5, 0);
			prints("取消删除行动\n");
			pressreturn();
			clear();
			return 0;
		}

		snprintf(secu, sizeof(secu), "删除系统档案：%s", explain_file[ch]);
		securityreport(secu);

		unlink(buf2);
		move(5, 0);
		prints("%s 已删除\n", explain_file[ch]);
		pressreturn();
		return 0;
	}

	modify_user_mode(EDITSFILE);
	aborted = vedit(buf2, EDIT_MODIFYHEADER); /* 不添加文件头, 允许修改头部信息 */
	clear();
	if (aborted != -1) {
		prints("%s 更新过", explain_file[ch]);
		snprintf(secu, sizeof(secu), "修改系统档案：%s", explain_file[ch]);
		securityreport(secu);

		if (!strcmp(e_file[ch], "Welcome")) {
			unlink("reclog/Welcome.rec");
			prints("\nWelcome 记录档更新");
		}

		#ifdef FILTER
		if (!strcmp(e_file[ch], "filter_words")) {
			init_filter();
		}
		#endif
	}
	pressreturn();
	return 0;
}

int
wall(void)
{
	char repbuf[100];

	if (!HAS_PERM(PERM_SYSOP))
		return 0;
	modify_user_mode(MSG);
	move(2, 0);
	clrtobot();
	if (!get_msg("所有使用者", msg_buf, 1)) {
		return 0;
	}
	snprintf(repbuf, sizeof(repbuf), "内容：%s\n", msg_buf);
	securityreport2("站长广播记录", NA, repbuf);
	if (apply_ulist(dowall) == -1) {
		move(2, 0);
		prints("线上空无一人\n");
		pressanykey();
	}
	outs("\n已经广播完毕...");
	pressanykey();
	return 1;
}

int
setsystempasswd(void)
{
	FILE *pass;
	char passbuf[PASSLEN + 1], prepass[PASSLEN + 1];
	unsigned char genpass[MD5_PASSLEN];

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return 0;

	if (strcmp(currentuser.userid, "SYSOP")) {
		clear();
		move(10, 20);
		prints("对不起，系统密码只能由 SYSOP 修改！");
		pressanykey();
		return 0;
	}

	getdata(2, 0, "请输入新的系统密码(直接回车则取消系统密码): ",
		passbuf, PASSLEN, NOECHO, YEA);
	if (passbuf[0] == '\0') {
		if (askyn("您确定要取消系统密码吗?", NA, NA) == YEA) {
			unlink("etc/.syspasswd");
			securityreport("\033[32m取消系统密码\033[37m");
		}
		return 0;
	}

	getdata(3, 0, "确认新的系统密码: ", prepass, 19, NOECHO, YEA);
	if (strcmp(passbuf, prepass)) {
		move(4, 0);
		prints("两次密码不相同, 取消此次设定.");
		pressanykey();
		return 0;
	}

	if ((pass = fopen("etc/.syspasswd", "w")) == NULL) {
		move(4, 0);
		prints("系统密码无法设定....");
		pressanykey();
		return 0;
	}

	fwrite(genpass, MD5_PASSLEN, 1, pass);
	fclose(pass);
	move(4, 0);
	prints("系统密码设定完成....");
	pressanykey();
	return 0;
}

int
my_queryreg(void)
{
	FILE *fn;
	char uident[IDLEN + 2];
	struct userec lookupuser;
	int have_auth;

	clear();
	move(2, 0);
	usercomplete("请输入要查询的代号: ", uident);
	if (uident[0] != '\0') {
		if (getuser(uident, &lookupuser) == 0) {
			move(2, 0);
			outs("错误的使用者代号...");
		} else {
			have_auth = (lookupuser.userlevel & PERM_WELCOME);
			outs(have_auth?"已激活\n":"尚未激活\n");
			snprintf(genbuf, sizeof(genbuf), "home/%c/%s/register", mytoupper(lookupuser.userid[0]), lookupuser.userid);
			if ((fn = fopen(genbuf, "r")) != NULL) {
				outs("\n注册资料如下:\n\n");
			
				while (fgets(genbuf, STRLEN, fn))
					outs(genbuf);
			
				fclose(fn);     /* add fclose by quickmouse 01/03/09 */
			} else {
				outs("\n\n找不到他/她的注册资料!!\n");
			}
			if(have_auth){
				pressanykey();
				clear();
				move(6,0);
				snprintf(genbuf, sizeof(genbuf), "home/%c/%s/auth", mytoupper(lookupuser.userid[0]), lookupuser.userid);
				if ((fn = fopen(genbuf, "r")) != NULL) {
					outs("\n激活资料如下:\n\n");
			
					while (fgets(genbuf, STRLEN, fn))
						outs(genbuf);
			
					fclose(fn);    
				} else {
					outs("\n\n找不到他/她的激活资料!!可能是使用邮箱激活的\n");
					prints("他/她的邮箱是： %s\n", lookupuser.email);
				}
			}
		}
	}
	pressanykey();
	return 0;
}

//Added by cancel
int
count_same_reg(char *username, char type, int myecho)
{
	FILE *fp;
	int count = 0, x, y;
	struct userec rec;
	char buf[STRLEN];

	fp = fopen(PASSFILE, "r");
	if (!fp) {
		move(4, 0);
		prints("打开文件出错");
		pressreturn();
		return -1;
	}
	count = 0;
	x = 0;
	y = 5;
	while (fread(&rec, sizeof (rec), 1, fp) > 0) {
		switch (type) {
		case '1':
			strcpy(buf, rec.realname);
			break;
		case '2':
			strcpy(buf, rec.ident);
			break;
		case '3':
			strcpy(buf, rec.lasthost);
			break;
		}
		if (!strcmp(buf, username)) {
			count++;
			if (y >= t_lines - 2) {
				x += 16;
				y = 5;
			}
			if (myecho) {
				move(y++, x);
				outs(rec.userid);
			}
		}
	}
	fclose(fp);
	return count;
}

int
my_searchreg(void)
{
	char username[STRLEN], ans[3];
	int count;
	const char titles[][20] = {
		"查找真实姓名: ", "查找注册位址: ", "查找最近上站地址: "
	};

	ans[0] = '\0';
	while (1) {
		username[0] = '\0';
		stand_title("查询相同资料用户");
		getdata(2, 0, "(0)退出  (1)真实姓名  (2)帐号注册位址 (3)最近上站地址 [0]: ",
			ans, 2, DOECHO, NA);
		if (ans[0] < '1' || ans[0] > '3')
			return 0;
		getdata(3, 0, (char *)titles[ans[0] - '1'], username, NICKNAMELEN, DOECHO, NA);
		if (username[0] == '\0')
			return 0;
		if ((count = count_same_reg(username, ans[0], YEA)) >= 0) {
			move(4, 0);
			prints("共查找到 %d 个匹配账号", count);
		}
		if (askyn("是否继续查询", YEA, YEA) == NA)
			return 0;
	}
}

/* monster: 重新编写的创建/修改讨论区代码 */

#define LASTITEM        13
#define	MAXGROUPNUM	36	// 0 - 9, 'a' - 'z'

struct GroupInfo {
	char name[256];
	char *chs;
} GroupsInfo[MAXGROUPNUM];

static int maxlen;

int
valid_brdname(char *brd)
{
	char ch;

	if (strlen(brd) < 2)
		return 0;

	ch = *brd++;
	if (!isalnum((unsigned int)ch) && ch != '_')
		return 0;
	while ((ch = *brd++) != '\0') {
		if (!isalnum((unsigned int)ch) && ch != '_')
			return 0;
	}
	return 1;
}

inline int
getgroupnum(char ch)
{
	if (ch >= '0' && ch <= '9')
		return ch - '0';

	if (ch >= 'A' && ch <= 'Z')
		return ch - 'A' + 10;

	return -1;
}

int
getgroupset(void)
{
	FILE *fp;
	char buf[256], buf2[8], *ptr, *ptr2;
	int gid, count = 0;

	maxlen = 0;
	memset(&GroupsInfo, 0, sizeof(GroupsInfo));
	if ((fp = fopen("etc/menu.ini", "r")) == NULL)
		return -1;

	strcpy(buf2, "EGROUP*");
	while (fgets(buf, sizeof(buf), fp)) {
		if (strncmp(buf, "@EGroups", 8))
			continue;

		if ((ptr = strchr(buf, '\"')) == NULL)
			continue;

		++ptr;
		buf2[6] = *ptr;

		if ((gid = getgroupnum(buf2[6])) == -1)
			continue;

		if (GroupsInfo[gid].name[0] != '\0')
			continue;

		GroupsInfo[gid].chs = sysconf_str(buf2);
		if ((ptr = strchr(ptr, '\"')) == NULL)
			continue;
		if ((ptr = strchr(ptr + 1, '\"')) == NULL)
			continue;
		if ((ptr2 = strchr(ptr + 1, '\"')) == NULL)
			continue;
		strlcpy(GroupsInfo[gid].name, ptr + 1, ptr2 - ptr);
		my_ansi_filter(GroupsInfo[gid].name);
		if (GroupsInfo[gid].name[0] != '\0') ++count;
		if (strlen(GroupsInfo[gid].name) > maxlen) maxlen = strlen(GroupsInfo[gid].name);
	}

	fclose(fp);
	return (count > 0) ? 0 : -1;
}

int
checkgroupinfo(void)
{
	clear();

	if (getgroupset() == -1) {
		move(2, 0);
		prints("【\033[0;1;4;33m注意\033[m】系统发现 menu.ini 的配置可能存在问题，建议检查后再继续进行！\n");
		return askyn("您仍然想继续执行吗", NA, NA);
	}

	return YEA;
}

char
getGroupNum(char ch)
{
	int i;

	for (i = 0; i < MAXGROUPNUM; i++) {
		if (GroupsInfo[i].chs == NULL)
			continue;

		if (strchr(GroupsInfo[i].chs, ch))
			return (i < 10) ? (i + '0') : (i - 10 + 'A');
	}
	return ' ';
}

unsigned int
showpropinfo(unsigned int pbits, int i, int flag)
{
	char str[STRLEN];

	switch (i) {
	case 0:
		strcpy(str, "限制           (RESTRICT)");
		break;
	case 1:
		strcpy(str, "转信           (EXPORT)");
		break;
	case 2:
		strcpy(str, "水版           (JUNK)");
		break;
	case 3:
		strcpy(str, "匿名           (ANONYMOUS)");
		break;
	case 4:
		strcpy(str, "不可回复       (NOREPLY)");
		break;
	case 5:
		strcpy(str, "校园           (HALFOPEN)");
		break;
	case 6:
		strcpy(str, "校内版面       (INTERN)");
		break;
	}

	move(i + 6 - ((i > 15) ? 16 : 0), 0 + ((i > 15) ? 40 : 0));
	prints("%c. %-30s %2s", 'A' + i, str, ((pbits >> i) & 1 ? "是" : "×"));
	refresh();
	return YEA;
}

void
move_cursor(int *cursor_pos, int upper, int lower, int inc)
{
	int pos = *cursor_pos;

	move(pos, 0);
	outc(' ');
	pos += inc;
	if (pos > lower)
		pos = upper;
	if (pos < upper)
		pos = lower;
	move(pos, 0);
	outc('>');

	*cursor_pos = pos;
}

void
show_item(struct boardheader *brd, int item)
{
	char buf[STRLEN];

	move(item, 2);
	clrtoeol();
	switch (item) {
	case 3:
		prints("讨论区名称        %s", brd->filename);
		break;
	case 4:
		prints("讨论区标题        %s", brd->title + 11);
		break;
	case 5:
		strcpy(buf, brd->title + 1);
		buf[6] = 0;
		prints("讨论区分区        %6s", buf);
		break;
	case 6:
		prints("讨论区管理员      %s",
		       (brd->BM[0] == '\0') ? "尚无版主" : brd->BM);
		break;
	case 7:
		prints("发文数目限制      ");
		if (brd->flag & NOPLIMIT_FLAG) {
			prints("无限制");
		} else {
			prints("%d", (brd->flag & BRD_MAXII_FLAG) ? MAX_BOARD_POST_II : MAX_BOARD_POST);
		}
		break;
	case 8:
		prints("公开投票结果      %s公开", (brd->flag & BRD_NOPOSTVOTE) ? "不" : "");
		break;
	case 9:
		prints("允许上传附件      %s", (brd->flag & BRD_ATTACH) ? "允许" : "不允许");
		break;
	case 10:
		buf[0] = '\0';
		prints("讨论区属性        [");

		strcpy(buf, (brd->flag & BRD_RESTRICT) ? "限制，" : "");
		strcat(buf, (brd->flag & OUT_FLAG) ? "转信，" : "");
		strcat(buf, (brd->flag & JUNK_FLAG) ? "水版，" : "");
		strcat(buf, (brd->flag & ANONY_FLAG) ? "匿名，" : "");
		strcat(buf, (brd->flag & NOREPLY_FLAG) ? "不可回复，" : "");
		strcat(buf, (brd->flag & BRD_HALFOPEN) ? "校园，" : "");
		strcat(buf, (brd->flag & BRD_INTERN) ? "校内版面，" : "");

		if (!buf[0]) {
			prints("]");
		} else {
			buf[strlen(buf) - 2] = ']';
			buf[strlen(buf) - 1] = 0;
			outs(buf);
		}
		break;
	case 11:
		prints("READ/POST 限制    ");
		if (brd->level & ~PERM_POSTMASK) {
			outs((brd->level & PERM_POSTMASK) ? "张贴" :
			       (brd->level & PERM_NOZAP) ? "ZAP" : "阅读");
		} else {
			outs("无限制");
		}
		break;
	case 12:
		prints("是否为讨论区列表  %s", (brd->flag & BRD_GROUP) ? "是" : "否");
		break;
	case 13:
		outs("从属列表          ");
		if (brd->parent == 0 || brd->parent > numboards || !(bcache[brd->parent - 1].flag & BRD_GROUP)) {
			outs("无");
		} else {
			outs(bcache[brd->parent - 1].filename);
		}
		break;
	}
	update_endline();
}

void
redraw_boardprop(struct boardheader *brd, int readonly)
{
	int i;

	stand_title("讨论区属性");
	move(1, 0);
	prints("返回[\033[1;32m←\033[m,\033[1;32mq\033[m] %s选择[\033[1;32m↑\033[m,\033[1;32m↓\033[m] 求助[\033[1;32mh\033[m]\n", (readonly == YEA) ? "" : "修改[\033[1;32m→\033[m,\033[1;32mRtn\033[m] ");
	outs("\033[1;44m  项目名称          项目属性                                                  \033[m");

	for (i = 3; i <= LASTITEM; i++)
		show_item(brd, i);
}

void
edit_item(struct boardheader *brd, int *cursor_pos)
{
	int i, j, pos;
	int action, item = *cursor_pos, level = 0;
	char buf[STRLEN], ans[4];
	struct boardheader *tmp = NULL, fh;

	switch (item) {
	case 3:
		strcpy(buf, brd->filename);
		getdata(t_lines - 4, 0, "讨论区名称: ", buf, 18, DOECHO, NA);
		if (buf[0] && killwordsp(buf)) {
			move(t_lines - 3, 0);

			if (!valid_brdname(buf)) {
				prints("\033[1;31m错误：讨论区名称非法\033[m");
				egetch();
			} else {
				if (brd->flag & BRD_RESTRICT) {
					prints("\033[1;31m错误：不能更改限制讨论区的名称\033[m");
					egetch();
				} else {
					if (search_record(BOARDS, tmp, sizeof(struct boardheader), cmpbnames, buf) > 0) {
						prints("\033[1;31m错误：此讨论区已经存在\033[m");
						egetch();
					} else {
						strcpy(brd->filename, buf);
					}
				}
			}
		}
		clear_line(t_lines - 3);
		clear_line(t_lines - 4);
		show_item(brd, 3);
		break;
	case 4:
		strcpy(buf, brd->title + 11);
		getdata(t_lines - 4, 0, "讨论区标题: ", buf, 24, DOECHO, NA);
		if (buf[0] && killwordsp(buf)) {
			strcpy(brd->title + 11, buf);
		}
		clear_line(t_lines - 4);
		show_item(brd, 4);
		break;
	case 5:
		clear();

		move(2, 0);
		prints("讨论区列表：");
		for (i = 0, j = 4; i < MAXGROUPNUM; i++) {
			if (GroupsInfo[i].name != '\0' && GroupsInfo[i].chs != NULL) {
				if (j <= t_lines - 5) {
					move(j, 10);
					prints("┃%s", GroupsInfo[i].name);
					move(j++, 12 + maxlen);
					outs("┃");
				}
			}
		}

		ans[0] = getGroupNum(brd->title[0]);
		ans[1] = '\0';
		while (1) {
			getdata(++j, 0, "该讨论区放置在哪一分区(参考右边提示编号)？: ", ans, 3, DOECHO, NA);

			if (ans[0] == '\0' || (i = getgroupnum(ans[0])) == -1)
				continue;
			if (GroupsInfo[i].name[0] == '\0' || GroupsInfo[i].chs == NULL)
				continue;
			break;
		}
		move(j + 1, 0);
		prints("\n第 %d 区的分类符号参考[\033[32m%s\033[m]: ", i, GroupsInfo[i].chs);
		j += 3;

		while (1) {
			ans[0] = brd->title[0];
			ans[1] = '\0';
			getdata(j, 0, "请输入该讨论区的分类符号: ", ans, 2, DOECHO, NA);
			if (ans[0] != '\0' && strchr(GroupsInfo[i].chs, ans[0]))
				break;
		}
		brd->title[0] = ans[0];

		j++;
		if (brd->title[1] == '[') {
			strcpy(buf, brd->title + 2);
		} else {
			strcpy(buf, brd->title + 1);
		}
		buf[4] = 0;
		getdata(j, 0, "请输入该讨论区的分类描述: ", buf, 5, DOECHO, NA);
		brd->title[1] = '[';
		brd->title[2] = ((unsigned char) buf[0] < 32) ? ' ' : buf[0];
		brd->title[3] = ((unsigned char) buf[1] < 32) ? ' ' : buf[1];
		brd->title[4] = ((unsigned char) buf[2] < 32) ? ' ' : buf[2];
		brd->title[5] = ((unsigned char) buf[3] < 32) ? ' ' : buf[3];
		brd->title[6] = ']';

		redraw_boardprop(brd, NA);
		move_cursor(cursor_pos, 3, LASTITEM, 0);
		break;
	case 6:
		strcpy(buf, brd->BM);
		getdata(t_lines - 4, 0, "版主名单: ", buf,
			sizeof (brd->BM), DOECHO, NA);
		if (buf[0] && killwordsp(buf)) {
			strcpy(brd->BM, buf);
		} else {
			if (askyn("取消该版所有版主", NA, NA) == YEA) {
				brd->BM[0] = 0;
			}
		}

		clear_line(t_lines - 4);
		clear_line(t_lines - 3);
		show_item(brd, 6);
		break;
	case 7:
		move(t_lines - 4, 0);
		snprintf(buf, sizeof(buf),
			"发文数目限制: 1) %d  2) %d  3) 无限制  0) 取消 [0]: ",
			MAX_BOARD_POST, MAX_BOARD_POST_II);
		getdata(t_lines - 4, 0, buf, ans, 2, DOECHO, YEA);
		clear_line(t_lines - 4);
		action = ans[0] - '0';

		switch (action) {
		case 1:
			brd->flag &= ~(NOPLIMIT_FLAG);
			brd->flag &= ~(BRD_MAXII_FLAG);
			break;
		case 2:
			brd->flag &= ~(NOPLIMIT_FLAG);
			brd->flag |= BRD_MAXII_FLAG;
			break;
		case 3:
			brd->flag |= NOPLIMIT_FLAG;
			break;
		}

		show_item(brd, 7);
		show_item(brd, 9);
		break;
	case 8:
		move(t_lines - 4, 0);
		if (askyn("是否公开投票结果", YEA, NA) == YEA)
			brd->flag &= ~BRD_NOPOSTVOTE;
		else
			brd->flag |= BRD_NOPOSTVOTE;
		clear_line(t_lines - 4);
		show_item(brd, 8);
		break;
	case 9:
		move(t_lines - 4, 0);
		if (askyn("是否允许上传附件", YEA, NA) == YEA)
			brd->flag |= BRD_ATTACH;
		else
			brd->flag &= ~BRD_ATTACH;
		clear_line(t_lines - 4);
		show_item(brd, 9);
		break;
	case 10:
		if (brd->flag & BRD_RESTRICT)
			level += 1;
		if (brd->flag & OUT_FLAG)
			level += 2;
		if (brd->flag & JUNK_FLAG)
			level += 4;
		if (brd->flag & ANONY_FLAG)
			level += 8;
		if (brd->flag & NOREPLY_FLAG)
			level += 16;
		if (brd->flag & BRD_HALFOPEN)
			level += 32;
		if (brd->flag & BRD_INTERN)
			level += 64;

		clear();
		move(2, 0);
		prints("设定 '%s' 讨论区属性", brd->filename);
		level = setperms(level, "属性", 7, showpropinfo);

		if (level & 1)
			brd->flag |= BRD_RESTRICT;
		else
			brd->flag &= ~BRD_RESTRICT;
		if (level & 2)
			brd->flag |= OUT_FLAG;
		else
			brd->flag &= ~OUT_FLAG;
		if (level & 4)
			brd->flag |= JUNK_FLAG;
		else
			brd->flag &= ~JUNK_FLAG;
		if (level & 8)
			brd->flag |= ANONY_FLAG;
		else
			brd->flag &= ~ANONY_FLAG;
		if (level & 16)
			brd->flag |= NOREPLY_FLAG;
		else
			brd->flag &= ~NOREPLY_FLAG;
		if (level & 32)
			brd->flag |= BRD_HALFOPEN;
		else
			brd->flag &= ~BRD_HALFOPEN;
		if (level & 64)
			brd->flag |= BRD_INTERN;
		else
			brd->flag &= ~BRD_INTERN;

		if (brd->flag & OUT_FLAG) {
			brd->title[8] = 0xa1; /* 这里不知道原来是什么 */
			brd->title[9] = 0xd1;
		} else {
			brd->title[8] = 0xa1;
			brd->title[9] = 0xf0;
		}

		redraw_boardprop(brd, NA);
		move_cursor(cursor_pos, 3, LASTITEM, 0);
		break;
	case 11:
		getdata(t_lines - 3, 0, "READ/POST 限制: 1) 阅读文章  2) 张贴文章  3) 取消限制  0) 取消 [0]: ",
			ans, 2, DOECHO, YEA);

		switch (ans[0]) {
			case '1':
				brd->level &= ~PERM_POSTMASK;
				brd->level &= ~PERM_NOZAP;

				break;
			case '2':
				brd->level |= PERM_POSTMASK;
				break;
			case '3':
				brd->level = 0;
				break;
		}

		if (ans[0] == '1' || ans[0] == '2') {
			stand_title("设定讨论区权限");
			move(2, 0);
			prints("设定 %s '%s' 讨论区的权限\n",
			       brd->level & PERM_POSTMASK ? "张贴" : "阅读",
			       brd->filename);
			brd->level = setperms(brd->level, "权限", NUMPERMS, showperminfo);
		}

		redraw_boardprop(brd, NA);
		move_cursor(cursor_pos, 3, LASTITEM, 0);
		break;
	case 12:
		move(t_lines - 4, 0);
		if (askyn("该讨论区是否为讨论区列表", NA, NA) == YEA)
			brd->flag |= BRD_GROUP;
		else
			brd->flag &= ~BRD_GROUP;
		clear_line(t_lines - 4);
		show_item(brd, 12);
		break;
	case 13:
		clear();
		if (askyn("是否取消该版的从属列表", YEA, NA) == YEA) {
			brd->parent = 0;
		} else {
			if (gettheboardname(3, "输入讨论区列表名称: ", &pos, &fh, buf)) {
				if (fh.flag & BRD_GROUP)
					brd->parent = pos;
			}
		}
		redraw_boardprop(brd, NA);
		move_cursor(cursor_pos, 3, LASTITEM, 0);
		break;
	}
}

int
m_editboard(struct boardheader *brd, int pos, int readonly)
{
	int cursor_pos = 3, new_board, ch;
	char vdir[PATH_MAX + 1], bdir[PATH_MAX + 1], adir[PATH_MAX + 1];
	char old_vdir[PATH_MAX + 1], old_bdir[PATH_MAX + 1], old_adir[PATH_MAX + 1];
	struct boardheader old_brd;

	memcpy(&old_brd, brd, sizeof (old_brd));
	redraw_boardprop(brd, readonly);
	move_cursor(&cursor_pos, 3, LASTITEM, 0);

	while (1) {
		ch = egetch();
		switch (ch) {
		case 'h':
			show_help("help/boardcphelp");
			redraw_boardprop(brd, readonly);
			move_cursor(&cursor_pos, 3, LASTITEM, 0);
			break;
		case 'q':
		case KEY_LEFT:
			goto update_board;
			break;
		case 'j':
		case KEY_UP:
			move_cursor(&cursor_pos, 3, LASTITEM, -1);
			break;
		case 'k':
		case KEY_DOWN:
			move_cursor(&cursor_pos, 3, LASTITEM, 1);
			break;
		case KEY_HOME:
			move_cursor(&cursor_pos, 3, LASTITEM, LASTITEM);
			break;
		case '$':
		case KEY_END:
			move_cursor(&cursor_pos, 3, LASTITEM, -LASTITEM);
			break;
		case '\n':
		case KEY_RIGHT:
			if (readonly == NA) {
				edit_item(brd, &cursor_pos);
			}
			break;
		}
	}

      update_board:
	if (readonly == YEA)
		return 0;

	clear();
	move(2, 0);
	if (askyn((pos < 0) ? "您确认要创建讨论区吗" : "您确认要更改讨论区属性吗", NA, NA) == NA) {
		pressanykey();
		return 0;
	}

	if (pos < 0) {
		new_board = 1;
		pos = getbnum("");
	} else {
		new_board = 0;
	}

	snprintf(vdir, sizeof(vdir), "vote/%s", brd->filename);
	snprintf(bdir, sizeof(bdir), "boards/%s", brd->filename);
	snprintf(adir, sizeof(adir), "0Announce/boards/%s", brd->filename);

	if (!new_board) {
		snprintf(old_vdir, sizeof(old_vdir), "vote/%s", old_brd.filename);
		snprintf(old_bdir, sizeof(old_bdir), "boards/%s", old_brd.filename);
		snprintf(old_adir, sizeof(old_adir), "0Announce/boards/%s", old_brd.filename);

		/* 更改讨论区相关目录 */
		rename(old_vdir, vdir);
		rename(old_bdir, bdir);
		rename(old_adir, adir);

		if (f_mkdir(vdir, 0755) == -1 || f_mkdir(bdir, 0755) == -1 || f_mkdir(adir, 0755) == -1) {
			outs("\n更改讨论区目录时出错!");
			pressanykey();
			return 0;
		}

		/* 修改 .BOARDS */
		if (substitute_record(BOARDS, brd, sizeof (struct boardheader), pos) == -1) {
			refresh_bcache();
			prints("\n修改讨论区属性失败！\n");
			pressanykey();
			return 0;
		}
	} else {
		/* 初始化讨论区目录 */
		if (f_mkdir(vdir, 0755) == -1 || f_mkdir(bdir, 0755) == -1 || f_mkdir(adir, 0755) == -1) {
			prints("\n更改讨论区目录时出错!\n");
			pressanykey();
			return 0;
		}

		/* 向 .BOARDS 增添新版记录 */
		if (append_record(BOARDS, brd, sizeof (struct boardheader)) == -1) {
			refresh_bcache();
			prints("\n成立新版失败！\n");
			pressanykey();
			return 0;
		}
	}
	refresh_bcache();

	if (!new_board) {
		snprintf(genbuf, sizeof(genbuf), "修改讨论区：%s(%s)", old_brd.title + 11, brd->filename);
	} else {
		snprintf(genbuf, sizeof(genbuf), "成立新版：%s", brd->filename);
	}
	securityreport(genbuf);

	if ( !new_board ) {
		/* clear the board control file when the boardname is changed, or
		   the restrict attribution is removed */ 
		int clear_brdctl = (old_brd.flag & BRD_RESTRICT) &&
			( strcmp(old_brd.filename,  brd->filename) || 
			  !(brd->flag & BRD_RESTRICT) );

		if (clear_brdctl){
			char boardctl[STRLEN], oldboard[BFNAMELEN], id[IDLEN + 1];
			int len;
			FILE* fp;

			setboardfile(boardctl, brd->filename, "board.ctl");
			
			if( (fp = fopen(boardctl, "r")) != NULL) {
				strlcpy(oldboard, currboard, sizeof(oldboard));
				strlcpy(currboard, old_brd.filename, sizeof(currboard));
				while(fgets(id, sizeof(id),  fp)) {
					len =  strlen(id);
					if( id[len - 1] == '\n' )
						id[len - 1] = '\0';
					update_boardlist(LE_REMOVE, NULL, id);
				}
				strlcpy(currboard, oldboard, sizeof(currboard));
				fclose(fp);
			}
			unlink(boardctl);
		}
	}
	if (brd->flag & BRD_RESTRICT) {
		if (askyn("您需要为讨论区设置成员名单吗? ", YEA, NA) == YEA) {
			char boardctl[STRLEN], oldboard[BFNAMELEN];

			strlcpy(oldboard, currboard, sizeof(oldboard));
			strlcpy(currboard, brd->filename, sizeof(currboard));
			setboardfile(boardctl, brd->filename, "board.ctl");
			listedit(boardctl, "编辑『本版成员』名单", update_boardlist);
			strlcpy(currboard, oldboard, sizeof(currboard));
			return 0;
		}
	}

	/* monster: 强制刷新BCACHE */
	refresh_bcache();

	pressanykey();
	return 0;
}

int
m_newbrd(void)
{
	struct boardheader newbrd, dh;
	char buf[STRLEN];

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return -1;

	if (checkgroupinfo() == NA)
		return -1;

	stand_title("开启新讨论区");
	move(2, 0);
	prints("您将开始创建一个\033[32m新\033[m的讨论区 [ENTER-->取消创建]");

	while (1) {
		getdata(3, 0, "讨论区名称(英文名): ", buf, 18, DOECHO, YEA);

		if (!buf[0])
			return 0;
		if (killwordsp(buf) == 0 || !valid_brdname(buf))
			continue;

		if (search_record(BOARDS, &dh, sizeof(dh), cmpbnames, buf) > 0) {
			prints("\n错误! 此讨论区已经存在!!");
			pressanykey();
			return -1;
		}
		memset(&newbrd, 0, sizeof (newbrd));
		strcpy(newbrd.filename, buf);
		strcpy(newbrd.title, "a[本站] ○ 新讨论区");
		break;
	}

	m_editboard(&newbrd, -1, NA);
	return 0;
}

int
m_editbrd(void)
{
	int pos;
	struct boardheader fh;
	char bname[STRLEN];

	modify_user_mode(ADMIN);
	if (!check_systempasswd())
		return -1;

	if (checkgroupinfo() == NA)
		return -1;

	stand_title("修改讨论区基本属性");
	if (!gettheboardname(2, "输入讨论区名称: ", &pos, &fh, bname))
		return -1;

	m_editboard(&fh, pos, NA);
	return 0;
}

#ifdef RESTART_BBSD
/* Pudding: 在管理界面重启bbsd */
int
m_restart_bbsd(void)
{
	/* 若不是设置euid, 重启功能也无法执行 */
#ifndef SET_EFFECTIVE_ID
	return 0;
#endif
	modify_user_mode(ADMIN);	
	clear();
	if (!HAS_PERM(PERM_SYSOP) || (strcmp(raw_fromhost, "127.0.0.1") != 0)) {
		prints("您无权执行此项功能\n");
		pressanykey();
		return -1;
	}
	if (!askyn("确定要重启bbsd吗?", NA, 0)) {
		pressanykey();
		return 0;
	}

	prints("重启中...\n");
	/* Do it */
	char buf[256];
#ifdef SET_EFFECTIVE_ID
	if (getuid() == 0)
		seteuid(0);
#endif
	kill(getppid(), SIGKILL);
	snprintf(buf, sizeof(buf), BBSHOME"/bin/bbsd %d", bbsport);
	prints("%s\n", buf);
	system(buf);
	prints("成功\n");
	pressanykey();
	return 0;
}
#endif
