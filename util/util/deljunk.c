#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bbs.h"

int
process(char *fname, int top)
{
	int fd, fd2, numents;
	int size = sizeof (struct fileheader);
	struct stat st;
	struct fileheader header;
	int count = 0;
	int limit;
	char fname2[80], fname3[80];

	lstat(fname, &st);
	numents = ((long) st.st_size) / size;
	if (numents <= top)
		return 1;
	limit = numents - top;

	if ((fd = open(fname, O_RDWR)) == -1)
		return -1;
	if (top > 0) {
		sprintf(fname2, "%s.deljunk_%d.tmp", fname);
		if ((fd2 = open(fname2, O_WRONLY | O_CREAT, 0644)) == -1)
			return -2;
		flock(fd2, LOCK_EX);
	}

	while (read(fd, &header, size) == size) {
		count++;
		if (count < limit) {
			if (!(header.flag & FILE_FORWARDED)) {
				unlink(header.filename);
			}
		} else {
			if (!(header.flag & FILE_FORWARDED) && header.filename[0] != '\0' && header.owner[0] != '\0') {
				if (top > 0)
					write(fd2, &header, size);
			}
		}
	}

	if (top > 0) {
		flock(fd2, LOCK_UN);
		close(fd2);
	}
	close(fd);

	if (top > 0) {
		sprintf(fname3, "deljunk_%d.tmp", getpid());
		rename(fname, fname3);
		rename(fname2, fname);
		unlink(fname3);
	} else {
		unlink(fname);
	}

	return 0;
}

int
remove_board(char *board)
{
	char path[250];
	DIR *dp;
	struct dirent *dirp;

	printf("Cleaning %s...\n", board);
	sprintf(path, "%s/boards/%s/", BBSHOME, board);
	if (chdir(path))
		return -1;

	process(".JUNK", 1000);
	process(".DELETED", 2500);

	if ((dp = opendir(".")) == NULL)
		return -2;

	while ((dirp = readdir(dp)) != NULL)
		if (!strncmp(dirp->d_name, ".removing.", 10))
			process(dirp->d_name, 0);

	return 0;
}

int
walk()
{
	char path[250];
	DIR *dp;
	struct dirent *dirp;

	sprintf(path, "%s/boards/", BBSHOME);
	if (chdir(path))
		return -1;

	if ((dp = opendir(".")) == NULL)
		return -2;

	while ((dirp = readdir(dp)) != NULL)
		if (dirp->d_name[0] != '.')
			remove_board(dirp->d_name);

	return 0;
}

int
clean_syssecurity()
{
	char path[250];

	sprintf(path, "%s/boards/syssecurity/", BBSHOME);
	if (chdir(path))
		return -1;

	return process(".DIR", 25000);
}

int
main(int argc, char **argv)
{
	int i;

	if (argc > 1) {
		for (i = 1; i < argc; i++)
			remove_board(argv[i]);
	} else {
		walk();
		clean_syssecurity();
	}

	return 0;
}
