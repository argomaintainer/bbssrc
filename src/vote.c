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
#include "vote.h"

extern int page, range;
static char *vote_type[] = { "是非", "单选", "复选", "数字", "问答" };
struct votebal currvote;
char controlfile[STRLEN];
unsigned int result[33];
int vnum;
int voted_flag;
FILE *sug;

int choose(int update, int defaultn, void (*title_show)(void), int (*key_deal)(int, int, int), int (*list_show)(void), int (*read)(int, int));
int makevote(struct votebal *ball, char *bname);
int setvoteperm(struct votebal *ball);

int
cmpvuid(void *userid, void *uv_ptr)
{
	struct ballot *uv = (struct ballot *)uv_ptr;

	return !strcmp((char *)userid, uv->uid);
}

static void
setvoteflag(char *bname, int flag)
{
	int pos;
	struct boardheader fh;

	if ((pos = search_record(BOARDS, &fh, sizeof(fh), cmpbnames, bname)) > 0) {
		fh.flag = (flag == 0) ? (fh.flag & ~VOTE_FLAG) : (fh.flag | VOTE_FLAG);
		substitute_record(BOARDS, &fh, sizeof(fh), pos);
		refresh_bcache();
	}
}

void
makevdir(char *bname)
{
	char buf[PATH_MAX + 1];

	snprintf(buf, sizeof(buf), "vote/%s", bname);
	f_mkdir(buf, 0755);
}

void
setcontrolfile(void)
{
	setvotefile(controlfile, currboard, "control");
}

int
b_notes_edit(void)
{
	char buf[STRLEN], buf2[STRLEN];
	char ans[4];
	int aborted, notetype;

	if (!HAS_PERM(PERM_ACBOARD))    /* monster: suggested by MidautumnDay */
		if (!current_bm)
			return 0;

	clear();
	move(1, 0);
	outs("编辑/删除本版备忘录");
	getdata(3, 0, "编辑或删除 (0)离开 (1)一般备忘录 (2)秘密备忘录? [1]: ",
		ans, 2, DOECHO, YEA);
	if (ans[0] == '0')
		return FULLUPDATE;
	if (ans[0] != '2') {
		setvotefile(buf, currboard, "notes");
		notetype = 1;
	} else {
		setvotefile(buf, currboard, "secnotes");
		notetype = 2;
	}
	snprintf(buf2, sizeof(buf2), "[%s] (E)编辑 (D)删除 (A)取消 [E]: ",
		(notetype == 1) ? "一般备忘录" : "秘密备忘录");
	getdata(5, 0, buf2, ans, 2, DOECHO, YEA);
	if (ans[0] == 'A' || ans[0] == 'a') {
		aborted = -1;
	} else if (ans[0] == 'D' || ans[0] == 'd') {
		move(6, 0);
		snprintf(buf2, sizeof(buf2), "真的要删除[%s]", (notetype == 1) ? "一般备忘录" : "秘密备忘录");
		if (askyn(buf2, NA, NA) == YEA) {
			unlink(buf);
			move(7, 0);
			prints("[%s]已经删除...\n", (notetype == 1) ? "一般备忘录" : "秘密备忘录");
			pressanykey();
			aborted = 1;
		} else {
			aborted = -1;
		}
	} else {
		aborted = vedit(buf, EDIT_MODIFYHEADER);
	}

	if (aborted == -1) {
		pressreturn();
	} else {
		if (notetype == 1) {
			setvotefile(buf, currboard, "noterec");
		} else if (notetype == 2) {
			setvotefile(buf, currboard, "notespasswd");
		}
		unlink(buf);
	}
	return FULLUPDATE;
}

int
b_notes_passwd(void)
{
	FILE *pass;
	char passbuf[PASSLEN + 1], prepass[PASSLEN + 1];
	char buf[STRLEN];
	unsigned char genpass[MD5_PASSLEN];

	if (!current_bm) {
		return 0;
	}
	clear();
	move(1, 0);
	outs("设定/更改/取消「秘密备忘录」密码...");
	setvotefile(buf, currboard, "secnotes");
	if (!dashf(buf)) {
		move(3, 0);
		outs("本讨论区尚无「秘密备忘录」。\n\n");
		outs("请先用 W 编好「秘密备忘录」再来设定密码...");
		pressanykey();
		return FULLUPDATE;
	}
	if (!check_notespasswd())
		return FULLUPDATE;
	getdata(3, 0, "请输入新的秘密备忘录密码(Enter 取消密码): ", passbuf,
		PASSLEN, NOECHO, YEA);
	if (passbuf[0] == '\0') {
		setvotefile(buf, currboard, "notespasswd");
		unlink(buf);
		outs("已经取消备忘录密码。");
		pressanykey();
		return FULLUPDATE;
	}
	getdata(4, 0, "确认新的秘密备忘录密码: ", prepass, PASSLEN, NOECHO,
		YEA);
	if (strcmp(passbuf, prepass)) {
		outs("\n密码不相符, 无法设定或更改....");
		pressanykey();
		return FULLUPDATE;
	}
	setvotefile(buf, currboard, "notespasswd");
	if ((pass = fopen(buf, "w")) == NULL) {
		move(5, 0);
		outs("备忘录密码无法设定....");
		pressanykey();
		return FULLUPDATE;
	}
	genpasswd(passbuf, genpass);
	fwrite(genpass, MD5_PASSLEN, 1, pass);
	fclose(pass);
	move(5, 0);
	outs("秘密备忘录密码设定完成....");
	pressanykey();
	return FULLUPDATE;
}

int
b_suckinfile(FILE *fp, char *fname)
{
	char inbuf[256];
	FILE *sfp;

	if ((sfp = fopen(fname, "r")) == NULL)
		return -1;
	while (fgets(inbuf, sizeof(inbuf), sfp) != NULL)
		fputs(inbuf, fp);
	fclose(sfp);
	return 0;
}

/*Add by SmallPig*/

/* 将fname第2行开始的内容添加到 fp 后面 */
int
catnotepad(FILE *fp, char *fname)
{
	char inbuf[256];
	FILE *sfp;
	int count;

	count = 0;
	if ((sfp = fopen(fname, "r")) == NULL) {
		fprintf(fp,
			"\033[1;34m  □\033[44m__________________________________________________________________________\033[m \n\n");
		return -1;
	}
	while (fgets(inbuf, sizeof(inbuf), sfp) != NULL) {
		if (count != 0)
			fputs(inbuf, fp);
		else
			count++;
	}
	fclose(sfp);
	return 0;
}

int
b_closepolls(void)
{
	char buf[80];
	time_t now, nextpoll;
	int i, end;

	now = time(NULL);
	resolve_boards();

	if (now < brdshm->pollvote) {
		return 0;
	}
	move(t_lines - 1, 0);
	outs("对不起，系统关闭投票中，请稍候...");
	refresh();
	nextpoll = now + 7 * 3600;
	brdshm->pollvote = nextpoll;
	strcpy(buf, currboard);
	for (i = 0; i < brdshm->number; i++) {
		strcpy(currboard, (&bcache[i])->filename);
		setcontrolfile();
		end = get_num_records(controlfile, sizeof(currvote));
		for (vnum = end; vnum >= 1; vnum--) {
			time_t closetime;

			get_record(controlfile, &currvote, sizeof(currvote), vnum);
			closetime = currvote.opendate + currvote.maxdays * 86400;
			if (now > closetime) {
				mk_result(vnum);
			} else if (nextpoll > closetime) {
				nextpoll = closetime + 300;
				brdshm->pollvote = nextpoll;
			}
		}
	}
	strcpy(currboard, buf);

	return 0;
}

int
count_result(void *bal_ptr, int unused)
{
	int i, j = 0;
	char choices[33];
	struct ballot *bal = (struct ballot *)bal_ptr;

	if (bal->msg[0][0] != '\0') {
		if (currvote.type == VOTE_ASKING) {
			fprintf(sug, "\033[1m%s \033[m的作答如下：\n", bal->uid);
		} else {
			if (currvote.type != VOTE_VALUE) {
				memset(choices, 0, sizeof(choices));
		                for (i = 0; i < 32; i++) {
                		        if ((bal->voted >> i) & 1) {
						choices[j] = 'A' + i;
						++j;
					}
				}
			}

			if (j == 0 || !currvote.report) {
				fprintf(sug, "\033[1m%s\033[m 的建议如下：\n", bal->uid);
			} else {
				fprintf(sug, "\033[1m%s\033[m 的选项为 \033[1;36m%s\033[m ，建议如下：\n", bal->uid, choices);
			}
		}
		for (i = 0; (bal->msg[i] && i < 3); i++) {
			fprintf(sug, "%s\n", bal->msg[i]);
		}
		fputc('\n', sug);
	}

	++result[32];
	if (currvote.type != VOTE_ASKING) {
		if (currvote.type != VOTE_VALUE) {
			for (i = 0; i < 32; i++) {
				if ((bal->voted >> i) & 1)
					(result[i])++;
			}
		} else {
			result[31] += bal->voted;
			result[(bal->voted * 10) / (currvote.maxtkt + 1)]++;
		}
	}
	return 0;
}

int
count_voteip(void *bal_ptr, int unused)
{
	time_t t;
	struct ballot *bal = (struct ballot *)bal_ptr;

	t = bal->votetime;
	fprintf(sug, "%-12s    %-16s   %24.24s\n", bal->uid, bal->votehost, ctime(&t));
	return 0;
}

void
get_result_title(void)
{
	char buf[PATH_MAX + 1];

	getdatestring(currvote.opendate);
	fprintf(sug, "⊙ 投票开启于：\033[1m%s\033[m  类别：\033[1m%s\033[m\n",
		datestring, vote_type[currvote.type - 1]);
	fprintf(sug, "⊙ 主题：\033[1m%s\033[m\n", currvote.title);
	if (currvote.type == VOTE_VALUE)
		fprintf(sug, "⊙ 此次投票的值不可超过：\033[1m%d\033[m\n\n",
			currvote.maxtkt);
	fprintf(sug, "⊙ 票选题目描述：\n\n");
	snprintf(buf, sizeof(buf), "vote/%s/desc.%d", currboard, (int)currvote.opendate);
	b_suckinfile(sug, buf);
}

/* monster: 生成彩色条形图 */
void
make_colorbar(int ratio, char *bar)
{
	bar[0] = 0;

	while (ratio >= 10) {
		ratio -= 10;
		strcat(bar, "█");
	}

	switch (ratio) {
	case 9:
	case 8:
		strcat(bar, "▉");
		break;
	case 7:
		strcat(bar, "▊");
		break;
	case 6:
		strcat(bar, "▋");
		break;
	case 5:
		strcat(bar, "▌");
		break;
	case 4:
		strcat(bar, "▍");
		break;
	case 3:
		strcat(bar, "▎");
		break;
	case 2:
	case 1:
		strcat(bar, "▏");
		break;
	}
}

int
mk_result(int num)
{
	char fname[PATH_MAX + 1], nname[PATH_MAX + 1], sugname[PATH_MAX + 1];
	char title[TITLELEN], color_bar[21];
	int i, ratio;
	unsigned int total = 0;

	setcontrolfile();
	snprintf(fname, sizeof(fname), "vote/%s/flag.%d", currboard, (int)currvote.opendate);
/*      count_result(NULL); */
	sug = NULL;
	snprintf(sugname, sizeof(sugname), "vote/%s/tmp.%d", currboard, uinfo.pid);
	if ((sug = fopen(sugname, "w")) == NULL)
		goto error_process;
	memset(result, 0, sizeof(result));

	fprintf(sug,
		"\033[1;44;36m——————————————┤使用者%s├——————————————\033[m\n\n\n",
		(currvote.type != VOTE_ASKING) ? "建议或意见" : "此次的作答");

	if (apply_record(fname, count_result, sizeof(struct ballot)) == -1)
		report("Vote apply flag error");
	fclose(sug);

	snprintf(nname, sizeof(nname), "vote/%s/results", currboard);
	if ((sug = fopen(nname, "w")) == NULL)
		goto error_process;

	get_result_title();
	fprintf(sug, "** 投票结果:\n\n");
	if (currvote.type == VOTE_VALUE) {
		total = result[32];
		for (i = 0; i < 10; i++) {
			/* monster: 使用彩色条形图来表示投票结果, based on henry's idea */
			ratio = (total <= 1) ? (result[i] * 100) : (result[i] * 100) / total;
			make_colorbar(ratio, color_bar);
			fprintf(sug,
				"\033[1m  %4d\033[m 到 \033[1m%4d\033[m 之间有 \033[1m%4d\033[m 票 \033[1;%dm%s\033[m\033[1m%d%%\033[m\n",
				(i * currvote.maxtkt) / 10 + ((i == 0) ? 0 : 1),
				((i + 1) * currvote.maxtkt) / 10, result[i],
				31 + i % 7, color_bar, ratio);

		}
		fprintf(sug, "此次投票结果平均值是: \033[1m%d\033[m\n", (total <= 1) ? (result[31]) : (result[31] / total));
	} else if (currvote.type == VOTE_ASKING) {
		total = result[32];
	} else {
		for (i = 0; i < currvote.totalitems; i++) {
			total += result[i];
		}
		for (i = 0; i < currvote.totalitems; i++) {
			ratio = (result[i] * 100) / ((total <= 0) ? 1 : total);
			make_colorbar(ratio, color_bar);
			fprintf(sug,
				"(%c) %-40s  %4d 票 \033[1;%dm%s\033[m\033[1m\033[1m%d%%\033[m\n",
				'A' + i, currvote.items[i], result[i],
				31 + i % 7, color_bar, ratio);
		}
	}

	fprintf(sug, "\n投票总人数 = \033[1m%d\033[m 人\n", result[32]);
	fprintf(sug, "投票总票数 =\033[1m %d\033[m 票\n\n", total);
	b_suckinfile(sug, sugname);
	unlink(sugname);

	if (currvote.report) {
		fprintf(sug, "\n\033[1;44;36m———————————————┤投票者资料├————————————————\033[m\n\n");
		fprintf(sug, "投票者          投票IP             投票时间\n");
		fprintf(sug, "============================================================\n");
		apply_record(fname, count_voteip, sizeof(struct ballot));
	}

	add_syssign(sug);
	fclose(sug);

	if (!strcmp(currboard, SYSTEM_VOTE)) {
		strlcpy(title, "[公告] 全站投票结果", sizeof(title));
	} else {
		snprintf(title, sizeof(title), "[公告] %s 版的投票结果", currboard);
	}

	if (can_post_vote(currboard))
		postfile(nname, VOTE_BOARD, title, 1);
	if (strcmp(currboard, VOTE_BOARD) != 0)
		postfile(nname, currboard, title, 1);
	dele_vote(num);
	return 0;

error_process:
	if (askyn("\033[1;31m结束投票时发生错误, 是否删除该投票", NA, NA) == YEA)
		dele_vote(num);
	pressanykey();
	return -1;
}

int
get_vitems(struct votebal *bal)
{
	int num;
	char buf[STRLEN];

	move(3, 0);
	outs("请依序输入可选择项, 按 ENTER 完成设定.\n");
	for (num = 0; num < 32; num++) {
		snprintf(buf, sizeof(buf), "%c) ", num + 'A');
		getdata((num & 15) + 4, (num >> 4) * 40, buf, bal->items[num], 36, DOECHO, YEA);
		if (bal->items[num][0] == '\0') {
			if (num == 0) {
				num = -1;
			} else {
				break;
			}
		}
	}
	bal->totalitems = num;
	return num;
}

/* monster: 检查当前版是否公开投票结果 */
int
can_post_vote(char *board)
{
	struct boardheader *bp;

	/* monster: 全站投票必须公开投票结果 */
	if (!strcmp(board, SYSTEM_VOTE)) {
		return YEA;
	}

	bp = getbcache(board);
	return (bp->flag & BRD_NOPOSTVOTE) ? NA : YEA;
}

int
vote_maintain(char *bname)
{
	char buf[STRLEN], ans[2];
	struct votebal *ball = &currvote;

	setcontrolfile();
	if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE))
		if (!current_bm)
			return 0;

	stand_title("开启投票箱");
	makevdir(bname);
	for (;;) {
		getdata(2, 0,
			"(1)是非 (2)单选 (3)复选 (4)数值 (5)问答 (6)取消 [6]: ",
			ans, 2, DOECHO, YEA);
		ans[0] -= '0';
		if (ans[0] < 1 || ans[0] > 5) {
			outs("取消此次投票\n");
			return FULLUPDATE;
		}
		ball->type = (int) ans[0];
		break;
	}
	ball->opendate = time(NULL);
	outc('\n');
	ball->report = askyn("是否记录投票者资料 （投票IP及时间）", NA, NA);
	if (makevote(ball, bname))
		return FULLUPDATE;
	setvoteflag(currboard, 1);
	clear();
	strcpy(ball->userid, currentuser.userid);
	if (append_record(controlfile, ball, sizeof(*ball)) == -1) {
		outs("无法开启投票，请与系统维护联系");
		b_report("Append Control file Error!!");
	} else {
		char votename[PATH_MAX + 1];
		int i;

		b_report("OPEN");
		outs("投票箱开启了！\n");
		range++;
		if (!can_post_vote(currboard)) {
			pressreturn();
			return FULLUPDATE;
		}
		snprintf(votename, sizeof(votename), "tmp/votetmp.%s.%05d", currentuser.userid, uinfo.pid);
		if ((sug = fopen(votename, "w")) != NULL) {
			if (!strcmp(currboard, SYSTEM_VOTE)) {
				snprintf(buf, sizeof(buf), "[通知] 全站投票：%s", ball->title);
			} else {
				snprintf(buf, sizeof(buf), "[通知] %s 举办投票：%s", currboard, ball->title);
			}
			get_result_title();
			if (ball->type != VOTE_ASKING &&
			    ball->type != VOTE_VALUE) {
				fprintf(sug, "\n【\033[1m选项如下\033[m】\n");
				for (i = 0; i < ball->totalitems; i++) {
					fprintf(sug, "(\033[1m%c\033[m) %-40s\n",
						'A' + i, ball->items[i]);
				}
			}
			add_syssign(sug);
			fclose(sug);

			if (can_post_vote(currboard))
				postfile(votename, VOTE_BOARD, buf, 1);
			if (strcmp(currboard, VOTE_BOARD) != 0)
				postfile(votename, currboard, buf, 1);

			unlink(votename);
		}
	}
	pressreturn();
	return FULLUPDATE;
}

int
makevote(struct votebal *ball, char *bname)
{
	char buf[PATH_MAX + 1];

	outs("\n请按任何键开始编辑此次 [投票的描述]: \n");
	igetkey();
	setvotefile(genbuf, bname, "desc");
	snprintf(buf, sizeof(buf), "%s.%d", genbuf, (int)ball->opendate);
	if (vedit(buf, EDIT_MODIFYHEADER) == -1) {
		clear();
		outs("取消此次投票设定\n");
		pressreturn();
		return 1;
	}

	clear();
	do {
		getdata(0, 0, "此次投票所须天数[1]: ", buf, 3, DOECHO, YEA);
		if (buf[0] == '\0') {
			ball->maxdays = 1;
			break;
		}
	} while ((ball->maxdays = atoi(buf)) <= 0);

	while (1) {
		getdata(1, 0, "投票箱的标题: ", ball->title, TITLELEN, DOECHO, YEA);
		if (killwordsp(ball->title) != 0)
			break;
		bell();
	}

	switch (ball->type) {
	case VOTE_YN:
		ball->maxtkt = 0;
		strcpy(ball->items[0], "赞成  （是的）");
		strcpy(ball->items[1], "不赞成（不是）");
		strcpy(ball->items[2], "没意见（不清楚）");
		ball->maxtkt = 1;
		ball->totalitems = 3;
		break;
	case VOTE_SINGLE:
		get_vitems(ball);
		ball->maxtkt = 1;
		break;
	case VOTE_MULTI:
		get_vitems(ball);
		for (;;) {
			snprintf(buf, sizeof(buf), "一个人最多几票? [%d]: ", ball->totalitems);
			getdata(21, 0, buf, buf, 5, DOECHO, YEA);
			ball->maxtkt = atoi(buf);
			if (ball->maxtkt <= 0)
				ball->maxtkt = ball->totalitems;
			if (ball->maxtkt > ball->totalitems)
				continue;
			break;
		}
		if (ball->maxtkt == 1)
			ball->type = VOTE_SINGLE;
		break;
	case VOTE_VALUE:
		for (;;) {
			getdata(3, 0, "输入数值最大不得超过 [100]: ", buf, 4,
				DOECHO, YEA);
			ball->maxtkt = atoi(buf);
			if (ball->maxtkt <= 0)
				ball->maxtkt = 100;
			break;
		}
		break;
	case VOTE_ASKING:
		ball->maxtkt = 0;
		currvote.totalitems = 0;
		break;
	default:
		ball->maxtkt = 1;
		break;
	}
	return 0;
}

int
setvoteperm(struct votebal *ball)
{
	int flag = NA, changeit = YEA;
	char buf[6], msgbuf[STRLEN];

	if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE))
		return 0;

	clear();
	prints("当前投票标题：[\033[1;33m%s\033[m]", ball->title);
	outs("\n您是本站的投票管理员，您可以对投票进行投票受限设定\n"
	     "\n投票受限设定共有三种限制，这三种限制可以做综合限定。\n"
	     "    (1) 权限限制 [\033[32m用户必须拥有某些权限才可以投票\033[m]\n"
	     "    (2) 投票名单 [\033[35m存在于投票名单中的人才可以投票\033[m]\n"
	     "    (3) 条件审核 [\033[36m用户必须满足一定的条件才可投票\033[m]\n"
	     "\n下面系统将一步一步引导您来设置投票受限设定。\n\n");
	if (askyn("您确定要开始对该投票进行投票受限设定吗", NA, NA) == NA)
		return 0;
	flag = (ball->level & ~(LISTMASK | VOTEMASK)) ? YEA : NA;
	prints("\n(\033[36m1\033[m) 权限限制  [目前状态: \033[32m%s\033[m]\n\n",
	     flag ? "有限制" : "无限制");
	if (flag == YEA) {
		if (askyn("需要取消投票者的权限限制吗", NA, NA) == YEA)
			flag = NA;
		else
			changeit =
			    askyn("您需要修改投票者的权限限制吗", NA, NA);
	} else
		flag = askyn("您希望投票者必须具备某种权限吗", NA, NA);
	if (flag == NA) {
		ball->level = ball->level & (LISTMASK | VOTEMASK);
		outs("\n\n当前投票\033[32m无权限限制\033[m！");
	} else if (changeit == NA) {
		outs("\n当前投票\033[32m【保留】权限限制\033[m！");
	} else {
		clear();
		outs("\n设定\033[32m投票者必需\033[m的权限。");
		ball->level = setperms(ball->level, "投票权限", NUMPERMS, showperminfo);
		move(1, 0);
		if (ball->level & ~(LISTMASK | VOTEMASK)) {
			outs("您已经\033[32m设定了\033[m投票的必需权限，系统接受您的设定");
		} else {
			outs("您现在\033[32m取消了\033[m投票的权限限定，系统接受您的设定");
		}
	}
	outs("\n\n\n\033[33m【下一步】\033[m将继续进行投票限制的设定\n");
	pressanykey();
	clear();
	flag = (ball->level & LISTMASK) ? YEA : NA;
	prints("\n(\033[36m2\033[m) 投票名单  [目前状态: \033[32m%s\033[m]\n\n",
	       flag ? "在名单中的 ID 才可投票" : "不受名单限制");
	if (askyn("您想改变本项设定吗", NA, NA) == YEA) {
		if (flag) {
			ball->level &= ~LISTMASK;
			outs("\n您的设定：\033[32m该投票不受投票名单的限定\033[m\n");
		} else {
			ball->level |= LISTMASK;
			outs("\n您的设定：\033[32m只有在名单中的 ID 才可以投票\033[m\n");
			outs("[\033[33m提示\033[m]设定完毕后，用 Ctrl+K 来编辑投票名单\n");
		}
	} else {
		prints("\n您的设定：[\033[32m保留设定\033[m]%s\n",
		       flag ? "在名单中的 ID 才可以投票" : "不受名单限制");
	}
	flag = (ball->level & VOTEMASK) ? YEA : NA;
	prints("\n(\033[36m3\033[m) 条件审核  [目前状态: \033[32m%s\033[m]\n\n",
	       flag ? "受条件限制" : "不受条件限制");
	if (flag == YEA) {
		if (askyn("您想取消投票者的上站次数、上站时间等的限制吗", NA, NA) == YEA) {
			flag = NA;
		} else {
			changeit = askyn("您想重新设定条件限制吗", NA, NA);
		}
	} else {
		changeit = YEA;
		flag = askyn("投票者是否需要受到上站次数等条件的限制", NA, NA);
	}
	if (flag == NA) {
		ball->level &= ~VOTEMASK;
		outs("\n您的设定：\033[32m不受条件限定\033[m\n\n投票受限设定完毕\n\n");
	} else if (changeit == NA) {
		outs("\n您的设定：\033[32m保留条件限定\033[m\n\n投票受限设定完毕\n\n");
	} else {
		ball->level |= VOTEMASK;

		snprintf(msgbuf, sizeof(msgbuf), "上站次数至少达到多少次？[%d]: ", ball->x_logins);
		do {
			getdata(11, 4, msgbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] == '\0' || (ball->x_logins = atoi(buf)) < 0);

		snprintf(msgbuf, sizeof(msgbuf), "发表的文章至少有多少篇？[%d]: ", ball->x_posts);
		do {
			getdata(12, 4, msgbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] == '\0' || (ball->x_posts = atoi(buf)) < 0);

		snprintf(msgbuf, sizeof(msgbuf), "在本站的累计上站时间至少有多少小时？[%d]: ", ball->x_stay);
		do {
			getdata(13, 4, msgbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] == '\0' || (ball->x_stay = atoi(buf)) < 0);
	
		snprintf(msgbuf, sizeof(msgbuf), "该帐号注册时间至少有多少天？[%d]: ", ball->x_live);
		do {
			getdata(14, 4, msgbuf, buf, 5, DOECHO, YEA);
		} while (buf[0] == '\0' || (ball->x_live = atoi(buf)) < 0);

		outs("\n投票受限设定完毕\n\n");
	}
	if (askyn("您确定要修改投票受限吗", NA, NA) == YEA) {
		snprintf(msgbuf, sizeof(msgbuf), "修改 %s 版投票[投票受限]", currboard);
		securityreport(msgbuf);
		return 1;
	} else {
		return 0;
	}
}

int
vote_flag(char *bname, char val, int mode)
{
	char buf[PATH_MAX + 1], flag;
	int fd, num, size;

	num = usernum - 1;
	switch (mode) {
	case 2:
		snprintf(buf, sizeof(buf), "reclog/Welcome.rec");     /* 进站的 Welcome 画面 */
		break;
	case 1:
		setvotefile(buf, bname, "noterec");        /* 讨论区备忘录的旗标 */
		break;
	default:
		return -1;
	}
	if (num >= MAXUSERS) {
		report("Vote Flag, Out of User Numbers");
		return -1;
	}
	if ((fd = open(buf, O_RDWR | O_CREAT, 0600)) == -1) {
		return -1;
	}
	f_exlock(fd);
	size = (int) lseek(fd, 0, SEEK_END);
	memset(buf, 0, sizeof(buf));
	while (size <= num) {
		write(fd, buf, sizeof(buf));
		size += sizeof(buf);
	}
	lseek(fd, (off_t) num, SEEK_SET);
	read(fd, &flag, 1);
	if ((flag == 0 && val != 0)) {
		lseek(fd, (off_t) num, SEEK_SET);
		write(fd, &val, 1);
	}
	f_unlock(fd);
	close(fd);
	return flag;
}

int
vote_check(int bits)
{
	int i, count;

	for (i = count = 0; i < 32; i++) {
		if ((bits >> i) & 1)
			count++;
	}
	return count;
}

unsigned int
showvoteitems(unsigned int pbits, int i, int flag)
{
	int count;

	if (flag == YEA) {
		count = vote_check(pbits);
		if (count > currvote.maxtkt)
			return NA;
		move(2, 0);
		clrtoeol();
		prints("您已经投了 \033[1m%d\033[m 票", count);
	}
	move(i + 6 - ((i > 15) ? 16 : 0), 0 + ((i > 15) ? 40 : 0));
	prints("%c.%2.2s%-36.36s", 'A' + i,
		((pbits >> i) & 1 ? "√" : "  "), currvote.items[i]);

	refresh();
	return YEA;
}

void
show_voteing_title(void)
{
	time_t closedate;
	char buf[PATH_MAX + 1];

	if (currvote.type != VOTE_VALUE && currvote.type != VOTE_ASKING) {
		snprintf(buf, sizeof(buf), "可投票数: \033[1m%d\033[m 票", currvote.maxtkt);
	} else {
		buf[0] = '\0';
	}
	closedate = currvote.opendate + currvote.maxdays * 86400;
	getdatestring(closedate);
	prints("投票将结束于: \033[1m%s\033[m  %s  %s\n",
	       datestring, buf, (voted_flag) ? "(\033[5;1m修改前次投票\033[m)" : "");
	prints("投票主题是: \033[1m%-50s\033[m类型: \033[1m%s\033[m \n", currvote.title,
	       vote_type[currvote.type - 1]);
	if (currvote.report) {
		outs("\033[1;31m注意：本投票将记录投票者资料\033[m\n");
	}
}

int
getsug(struct ballot *uv)
{
	int i, line;

	clear();
	if (currvote.type == VOTE_ASKING) {
		show_voteing_title();
		line = 3;
		outs("请填入您的作答(三行):\n");
	} else {
		line = 1;
		outs("请填入您宝贵的意见(三行):\n");
	}
	move(line, 0);
	for (i = 0; i < 3; i++) {
		prints(": %s\n", uv->msg[i]);
	}
	for (i = 0; i < 3; i++) {
		getdata(line + i, 0, ": ", uv->msg[i], STRLEN - 2, DOECHO, NA);
		if (uv->msg[i][0] == '\0')
			break;
	}
	return i;
}

int
multivote(struct ballot *uv)
{
	unsigned int i;

	i = uv->voted;
	move(0, 0);
	show_voteing_title();
	uv->voted = setperms(uv->voted, "选票", currvote.totalitems, showvoteitems);
	if (uv->voted == i)
		return -1;
	return 1;
}

int
valuevote(struct ballot *uv)
{
	unsigned int chs;
	char buf[10];

	chs = uv->voted;
	move(0, 0);
	show_voteing_title();
	prints("此次作答的值不能超过 \033[1m%d\033[m", currvote.maxtkt);
	if (uv->voted != 0) {
		snprintf(buf, sizeof(buf), "%d", uv->voted);
	} else {
		memset(buf, 0, sizeof(buf));
	}
	do {
		getdata(3, 0, "请输入一个值? [0]: ", buf, 5, DOECHO, NA);
		uv->voted = abs(atoi(buf));
	} while (uv->voted > currvote.maxtkt && buf[0] != '\n' && buf[0] != '\0');
	if (buf[0] == '\n' || buf[0] == '\0' || uv->voted == chs)
		return -1;
	return 1;
}

int
user_vote(int num, int unused)
{
	char fname[PATH_MAX + 1], bname[PATH_MAX + 1], buf[PATH_MAX + 1];
	struct ballot uservote, tmpbal;
	int votevalue, result = NA;
	int aborted = NA, pos;

	move(t_lines - 2, 0);
	get_record(controlfile, &currvote, sizeof(struct votebal), num);
	if (currentuser.firstlogin > currvote.opendate) {
		outs("对不起, 本投票在您帐号申请之前开启，您不能投票\n");
	} else if (!HAS_PERM(currvote.level & ~(LISTMASK | VOTEMASK))) {
		outs("对不起，您目前尚无权在本票箱投票\n");
	} else if (currvote.level & LISTMASK) {
		char listfilename[PATH_MAX + 1];

		setvotefile(listfilename, currboard, "vote.list");
		if (!dashf(listfilename)) {
			outs("对不起，本票箱需要设定好投票名册方可进行投票\n");
		} else if (!seek_in_file(listfilename, currentuser.userid)) {
			outs("对不起, 投票名册上找不到您的大名\n");
		} else {
			result = YEA;
		}
	} else if (currvote.level & VOTEMASK) {
		if (currentuser.numlogins < currvote.x_logins
		    || currentuser.numposts < currvote.x_posts
		    || currentuser.stay < currvote.x_stay * 3600
		    || currentuser.firstlogin >
		    currvote.opendate - currvote.x_live * 86400) {
			outs("对不起，您目前尚不够资格在本票箱投票\n");
		} else {
			result = YEA;
		}
	} else {
		result = YEA;
	}

	if (result == NA) {
		pressanykey();
		return 0;
	}
	snprintf(fname, sizeof(fname), "vote/%s/flag.%d", currboard, (int)currvote.opendate);
	if ((pos = search_record(fname, &uservote, sizeof(uservote), cmpvuid, currentuser.userid)) <= 0) {
		memset(&uservote, 0, sizeof(uservote));
		voted_flag = NA;
	} else {
		voted_flag = YEA;
	}
	strcpy(uservote.uid, currentuser.userid);
	snprintf(bname, sizeof(bname), "desc.%d", (int)currvote.opendate);
	setvotefile(buf, currboard, bname);
	ansimore(buf, YEA);
	clear();
	switch (currvote.type) {
	case VOTE_SINGLE:
	case VOTE_MULTI:
	case VOTE_YN:
		votevalue = multivote(&uservote);
		if (votevalue == -1)
			aborted = YEA;
		break;
	case VOTE_VALUE:
		votevalue = valuevote(&uservote);
		if (votevalue == -1)
			aborted = YEA;
		break;
	case VOTE_ASKING:
		uservote.voted = 0;
		aborted = !getsug(&uservote);
		break;
	}
	clear();
	if (aborted == YEA) {
		prints("保留 【\033[1m%s\033[m】原来的的投票。\n", currvote.title);
	} else {
		strlcpy(uservote.votehost, currentuser.lasthost, sizeof(uservote.votehost));
		uservote.votetime = time(NULL);

		if (currvote.type != VOTE_ASKING)
			getsug(&uservote);

		if ((pos = search_record(fname, &tmpbal, sizeof(tmpbal), cmpvuid, currentuser.userid)) <= 0) {
			if (append_record(fname, &uservote, sizeof(uservote)) == -1) {
				clear_line(2);
				outs("投票失败! 请与系统维护联系\n");
				pressreturn();
			}
		} else {
			substitute_record(fname, &uservote, sizeof(uservote), pos);
		}
		outs("\n已经帮您投入票箱中...\n");
	}
	pressanykey();
	return result;
}

void
voteexp(void)
{
	clrtoeol();
	prints("\033[1;44m编号 开启投票箱者 开启日 %-40s类别 天数 人数\033[m\n", "投票主题");
}

int
printvote(void *ent_ptr, int unused)
{
	static int i;
	struct ballot uservote;
	char buf[PATH_MAX + 1], flagname[PATH_MAX + 1];
	int num_voted;
	struct votebal *ent = (struct votebal *)ent_ptr;

	if (ent == NULL) {
		move(2, 0);
		voteexp();
		i = 0;
		return 0;
	}
	i++;
	if (i > page + (t_lines - 5) || i > range)
		return QUIT;
	else if (i <= page)
		return 0;
	snprintf(buf, sizeof(buf), "flag.%d", (int)ent->opendate);
	setvotefile(flagname, currboard, buf);
	if (search_record(flagname, &uservote, sizeof(uservote), cmpvuid,  currentuser.userid) <= 0) {
		voted_flag = NA;
	} else {
		voted_flag = YEA;
	}
	num_voted = get_num_records(flagname, sizeof(struct ballot));
	getdatestring(ent->opendate);
	prints(" %s%3d %-12.12s %6.6s%s%-40.40s%-4.4s %3d  %4d\033[m\n",
		(voted_flag == NA) ? "\033[1m" : "", i, ent->userid,
		datestring + 6, ent->level ? "\033[33ms\033[37m" : " ",
		ent->title, vote_type[ent->type - 1], ent->maxdays, num_voted);
	return 0;
}

int
dele_vote(int num)
{
	char buf[PATH_MAX + 1];

	snprintf(buf, sizeof(buf), "vote/%s/flag.%d", currboard, (int)currvote.opendate);
	unlink(buf);
	snprintf(buf, sizeof(buf), "vote/%s/desc.%d", currboard, (int)currvote.opendate);
	unlink(buf);
	if (delete_record(controlfile, sizeof(currvote), num) == -1) {
		outs("发生错误，请与系统维护联系...");
		pressanykey();
	}
	range--;
	if (get_num_records(controlfile, sizeof(currvote)) == 0)
		setvoteflag(currboard, NA);
	return PARTUPDATE;
}

int
vote_results(char *bname)
{
	char buf[PATH_MAX + 1];

	setvotefile(buf, bname, "results");
	if (ansimore(buf, YEA) == -1) {
		move(3, 0);
		outs("目前没有任何投票的结果");
		clrtobot();
		pressreturn();
	} else
		clear();
	return FULLUPDATE;
}

int
b_vote_maintain(void)
{
	return vote_maintain(currboard);
}

void
vote_title(void)
{

	docmdtitle("[投票箱列表]",
		   "[\033[1;32m←\033[m,\033[1;32me\033[m] 离开 [\033[1;32mh\033[m] 求助 [\033[1;32m→\033[m,\033[1;32mr <cr>\033[m] 进行投票 [\033[1;32m↑\033[m,\033[1;32m↓\033[m] 上,下选择 \033[1m高亮度\033[m表示尚未投票");
	update_endline();
}

int
vote_key(int ch, int allnum, int pagenum)
{
	int deal = 0;
	char buf[STRLEN];

	switch (ch) {
	case 'v':
	case 'V':
	case '\n':
	case '\r':
	case 'r':
	case KEY_RIGHT:
		user_vote(allnum + 1, 0);
		deal = 1;
		break;
	case 'R':
		vote_results(currboard);
		deal = 1;
		break;
	case 'H':
	case 'h':
		show_help("help/votehelp");
		deal = 1;
		break;
	case 'A':
	case 'a':
		if (!current_bm)
			return YEA;
		vote_maintain(currboard);
		deal = 1;
		break;
	case 'O':
	case 'o':
		if (!current_bm)
			return YEA;
		clear();
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		prints("\033[5;1;31m警告!!\033[m\n投票箱标题：\033[1m%s\033[m\n", currvote.title);
		if (askyn("你确定要提早结束这个投票吗", NA, NA) != YEA) {
			move(2, 0);
			outs("取消提早结束投票行动\n");
			pressreturn();
			clear();
			break;
		}
		mk_result(allnum + 1);
		snprintf(buf, sizeof(buf), "提早结束投票 %s", currvote.title);
		securityreport(buf);
		break;
	case 'T':
		if (!current_bm)
			return YEA;
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		getdata(t_lines - 1, 0, "新投票主题：", currvote.title, 51, DOECHO, YEA);
		if (currvote.title[0] != '\0') {
			substitute_record(controlfile, &currvote,
					  sizeof(struct votebal), allnum + 1);
		}
		break;
	case 's':
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		if (currvote.level == 0)
			break;
		deal = 1;
		clear_line(t_lines - 8);
		prints("\033[47;30m『\033[31m%s\033[30m』投票受限说明：\033[m\n", currvote.title);
		if (currvote.level & ~(LISTMASK | VOTEMASK)) {
			int num, len;

			strcpy(genbuf, "bTCPRD#@XWBA#VS-DOM-F0s2345678");
			len = strlen(genbuf);
			for (num = 0; num < len; num++)
				if (!(currvote.level & (1 << num)))
					genbuf[num] = '-';
			prints("权限受限：[\033[32m%s\033[m]\n", genbuf);
		} else {
			outs("权限受限：[\033[32m本票箱投票不受权限限制\033[m]\n");
		}
		if (currvote.level & LISTMASK) {
			outs("名单受限：[\033[35m只有投票名单中的 ID 才可投票  \033[m]\n");
		} else {
			outs("名单受限：[\033[35m本票箱投票不受投票名单限制    \033[m]\n");
		}
		if (currvote.level & VOTEMASK) {
			prints("          ┏ ①、在本站的上站次数至少 [%-4d] 次\n", currvote.x_logins);
			prints("条件受限：┣ ②、在本站发表文章至少   [%-4d] 篇\n", currvote.x_posts);
			prints("          ┣ ③、实际累计上站时间至少 [%-4d] 小时\n", currvote.x_stay);
			prints("          ┗ ④、该 ID 的注册时间至少 [%-4d] 天\n", currvote.x_live);
		} else {
			outs("条件受限：[\033[36m本票箱投票不受个人条件限制\033[m    ]\n");
		}
		pressanykey();
		break;
	case 'S':
		if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE))
			return YEA;
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		if (setvoteperm(&currvote) != 0) {
			substitute_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		}
		break;
	case Ctrl('K'):
		if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE))
			return YEA;
		deal = 1;
		setvotefile(genbuf, currboard, "vote.list");
		listedit(genbuf, "编辑『投票名单』", NULL);
		break;
	case 'E':
		if (!current_bm)
			return YEA;
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		setvotefile(genbuf, currboard, "desc");
		snprintf(buf, sizeof(buf), "%s.%d", genbuf, (int)currvote.opendate);
		vedit(buf, EDIT_MODIFYHEADER);
		break;
	case 'M':
	case 'm':
		if (!current_bm)
			return YEA;
		clear();
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		prints("\033[5;1;31m警告!!\033[m\n投票箱标题：\033[1m%s\033[m\n", currvote.title);
		if (askyn("你确定要修改这个投票的设定吗", NA, NA) != YEA) {
			move(2, 0);
			outs("取消修改投票行动\n");
			pressreturn();
			clear();
			break;
		}
		outc('\n');
		currvote.report = askyn("是否记录投票者资料 （投票IP、选项及时间）", currvote.report, NA);
		makevote(&currvote, currboard);
		substitute_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		snprintf(buf, sizeof(buf), "修改投票设定 %s", currvote.title);
		securityreport(buf);
		break;
	case 'D':
	case 'd':
		if (!HAS_PERM(PERM_SYSOP | PERM_OVOTE)) {
			if (!current_bm)
				return YEA;
		}
		deal = 1;
		get_record(controlfile, &currvote, sizeof(struct votebal), allnum + 1);
		clear();
		prints("\033[5;1;31m警告!!\033[m\n投票箱标题：\033[1m%s\033[m\n", currvote.title);
		if (askyn("您确定要强制关闭这个投票吗", NA, NA) != YEA) {
			move(2, 0);
			outs("取消强制关闭行动\n");
			pressreturn();
			clear();
			break;
		}
		snprintf(buf, sizeof(buf), "强制关闭投票 %s", currvote.title);
		securityreport(buf);
		dele_vote(allnum + 1);
		break;
	default:
		return 0;
	}
	if (deal) {
		show_votes();
		vote_title();
	}
	return 1;
}

int
show_votes(void)
{
	move(3, 0);
	clrtobot();
	printvote(NULL, 0);
	setcontrolfile();
	if (apply_record(controlfile, printvote, sizeof(struct votebal)) == -1) {
		outs("错误，没有投票箱开启....");
		pressreturn();
	} else {
		clrtobot();
	}
	return 0;
}

int
b_vote(void)
{
	int num_of_vote;
	int voting;

	if (!HAS_PERM(PERM_VOTE) || (currentuser.stay < 1800))
		return DONOTHING;

	setcontrolfile();
	if ((num_of_vote = get_num_records(controlfile, sizeof(struct votebal))) == 0) {
		move(2, 0);
		clrtobot();
		outs("\n抱歉, 目前并没有任何投票举行。\n");
		pressreturn();
		return FULLUPDATE;
	}
	setvoteflag(currboard, 1);
	modify_user_mode(VOTING);
	range = num_of_vote;            /* setlistrange(num_of_vote); */
	clear();
	voting = choose(NA, 0, vote_title, vote_key, show_votes, user_vote);
	clear();
	return FULLUPDATE;
}

int
b_results(void)
{
	return vote_results(currboard);
}

int
m_vote(void)
{
	char buf[STRLEN];

	strcpy(buf, currboard);
	strcpy(currboard, SYSTEM_VOTE);
	modify_user_mode(ADMIN);
	vote_maintain(SYSTEM_VOTE);
	strcpy(currboard, buf);
	return 0;
}

int
x_vote(void)
{
	char buf[STRLEN];

	modify_user_mode(XMENU);
	strcpy(buf, currboard);
	strcpy(currboard, SYSTEM_VOTE);
	b_vote();
	strcpy(currboard, buf);
	return 0;
}

int
x_results(void)
{
	modify_user_mode(XMENU);
	return vote_results(SYSTEM_VOTE);
}
