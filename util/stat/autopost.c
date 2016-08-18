#include "bbs.h"
#include "libBBS.h"
#define FILE_FLAG (FILE_MARKED + FILE_NOREPLY)

char datestring[STRLEN];

int
main()
{
	FILE *check;
	char notetitle[STRLEN];
	char tmp[STRLEN * 2];
	char *fname, *bname, *ntitle;

	chdir(BBSHOME);
	if ((check = fopen("etc/autopost", "r")) != NULL) {
		while (fgets(tmp, STRLEN, check) != NULL) {
			fname = strtok(tmp, " \n\t:@");
			bname = strtok(NULL, " \n\t:@");
			ntitle = strtok(NULL, " \n\t:@");
			if (fname == NULL || bname == NULL || ntitle == NULL)
				continue;
			else {
				getdatestring_now(datestring);
				sprintf(notetitle, "[%8.8s %6.6s] %s",
					datestring + 6, datestring + 23,
					ntitle);
				if (dashf(fname)) {
					postfile(fname, bname, notetitle, BBSID,
						 FILE_FLAG);
				}
			}
		}
		fclose(check);
	}
	getdatestring_now(datestring);
	sprintf(notetitle, "[%s] ÁôÑÔ°å¼ÇÂ¼", datestring);
	if (dashf("etc/notepad")) {
		postfile("etc/notepad", "notepad", notetitle, BBSID, FILE_FLAG);
		unlink("etc/notepad");
	}
	return 0;
}
