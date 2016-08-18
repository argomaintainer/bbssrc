/** postfile.c
 * This is a small util to post a file to BBS
 * with standard header
 * means to easist BBS applications using script language like Perl and Python
 */

#include "bbs.h"
#include "libBBS.h"

int
main(int argc, char **argv)
{
	if (argc < 5) {
		fprintf(stderr, "Usage: %s title to-board username nick\n", argv[0]);
		return -1;
	}

	FILE *fout;
	char fname[256];

	chdir(BBSHOME);
	snprintf(fname, sizeof(fname), "postfile.tmp.%d", getpid());
	fout = fopen(fname, "w");
	if (!fout) return -1;

	struct tm *tv;
	time_t now;
	char timebuf[256];
	char buf[255];
	now = time(NULL);
	tv = localtime(&now);
	strftime(timebuf, sizeof(timebuf), "%a %b %e %T %Y", tv);

	fprintf(fout, "发信人: %s (%s), 信区: %s\n", argv[3], argv[4], argv[2]);
	fprintf(fout, "标  题: %s\n", argv[1]);
	fprintf(fout, "发信站: %s (%s), 自动发信\n", BBSNAME, timebuf);
	fprintf(fout, "\n");

	while (fgets(buf, sizeof(buf), stdin))
		fprintf(fout, "%s", buf);

	fprintf(fout, "\n--\n\033[m\033[1;31m※ 来源:．%s %s．[FROM: postfile.c]\033[m\n",
		BBSNAME, BBSHOST);	
	fclose(fout);

	postfile(fname, argv[2], argv[1], argv[3], FILE_MARKED);
	unlink(fname);
	
	return 0;
}
