#include "bbs.h"
#include "libBBS.h"

char genbuf[1024];
extern struct UCACHE *uidshm;

#define KILL_LOG	BBSHOME"/reclog/autokill.log"

int
compute_user_value(struct userec *urec)
{
	int value;

	if (urec->userlevel & PERM_XEMPT || !strcmp(urec->userid, "guest") || !strcmp(urec->userid, "SYSOP"))
		return 999;

	if (urec->numlogins == 0)
		return -1;

	value = (time(NULL) - urec->lastlogin) / 60;

	if (urec->userlevel & PERM_SUICIDE) {
		value = (3 * 1440 - value) / 1440;      /* monster: 自杀者最多拥有三点生命力 */
	} else if (urec->numlogins <= 3 && !(urec->userlevel & PERM_WELCOME)) {
		value = (15 * 1440 - value) / 1440;
	} else if (!(urec->userlevel & PERM_LOGINOK)) {
		value = (30 * 1440 - value) / 1440;
	} else if (urec->stay > 1000000) {
		value = (365 * 1440 - value) / 1440;
	} else {
		value = (120 * 1440 - value) / 1440;
	}

	if (value >= 0)
		return value;

	if (urec->userlevel & PERM_BOARDS)      /* monster: 避免版主在任期间死亡 */
		return 0;
	else
		return (-1);
}

/* monster: 删除用户个人与邮件目录 */
void
clear_userdir(char *userid, struct tm *time_result)
{
	char tmpstr[256];
	char dststr[256];

	if (userid == NULL)
		return;

#ifndef ALLOW_CHINESE_ID
	if (!isalpha(userid[0]))
		return;
#endif

	sprintf(tmpstr, "mail/%c/%s", mytoupper(userid[0]), userid);
	sprintf(dststr, "suicide/dead/%s.%d-%d-%d.mail", userid, time_result->tm_year, time_result->tm_mon, time_result->tm_mday);
	if(f_mv(tmpstr, dststr) == -1)
		f_rm(tmpstr);
	sprintf(tmpstr, "home/%c/%s", mytoupper(userid[0]), userid);
	sprintf(dststr, "suicide/dead/%s.%d-%d-%d.home", userid, time_result->tm_year, time_result->tm_mon, time_result->tm_mday);
	if(f_mv(tmpstr, dststr) == -1)
		f_rm(tmpstr);
}

/*
 * void
 * touchnew()
 * {
 *	sprintf(genbuf, "touch by: %d\n", time(0));
 *	file_append(FLUSH, genbuf);
 * }
 */

void
touchnew()
{
	resolve_ucache();
	uidshm->uptime = 0;
}

int
main()
{
	struct userec utmp;
	struct userec zerorec;
	int size = sizeof(struct userec);
	struct stat st;
	int fd, val, i;
	FILE *flog;
	time_t now;
	char times[256];
	struct tm *time_result;	

	chdir(BBSHOME);
	memset(&zerorec, 0, sizeof (zerorec));

	now = time(NULL);
	strlcpy(times, ctime(&now), sizeof(times));
	times[strlen(times) - 1] = '\0';
	time_result = gmtime(&now);

	flog = fopen(KILL_LOG, "a");
	if (!flog) return 0;

	if ((fd = open(PASSFILE, O_RDWR | O_CREAT, 0600)) == -1)
		return 0;
	f_exlock(fd);
	size = sizeof (struct userec);
	for (i = 0; i < MAXUSERS; i++) {
		if (read(fd, &utmp, size) != size)
			break;
		val = compute_user_value(&utmp);
		if (utmp.userid[0] != '\0' && val < 0) {
			/* monster: 发表临别赠言 */
			sprintf(genbuf, "suicide/suicide.%s", utmp.userid);
			if (stat(genbuf, &st) != -1) {
				char title[STRLEN];

				sprintf(title, "%s 的临别赠言", utmp.userid);
				postfile(genbuf, "ID", title, utmp.userid, 0);
				sleep(1);
				unlink(genbuf);
			}
			clear_userdir(utmp.userid, time_result);

			if (lseek(fd, (off_t) (i * size), SEEK_SET) == -1) {
				f_unlock(fd);
				close(fd);
				return;
			}
			write(fd, &zerorec, sizeof (zerorec));

			/* Append log */
			fprintf(flog, "%s\tKilled %s\n", times, utmp.userid);
		}
	}
	f_unlock(fd);
	close(fd);
	fclose(flog);
	touchnew();
}
