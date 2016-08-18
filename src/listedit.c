#include "bbs.h"

static int col[3] = { 2, 24, 48 };
static int ccol[3] = { 0, 22, 46 };
static int pagetotal;

void
listedit_redraw(char *title, slist *list, int current, int pagestart, int pageend)
{
	int i, j, len1, len2;

	len1 = 39 - strlen(BoardName) / 2;
	len2 = len1 - strlen(title);

	clear();
	move(0, 0);
	prints("\033[1;33;44m%s\033[1;37m%*s%s%*s\033[m\n", title, len2, " ", BoardName, len1, " ");
	prints("确定[\033[1;32mRtn\033[m] 选择[\033[1;32m←\033[m,\033[1;32m→\033[m,\033[1;32m↑\033[m,\033[1;32m↓\033[m] 增加[\033[1;32ma\033[m] 删除[\033[1;32md\033[m] 引入/导出名单[\033[1;32mI/E\033[m] 清除名单[\033[1;32mC\033[m] \n");
	prints("\033[1;37;44m  名  单                名  单                  名  单                        \033[m\n");

	for (i = pagestart, j = 3; i < pageend && i < list->length; i++) {
		move(j, col[i % 3]);
		outs(list->strs[i]);
		if (i % 3 == 2) ++j;
	}

	update_endline();
}

int
show_idlistfiles(int baseindex, int limit)
{
	FILE *fp;
	char fname[PATH_MAX + 1], desc[TITLELEN + 1];

	sethomefile(fname, currentuser.userid, "idlist");
	if ((fp = fopen(fname, "r")) == NULL)
		return baseindex;

	while (baseindex < limit && fgets(desc, sizeof(desc), fp)) {
		prints("[\033[1;32m%d\033[m] %s", baseindex, desc);
		++baseindex;
	}

	return baseindex;
}

void
listedit_import(slist *list, int (*callback)(int action, slist *list, char *ident))
{
	int i, listnum;
	FILE *fp;
	char ch[2], uident[IDLEN + 2], fname[PATH_MAX + 1];

	stand_title("引入名单");
	move(2, 0);
	prints("[\033[1;32m1\033[m] 好友列表\n");
	prints("[\033[1;32m2\033[m] 坏人列表\n");
	listnum = show_idlistfiles(3, 3 + MAX_IDLIST) - 3;
	getdata(5 + listnum, 0, "你想把上述哪个列表引入名单: ", ch, 2, DOECHO, YEA);

	switch (ch[0]) {
	case '1':
		for (i = 0; i < MAXFRIENDS; i++) {
			if (uinfo.friends[i] <= 0)
				continue;
			getuserid(uident, uinfo.friends[i]);

			if (uident[0] != '\0' && slist_indexof(list, uident) == -1)
				if (callback == NULL || callback(LE_ADD, list, uident) == YEA)
					slist_add(list, uident);			
		}
		break;
	case '2':
		for (i = 0; i < MAXREJECTS; i++) {
			if (uinfo.reject[i] <= 0)
				continue;
			getuserid(uident, uinfo.reject[i]);

			if (uident[0] != '\0' && slist_indexof(list, uident) == -1)
				if (callback == NULL || callback(LE_ADD, list, uident) == YEA)
					slist_add(list, uident);			
		}
		break;
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		ch[0] -= '3';
		if (ch[0] >= listnum)
			break;

		snprintf(fname, sizeof(fname), "home/%c/%s/idlist.00%d", mytoupper(currentuser.userid[0]), 
			 currentuser.userid, ch[0]);
		if ((fp = fopen(fname, "r")) == NULL)
			break;

		while (fgets(uident, sizeof(uident), fp)) {
			char *ptr;

			if ((ptr = strrchr(uident, '\n')) != NULL)
				*ptr = '\0';

			if (getuser(uident, NULL) > 0 && slist_indexof(list, uident) == -1)
				if (callback == NULL || callback(LE_ADD, list, uident) == YEA)
					slist_add(list, uident);
		}

		fclose(fp);
	}
}

void
listedit_export(slist *list)
{
	int i = 0, listnum;
	FILE *fp, *fp2;
	char ch[2], desc[TITLELEN + 1];
	char fname[PATH_MAX + 1], fname2[PATH_MAX + 1], fname3[PATH_MAX + 1];

	stand_title("导出名单");
	while (1) {
		move(2, 0);
		clrtobot();
		listnum = show_idlistfiles(1, 1 + MAX_IDLIST) - 1;
		prints("[\033[1;32mA\033[m] 增加列表\n");
		prints("[\033[1;32mD\033[m] 删除列表\n");
input:
		getdata(5 + listnum, 0, "你想把名单导出至哪个列表: ", ch, 2, DOECHO, YEA);
		switch (ch[0]) {
			case 'A':
			case 'a':
				if (listnum >= MAX_IDLIST)
					goto input;

				getdata(6 + listnum, 0, "列表名称: ", desc, sizeof(desc), DOECHO, YEA);				
				if (desc[0] == '\0') {
					strlcpy(desc, "未命名列表", sizeof(desc));
				}
				
				sethomefile(fname, currentuser.userid, "idlist");
				if ((fp = fopen(fname, "a+")) == NULL)
					break;

				fprintf(fp, "%s\n", desc); 
				fclose(fp);

				if (askyn("是否把当前名单导入该列表", YEA, NA) == YEA) {
					snprintf(fname, sizeof(fname), "home/%c/%s/idlist.00%d", mytoupper(currentuser.userid[0]), 
						 currentuser.userid, listnum);
					slist_savetofile(list, fname);
					return;
				}
				break;
			case 'D':
			case 'd':
				if (listnum <= 0)
					goto input;

				getdata(6 + listnum, 0, "列表编号: ", ch, 2, DOECHO, YEA);
				if (ch[0] <= '0')
					break;
				ch[0] -= '1';
				if (ch[0] >= listnum)
					break;

				sethomefile(fname, currentuser.userid, "idlist");
				sethomefile(fname2, currentuser.userid, "idlist2");

				if (listnum == 1) {
					unlink(fname);
					break;
				}

				if ((fp = fopen(fname, "r")) == NULL)
					break;
				unlink(fname2);
				if ((fp2 = fopen(fname2, "w")) == NULL) {
					fclose(fp);
					break;
				}
				
				i = 0;
				while (fgets(desc, sizeof(desc), fp)) {
					if (i == ch[0]) {
						snprintf(fname3, sizeof(fname3), "home/%c/%s/idlist.00%d", mytoupper(currentuser.userid[0]), 
			 				 currentuser.userid, ch[0]);
						unlink(fname3);
					} else {
						fputs(desc, fp2);
					}
					++i;
				}
				fclose(fp);
				fclose(fp2);
				rename(fname2, fname);
				break;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				ch[0] -= '1';
				if (ch[0] > listnum)
					goto input;

				snprintf(fname, sizeof(fname), "home/%c/%s/idlist.00%d", mytoupper(currentuser.userid[0]), 
					 currentuser.userid, ch[0]);
				slist_savetofile(list, fname);
				return;
			default:	
				return;
		}
	}
}


int
listedit(char *listfile, char *title, int (*callback)(int action, slist *list, char *uident))
{
	slist *list;
	int current = 0, i, len, pagestart, pageend, oldx, oldy;
	char uident[IDLEN + 2];

	if ((list = slist_init()) == NULL)
		return 0;
	slist_loadfromfile(list, listfile);

	oldy = 3;
	oldx = 0;
	pagetotal = (t_lines - 4) * 3;
	pagestart = 0;
	pageend = pagetotal - 1;
	listedit_redraw(title, list, current, pagestart, pageend);
	if (list->length > 0) {
		move(3, 0);
		outc('>');
	}

	while (1) {
		switch (egetch()) {
		case 'h':
			show_help("help/listedithelp");
			listedit_redraw(title, list, current, pagestart, pageend);
			break;
		case '\n':
			goto out;
		case 'a':
			clear_line(1);
			prints("使用者代号: ");
			usercomplete(NULL, uident);
			if (uident[0] != '\0' && slist_indexof(list, uident) == -1)
				if (callback == NULL || callback(LE_ADD, list, uident) == YEA)
					slist_add(list, uident);
			if (list->length == 1)
				current = 0;
			goto redraw;
		case 'd':
		case KEY_DEL:
			clear_line(t_lines - 1);
			snprintf(genbuf, sizeof(genbuf), "从名单中移除 %s 吗", list->strs[current]);
			if (askyn(genbuf, NA, NA) == NA) {
				update_endline();
				goto unchanged;
			}
			if (callback == NULL || callback(LE_REMOVE, list, list->strs[current]) == YEA) {
				slist_remove(list, current);
				if (current >= list->length && current > 0)
					--current;
			}
			goto redraw;
		case 'I':
			listedit_import(list, callback);
			goto redraw;
		case 'E':
			listedit_export(list);
			goto redraw;
		case 'C':
			clear_line(t_lines - 1);
			if (askyn("确认清除名单吗", NA, NA) == NA) {
				update_endline();
				goto unchanged;
			}
			if (callback != NULL) {
				for (i = 0; i < list->length; i++)
					callback(LE_REMOVE, list, list->strs[i]);
			}
			slist_clear(list);	
			current = 0;
			goto redraw;				
		case KEY_HOME:
			if (list->length == 0 || current == 0)
				goto unchanged;
			current = 0;
			break;
		case KEY_END:
			if (list->length == 0 || current == list->length - 1)
				goto unchanged;
			current = list->length - 1;
			break;
		case KEY_LEFT:
			if (current == 0)
				goto unchanged;
			current--;
			break;
		case KEY_RIGHT:
			if (current == list->length - 1)
				goto unchanged;
			current++;
			break;
		case KEY_UP:
			if (current < 3)
				goto unchanged;
			current -= 3;
			break;
		case KEY_DOWN:
			if (current >= list->length - 3)
				goto unchanged;
			current += 3;
			break;
		case KEY_PGUP:
			if (current < pagetotal)
				goto unchanged;
			current -= pagetotal;
		case KEY_PGDN:
			if (current >= list->length - pagetotal)
				goto unchanged;
			current += pagetotal;
			break;
		default:
			goto unchanged;
		}

		if (current - current % pagetotal != pagestart || current - current % pagetotal + pagetotal != pageend) {
redraw:
			pagestart = current - current % pagetotal;
			pageend = current - current % pagetotal + pagetotal;
			listedit_redraw(title, list, current, pagestart, pageend);
		} else {
			move(oldy, oldx);
			outc(' ');
		}

		if (list->length > 0) {
			oldy = 3 + (current - pagestart) / 3;
			oldx = ccol[current % 3];
			move(oldy, oldx);
			outc('>');
		}
unchanged: ;
	}

out:
	clear();
	len = list->length;
	slist_savetofile(list, listfile);
	slist_free(list);
	return len;
}
