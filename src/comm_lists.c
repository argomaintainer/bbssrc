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

#define SC_BUFSIZE              20480
#define SC_KEYSIZE              256
#define SC_CMDSIZE              256
#define sysconf_ptr( offset )   (&sysconf_buf[ offset ]);

struct smenuitem {
	int line, col, level;    /* 行, 列, 权限 */
	char *name, *desc, *arg; /* name, description, argument */
	int (*fptr) (); 	 /* 函数指针 */
} *menuitem;

struct sdefine {
	char *key, *str;
	int val;
} *sysvar;

char *sysconf_buf;
int sysconf_menu, sysconf_key, sysconf_len;
/*   menu数目,  key数目, syconf_buf长度 */

typedef struct {
	char *name;
	int (*fptr) ();
	int type;
} MENU;

MENU currcmd;

MENU sysconf_cmdlist[] = {
	{"domenu", domenu, 0},
	{"EGroups", EGroup, 0},
	{"BoardsAll", Boards, 0},
	{"BoardsGood", GoodBrds, 0},
	{"BoardsSysGood", SysGoodBrds, 0},
	{"BoardsNew", New, 0},
	{"LeaveBBS", Goodbye, 0},
	{"Announce", announce, 0},
	{"Personal", show_personal_announce, 0},
	{"SelectBoard", Select, 0},
	{"ReadBoard", Read, 0},
	{"PostArticle", Post, 0},
	{"SetAlarm", setcalltime, 0},
	{"MailAll", mailall, 0},
	{"LockScreen", x_lockscreen, 0},
	{"ShowUser", x_showuser, 0},
	{"OffLine", offline, 0},
	{"ReadNewMail", m_new, 0},
	{"ReadMail", m_read, 0},
	{"SendMail", m_send, 0},
	{"GroupSend", g_send, 0},
#ifdef INTERNET_EMAIL
	{"SendNetMail", m_internet, 0},
#endif
	{"UserDefine", x_userdefine, 0},
	{"ShowFriends", t_friends, 0},
	{"ShowLogins", t_users, 0},
	{"QueryUser", t_query, 0},
#ifdef CHK_FRIEND_BOOK
	{"WaitFriend", wait_friend, 0},
#endif
	{"Talk", t_talk, 0},
	{"SetPager", t_pager, 0},
	{"SetCloak", x_cloak, 0},
	{"SendMsg", s_msg, 0},
	{"ShowMsg", show_allmsgs, 0},
	{"SetFriends", t_friend, 0},
	{"SetRejects", t_reject, 0},
	{"SetMaildeny", t_maildeny, 0},	/* Added by cancel at 01/09/18 */
	{"FriendWall", friend_wall, 0},
	{"EnterChat", ent_chat, 0},
	{"ListLogins", t_list, 0},
	{"Monitor", t_monitor, 0},
/*	{"FillForm", x_fillform, 0}, */		/* Canceled by betterman06.08.25 */
	{"Information", x_info, 0},
	{"EditUFiles", x_edits, 0},
	{"ShowLicense", Conditions, 0},
	{"ShowVersion", Info, 0},
	{"Notepad", shownotepad, 0},
	{"Vote", x_vote, 0},
	{"VoteResult", x_results, 0},
	{"ExecBBSNet", ent_bnet, 0},
	{"ShowWelcome", Welcome, 0},
	{"AllUsers", Users, 0},
	{"AddPCorpus", add_personalcorpus, 0},
#ifdef ALLOWSWITCHCODE
	{"SwitchCode", switch_code, 0},
#endif
	{"Kick", kick_user, 0},
	{"OpenVote", m_vote, 0},
	{"Setsyspass", setsystempasswd, 0},
	{"Register", m_register, 0},
	{"Info", m_info, 0},
	{"Level", x_level, 0},
	{"OrdainBM", m_ordainBM, 0},
	{"RetireBM", m_retireBM, 0},
	{"ChangeLevel", x_denylevel, 0},
	{"DelUser", d_user, 0},
	{"NewBoard", m_newbrd, 0},
	{"ChangeBrd", m_editbrd, 0},
	{"BoardDel", d_board, 0},
	{"SysFiles", a_edits, 0},
	{"Wall", wall, 0},
	{"WinMine", ent_winmine, 0},
	{"Worker", ent_worker, 0},
	{"KeyQuery", x_keyquery, 0},
	{"PostPerm", bm_post_perm, 0},
	{"PostStat", bm_post_stat, 0},
	{"ViewAReport", bm_ann_report, 0},
	{"UserPostStat", user_poststat, 0},
	{"ClearAllFlag", flag_clear_allboards, 0},
#ifdef	RESTART_BBSD
	{"RestartBbsd", m_restart_bbsd, 0},
#endif
	{"Activation", m_activation, 0}, /* Add by betterman 06/07/15 */
	{0, 0, 0}
};

/*
 * 对 str 进行编码, n个连续相同字符ch采用 ’\01' 'ch' 'n'存储 (字符计数方式), 
 * 编码后的字符串保存回 str
 */
void
encodestr(char *str)
{
	char ch, *buf;
	int n;

	buf = str;
	while ((ch = *str++) != '\0') {
		if (*str == ch && str[1] == ch && str[2] == ch) {
			n = 4;
			str += 3;
			while (*str == ch && n < 100) {
				str++;
				n++;
			}
			*buf++ = '\01';
			*buf++ = ch;
			*buf++ = n;
		} else
			*buf++ = ch;
	}
	*buf = '\0';
}

void
decodestr(char *str)
{
	char ch;
	int n;

	while ((ch = *str++) != '\0')
		if (ch != '\01')
			outc(ch);
		else if (*str != '\0' && str[1] != '\0') {
			ch = *str++;
			n = *str++;
			while (--n >= 0)
				outc(ch);
		}
}

#if 0
void *
sysconf_funcptr(char *func_name, int *type)
{
	int n = 0, len;
	char *str;

	str = strchr(func_name, ':');
	len = (str != NULL) ? (str - func_name) : strlen(func_name);

	while ((str = sysconf_cmdlist[n].name) != NULL) {
		if (strncmp(func_name, str, len) == 0) {
			*type = sysconf_cmdlist[n].type;
			return (sysconf_cmdlist[n].fptr);
		}
		n++;
	}

	*type = -1;
	return NULL;
}
#else
void *
sysconf_funcptr(char *func_name, int *type)
{
	int n = 0;
	char *str;

	while ((str = sysconf_cmdlist[n].name) != NULL) {
		if (strcmp(func_name, str) == 0) {
			*type = sysconf_cmdlist[n].type;
			return (sysconf_cmdlist[n].fptr);
		}
		n++;
	}

	*type = -1;
	return NULL;
}
#endif

/* 将str指向的字符串copy到全局的sysconf_buf里, 
 * 并相应增加全局变量 sysconf_len.
 * 返回str在全局sysconf_buf的位置 */
void *
sysconf_addstr(char *str)
{
	int len = sysconf_len;
	char *buf;

	buf = sysconf_buf + len;
	strcpy(buf, str);
	sysconf_len = len + strlen(str) + 1;
	return buf;
}

char *
sysconf_str(char *key)
{
	int n;

	for (n = 0; n < sysconf_key; n++)
		if (strcmp(key, sysvar[n].key) == 0)
			return (sysvar[n].str);
	return NULL;
}

/* 返回 key 所对应的 value
 * if   key 已经在sysvar里, 直接返回其value
 * else 调用strtol返回key所对应的value */
int
sysconf_eval(char *key)
{
	int n;

	for (n = 0; n < sysconf_key; n++)
		if (strcmp(key, sysvar[n].key) == 0)
			return (sysvar[n].val);
	if (*key < '0' || *key > '9') {
		report("sysconf: unknown key: %s.", key);
	}
	return (strtol(key, NULL, 0));
}

void
sysconf_addkey(char *key, char *str, int val)
{
	int num;

	if (sysconf_key < SC_KEYSIZE) {
		if (str == NULL)
			str = sysconf_buf;
		else
			str = sysconf_addstr(str);
		num = sysconf_key++;
		sysvar[num].key = sysconf_addstr(key);
		sysvar[num].str = str;
		sysvar[num].val = val;
	}
}

/*
 * 从fp文件读进menu到 全局变量 menuitem, 并增加syconf_menu变量(menu数目)
 * str会添加到syconf_buf.
 * 会增加一个无用menuitem 
 */
void
sysconf_addmenu(FILE *fp, char *key)
{
	struct smenuitem *pm;
	char buf[256];
	char *cmd, *arg[5], *ptr;
	int n;

	sysconf_addkey(key, "menu", sysconf_menu);
	while (fgets(buf, sizeof (buf), fp) != NULL && buf[0] != '%') {
		cmd = strtok(buf, " \t\n");
		if (cmd == NULL || *cmd == '#') {
			continue;
		}
		arg[0] = arg[1] = arg[2] = arg[3] = arg[4] = "";
		n = 0;
		for (n = 0; n < 5; n++) {
			if ((ptr = strtok(NULL, ",\n")) == NULL)
				break;
			while (*ptr == ' ' || *ptr == '\t')
				ptr++;
			if (*ptr == '"') {
				arg[n] = ++ptr;
				while (*ptr != '"' && *ptr != '\0')
					ptr++;
				*ptr = '\0';
			} else {
				arg[n] = ptr;
				while (*ptr != ' ' && *ptr != '\t' &&
				       *ptr != '\0')
					ptr++;
				*ptr = '\0';
			}
		}
		pm = &menuitem[sysconf_menu++];
		pm->line = sysconf_eval(arg[0]);
		pm->col = sysconf_eval(arg[1]);
		if (*cmd == '@') {
			pm->level = sysconf_eval(arg[2]);
			pm->name = sysconf_addstr(arg[3]);
			pm->desc = sysconf_addstr(arg[4]);
			pm->fptr = sysconf_addstr(cmd + 1);
			pm->arg = pm->name;
		} else if (*cmd == '!') {
			pm->level = sysconf_eval(arg[2]);
			pm->name = sysconf_addstr(arg[3]);
			pm->desc = sysconf_addstr(arg[4]);
			pm->fptr = sysconf_addstr("domenu");
			pm->arg = sysconf_addstr(cmd + 1);
		} else {
			pm->level = -2;
			pm->name = sysconf_addstr(cmd);
			pm->desc = sysconf_addstr(arg[2]);
			pm->fptr = (void *) sysconf_buf;
			pm->arg = sysconf_buf;
		}
	}
	pm = &menuitem[sysconf_menu++];
	pm->name = pm->desc = pm->arg = sysconf_buf;
	pm->fptr = (void *) sysconf_buf;
	pm->level = -1; /* 作为终结标记, see domenu_screen */
}

/* 从fp里读进内容并添加到sysconf_buf中, 并相应改变sysconf_len
 * 直到读到一个%结束. 
 * 读进的内容调用encodestr编码. 
 * 并添加key到sysvar中
 */
void
sysconf_addblock(FILE *fp, char *key)
{
	char buf[256];
	int num;

	if (sysconf_key < SC_KEYSIZE) {
		num = sysconf_key++;
		sysvar[num].key = sysconf_addstr(key);
		sysvar[num].str = sysconf_buf + sysconf_len;
		sysvar[num].val = -1;
		while (fgets(buf, sizeof (buf), fp) != NULL && buf[0] != '%') {
			encodestr(buf);
			strcpy(sysconf_buf + sysconf_len, buf);
			sysconf_len += strlen(buf);
		}
		sysconf_len++;
	} else {
		while (fgets(buf, sizeof (buf), fp) != NULL && buf[0] != '%') {
		}
	}
}

void
parse_sysconf(char *fname)
{
	FILE *fp;
	char buf[256];
	char tmp[256], *ptr;
	char *key, *str;
	int val;

	if ((fp = fopen(fname, "r")) == NULL)
		return;

	sysconf_addstr("(null ptr)");
	while (fgets(buf, sizeof (buf), fp) != NULL) {
		ptr = buf;
		while (*ptr == ' ' || *ptr == '\t')
			ptr++;

		if (*ptr == '%') {
			strtok(ptr, " \t\n");
			if (strcmp(ptr, "%menu") == 0) {
				if ((str = strtok(NULL, " \t\n")) != NULL)
					sysconf_addmenu(fp, str);
			} else {
				sysconf_addblock(fp, ptr + 1);
			}
		} else if (*ptr == '#') {
			if ((key = strtok(ptr, " \t\"\n")) != NULL && 
			    (str = strtok(NULL, " \t\"\n")) != NULL && 
			     strcmp(key, "#include") == 0)
				parse_sysconf(str);
		} else if (*ptr != '\n') {
			if ((key = strtok(ptr, "=#\n")) != NULL && (str = strtok(NULL, "#\n")) != NULL) {
				strtok(key, " \t");
				while (*str == ' ' || *str == '\t')
					str++;
				if (*str == '"') {
					str++;
					strtok(str, "\"");
					val = atoi(str);
					sysconf_addkey(key, str, val);
				} else { 
					/* for example:
					 * PERM_BASIC = 	000001 
					 * key = PERM_BASIC 
					 * val = 1 */
					val = 0;
					strcpy(tmp, str);
					ptr = strtok(tmp, ", \t");
					while (ptr != NULL) {
						val |= sysconf_eval(ptr);
						ptr = strtok(NULL, ", \t");
					}
					sysconf_addkey(key, NULL, val);
				}
			} else {
				report(ptr);
			}
		}
	}
	fclose(fp);
}

void
build_sysconf(char *configfile, char *imgfile)
{
	struct smenuitem *old_menuitem;
	struct sdefine *old_sysvar;
	char *old_buf;
	int old_menu, old_key, old_len;
	struct sysheader {
		char *buf;
		int menu, key, len;
	} shead;
	int fh;

	old_menuitem = menuitem;
	old_menu = sysconf_menu;
	old_sysvar = sysvar;
	old_key = sysconf_key;
	old_buf = sysconf_buf;
	old_len = sysconf_len;
	menuitem = (void *) malloc(SC_CMDSIZE * sizeof (struct smenuitem));
	sysvar = (void *) malloc(SC_KEYSIZE * sizeof (struct sdefine));
	sysconf_buf = (void *) malloc(SC_BUFSIZE);
	sysconf_menu = 0;
	sysconf_key = 0;
	sysconf_len = 0;
	parse_sysconf(configfile);
	if ((fh = open(imgfile, O_WRONLY | O_CREAT, 0644)) > 0) {
		ftruncate(fh, 0);
		shead.buf = sysconf_buf;
		shead.menu = sysconf_menu;
		shead.key = sysconf_key;
		shead.len = sysconf_len;
		write(fh, &shead, sizeof (shead));
		write(fh, menuitem, sysconf_menu * sizeof (struct smenuitem));
		write(fh, sysvar, sysconf_key * sizeof (struct sdefine));
		write(fh, sysconf_buf, sysconf_len);
		close(fh);
	}
	free(menuitem);
	free(sysvar);
	free(sysconf_buf);
	menuitem = old_menuitem;
	sysconf_menu = old_menu;
	sysvar = old_sysvar;
	sysconf_key = old_key;
	sysconf_buf = old_buf;
	sysconf_len = old_len;
}

void
load_sysconf_image(char *imgfile, int rebuild) 
{
	struct sysheader {
		char *buf;
		int menu, key, len;
	} shead;
	struct stat st;
	char *ptr, *func;
	int fh, n, diff, x;

rebuild:
	if (rebuild == YEA) {
		report("build & reload sysconf.img");
		build_sysconf("etc/sysconf.ini", "sysconf.img");
	}

	if ((fh = open(imgfile, O_RDONLY)) > 0) {
		fstat(fh, &st);
		ptr = malloc(st.st_size);
		read(fh, &shead, sizeof (shead));
		read(fh, ptr, st.st_size);
		close(fh);

		menuitem = (void *) ptr;
		ptr += shead.menu * sizeof (struct smenuitem);
		sysvar = (void *) ptr;
		ptr += shead.key * sizeof (struct sdefine);
		sysconf_buf = (void *) ptr;
		ptr += shead.len;
		sysconf_menu = shead.menu;
		sysconf_key = shead.key;
		sysconf_len = shead.len;
		diff = sysconf_buf - shead.buf;
		for (n = 0; n < sysconf_menu; n++) {
			menuitem[n].name += diff;	/* menuitem里保存的是原buf的地址, 现在加上偏移量*/
			menuitem[n].desc += diff;
			menuitem[n].arg += diff;
			func = (char *) menuitem[n].fptr;
			menuitem[n].fptr = sysconf_funcptr(func + diff, &x);
		}
		for (n = 0; n < sysconf_key; n++) {
			sysvar[n].key += diff;
			sysvar[n].str += diff;
		}
	} else if (rebuild == NA) {
		rebuild = YEA;
		goto rebuild;
	}
}

int
domenu_screen(struct smenuitem *pm)
{
	char *str;
	int line, col, num;

/*    if(!DEFINE(DEF_NORMALSCR))  */
	clear();
	line = 3; /* current line */
	col = 0;  /* current col */
	num = 0;
	while (1) {
		switch (pm->level) {
		case -1:
			return (num);
		case -2:
			if (strcmp(pm->name, "title") == 0) {
				firsttitle(pm->desc);
			} else if (strcmp(pm->name, "screen") == 0) { /* 背景 */
				if ((str = sysconf_str(pm->desc)) != NULL) {
					refresh();	/* added by forgetful */
					move(pm->line, pm->col);
					decodestr(str);
				}
			}
			break;
		default:
			refresh();	/* added by forgetful */
			if (pm->line >= 0 && HAS_PERM(pm->level)) {
				if (pm->line == 0) {
					pm->line = line;
					pm->col = col;
				} else {
					line = pm->line;
					col = pm->col;
				}
				move(line, col);
				prints("  %s", pm->desc); /* 两个空格显示 "> "*/
				line++;
			} else {
				if (pm->line > 0) {
					line = pm->line;
					col = pm->col;
				}
				pm->line = -1;
			}
		}
		num++;
		pm++;
	}
}

int
domenu(char *menu_name)
{
	extern int refscreen;
	struct smenuitem *pm;
	int size, now;
	int cmd, i, j;
	int control; // 控制符操作区域

	if (sysconf_menu <= 0) {
		return -1;
	}
	pm = &menuitem[sysconf_eval(menu_name)];
	size = domenu_screen(pm);   /*  画menu */
	now = 0;
	if (strcmp(menu_name, "TOPMENU") == 0 && chkmail(NA)) {
		for (i = 0; i < size; i++)
			if (pm[i].line > 0 && pm[i].name[0] == 'M')
				now = i;

	}
	modify_user_mode(MMENU);
	R_monitor();
	while (1) {
		// printacbar();
		while (pm[now].level < 0 || !HAS_PERM(pm[now].level)) {
			now++;
			if (now >= size)
				now = 0;
		}
		move(pm[now].line, pm[now].col);
		prints("> ");
		move(pm[now].line, pm[now].col + 1);
		cmd = egetch();
		move(pm[now].line, pm[now].col);
		prints("  ");
		switch (cmd) {
		case EOF:
			if (!refscreen) {
				abort_bbs();
			}
			domenu_screen(pm);
			modify_user_mode(MMENU);
			R_monitor();
			break;
		case KEY_RIGHT:
			for (i = 0; i < size; i++) {
				if (pm[i].line == pm[now].line &&
				    pm[i].level >= 0 && pm[i].col > pm[now].col /* 右边有无菜单 */
				    && HAS_PERM(pm[i].level))
					break;
			}
			if (i < size) {
				now = i;
				break;
			}
		case '\n':
		case '\r':
			if (strcmp(pm[now].arg, "..") == 0) { /* 回到主选单 */
				return 0;
			}
			if (pm[now].fptr != NULL) {
				int type;

				(void *) sysconf_funcptr(pm[now].name, &type); /* 获取函数指针 */
				(*pm[now].fptr) (pm[now].arg);  /* 调用相应函数 */
				if (pm[now].fptr == Select) {
					now++;
				}
				domenu_screen(pm);
				modify_user_mode(MMENU);
				R_monitor();
			}
			break;
		case KEY_LEFT:
			for (i = 0; i < size; i++) {
				if (pm[i].line == pm[now].line &&
				    pm[i].level >= 0 && pm[i].col < pm[now].col /* 左边有无菜单项*/
				    && HAS_PERM(pm[i].level))
					break;
				if (pm[i].fptr == Goodbye)
					break;
			}
			if (i < size) {
				now = i;
				break;
			}
			return 0;
		case KEY_DOWN:
			now++;
			break;
		case KEY_UP:
			now--;
			while (pm[now].level < 0 || !HAS_PERM(pm[now].level)) {
				if (now > 0)
					now--;
				else
					now = size - 1;
			}
			break;
			// Modified by Flier - 2000.5.12 - Begin
		case KEY_PGUP:
			now = 0;
			break;
		case KEY_PGDN:
			now = size - 1;
			while (pm[now].level < 0 || !HAS_PERM(pm[now].level))
				now--;
			break;
			// Modified by Flier - 2000.5.12 - End
		case '~':
			if (!HAS_PERM(PERM_SYSOP) && !HAS_PERM(PERM_ACBOARD))
				break;

			free(menuitem);
			load_sysconf_image("sysconf.img", YEA);
			pm = &menuitem[sysconf_eval(menu_name)];
			endline_init();
			activeboard_init();
			size = domenu_screen(pm);
			now = 0;
			break;
		case '!':	/* youzi leave */
			if (strcmp("TOPMENU", menu_name) == 0)
				break;
			else
				return Goodbye();
		case Ctrl('V'):	/* monster: 快速锁屏 */
			x_lockscreen_silent();
			break;
		default:
			if (cmd >= 'a' && cmd <= 'z')
				cmd = cmd - 'a' + 'A';
			for (i = 0; i < size; i++) {
				// rovingcloud 2008.6.16
				// fix a bug - contradiction of shortcuts	
				if (pm[i].level < 0 || pm[i].line <= 0 || !HAS_PERM(pm[i].level))
					continue;
				control = 0;
				for (j = 0; pm[i].desc[j]; j++) {
					if (control) {
						// 过滤控制区域，直到遇到'm'结束
						if (pm[i].desc[j] == 'm')
							control = 0;
						continue;
					}
					if (pm[i].desc[j] == 27)  // 控制符
						control = 1;
					else if (pm[i].desc[j] != '(') {
						if (cmd == pm[i].desc[j])
							now = i;
						break;
					}					
				}
			}
		}
	}
}

