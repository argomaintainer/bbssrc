#include "bbs.h"

#define PATHLEN 250

char logpath[STRLEN];

void
init_historydir()
{
	time_t now;
	struct tm t;

	now = time(0);
	localtime_r(&now, &t);
	sprintf(logpath, "log_history/%d-%d-%d", t.tm_year + 1900, t.tm_mon + 1,
		t.tm_mday);
	mkdir("log_history", 511);
	mkdir(logpath, 511);
}

void
move_file(const char *filename)
{
	char src[PATHLEN], dst[PATHLEN];

	sprintf(src, "reclog/%s", filename);
	sprintf(dst, "%s/%s", logpath, filename);
	rename(src, dst);
}

void
remove_file(const char *filename)
{
	char fname[PATHLEN];

	sprintf(fname, "reclog/%s", filename);
	unlink(fname);
}

int
main()
{
	init_historydir();

	move_file("bbsnet.log");
	move_file("logins.bad");
	move_file("mail-log");
	move_file("pop3d.log");
	move_file("trace");
	move_file("uptime.log");
	move_file("use_board");
	move_file("usies");

	remove_file("usage.history");
	remove_file("usage.last");
}
