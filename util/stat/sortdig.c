#include "bbs.h"

static struct flock fl = {
	l_whence:SEEK_SET,
	l_start:0,
	l_len:0,
};

int
f_exlock(int fd)
{
	fl.l_type = F_WRLCK;
	return fcntl(fd, F_SETLKW, &fl);
}

int
sort_records(char *filename, int size, int (*compare)(const void *, const void *))
{
	int fd, result = 0;
	char *buf;
	struct stat st;

	if (size <= 0 || compare == NULL)
		return -1;

	if ((fd = open(filename, O_RDWR, 0)) == -1)
		return -1;

	if (fstat(fd, &st) == -1) {
		close(fd);
		return -1;
	}

	f_exlock(fd);
	buf = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	if (buf == NULL || st.st_size <= 0 || st.st_size % size != 0) {
		close(fd);
		return -1;
	}
	qsort(buf, st.st_size / size, size, compare);
	munmap(buf, st.st_size);
	close(fd);
	return result;
}

int
compare_filetime(const void *f1, const void *f2)
{
	const struct fileheader *fh1 = f1;
	const struct fileheader *fh2 = f2;
	return fh1->filetime - fh2->filetime;
}


#undef snprintf

int
main(int argc, char **argv)
{
	char filename[256];

	if (argc < 2) {
		printf("Usage: sortdig board-name\n");
		return -1;
	}
	snprintf(filename, sizeof(filename), BBSHOME"/boards/%s/.DIGEST", argv[1]);
		
	return sort_records(filename, sizeof(struct fileheader), compare_filetime);
}
