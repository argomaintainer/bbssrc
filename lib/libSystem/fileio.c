/*
 * fileio.c		-- some stuff that file/io related
 *
 * of SEEDNetBBS generation 1 (libtool implement)
 *
 * Copyright (c) 1999, Edward Ping-Da Chuang <edwardc@edwardc.dhs.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * CVS: $Id: fileio.c,v 1.9 2008-10-06 19:29:41 freestyler Exp $
 */

#ifdef BBS
#include "bbs.h"
#else
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "sysdep.h"
#endif

#define        BLK_SIZE         8192

int
file_append(char *fpath, char *msg)
{
	int fd, result;

	if ((fd = open(fpath, O_WRONLY | O_CREAT | O_APPEND, 0644)) > 0) {
		result = write(fd, msg, strlen(msg));
		close(fd);
		return result;
	}

	return -1;
}

int
file_appendfd(char *fpath, char *msg, int *fd)
{
	if (*fd <= 0) {
		if ((*fd = open(fpath, O_WRONLY | O_CREAT | O_APPEND, 0644)) <= 0)
			return -1;
	}

	return write(*fd, msg, strlen(msg));
}

int
file_appendline(char *fpath, char *msg)
{
	FILE *fp;
	int result;

	if ((fp = fopen(fpath, "a")) != NULL) {
		result = fprintf(fp, "%s\n", msg);
		fclose(fp);
		return (result == EOF) ? -1 : 1;
	}

	return -1;
}

int
dashf(char *fname)
{
	struct stat st;

	return (stat(fname, &st) == 0 && S_ISREG(st.st_mode));
}

int
dashd(char *fname)
{
	struct stat st;

	return (stat(fname, &st) == 0 && S_ISDIR(st.st_mode));
}

int
dash(char *fname)
{
	struct stat st;

	if (stat(fname, &st) == 0) {
		if (S_ISREG(st.st_mode))
			return 1;
		if (S_ISDIR(st.st_mode))
			return 2;
	}

	return 0;
}

int
part_cp(char *src, char *dst, char *mode)
{
	int flag = 0;
	char buf[256];
	FILE *fsrc, *fdst;

	fsrc = fopen(src, "r");
	if (fsrc == NULL)
		return 0;
	fdst = fopen(dst, mode);
	if (fdst == NULL) {
		fclose(fsrc);	/* add close fsrc by quickmouse 01/03/09 */
		return 0;
	}
	while (fgets(buf, 256, fsrc) != NULL) {
		if (flag == 1 && (buf[0] == '-' && buf[1] == '-' && buf[2] == '\n')) {
			fputs(buf, fdst);
			break;
		}
		if (flag == 0 && (!strncmp(buf + 2, "ÐÅÈË: ", 6)
				  || !strncmp(buf, "[1;41;33m·¢ÐÅÈË: ", 18))) {
			fputs(buf, fdst);
			continue;
		}
		if (flag == 0 &&
		    (buf[0] == '\0' || buf[0] == '\n' ||
		     !strncmp(buf + 2, "ÐÅÈË: ", 6)
		     || !strncmp(buf, "±ê  Ìâ: ", 8) ||
		     !strncmp(buf, "·¢ÐÅÕ¾: ", 8)
		     || !strncmp(buf, "[1;41;33m·¢ÐÅÈË: ", 18)))
			continue;
		flag = 1;
		fputs(buf, fdst);
	}
	fclose(fdst);
	fclose(fsrc);
	return 1;
}

#ifndef LINUX
/* mode == O_EXCL / O_APPEND / O_TRUNC */

int
f_cp(char *src, char *dst, int mode)
{
	int fsrc, fdst, ret;

	ret = 0;
	if ((fsrc = open(src, O_RDONLY)) >= 0) {
		ret = -1;
		if ((fdst = open(dst, O_WRONLY | O_CREAT | mode, 0644)) >= 0) {
			char pool[BLK_SIZE];

			src = pool;
			do {
				ret = read(fsrc, src, BLK_SIZE);
				if (ret <= 0)
					break;
			} while (write(fdst, src, ret) > 0);
			close(fdst);
		}
		close(fsrc);
	}
	return ret;
}
#else
int
f_cp(char *src, char *dst, int mode)
{
	int fsrc, fdst, ret;

	ret = 0;
	if ((fsrc = open(src, O_RDONLY)) >= 0) {
		ret = -1;
		if ((fdst = open(dst, O_WRONLY | O_CREAT | mode, 0644)) >= 0) {
			char pool[BLK_SIZE];

			src = pool;
			do {
				ret = read(fsrc, src, BLK_SIZE);
				if (ret <= 0)
					break;
			} while (write(fdst, src, ret) > 0);
			close(fdst);
		}
		close(fsrc);
	}
	return ret;
}

/*
  Presently (Linux 2.6.9): in_fd, must correspond to a  file  which  supports
  mmap(2)-like  operations (i.e., it cannot be a socket); and out_fd must refer
  to a socket.
  
#include <sys/sendfile.h>
#include <limits.h>

int
f_cp(char *src, char *dst, int mode)
{
	int fin, fout, ret;
	off_t offset = 0;

	if ((fin = open(src, O_RDONLY)) < 0)
		return -1;

	if ((fout = open(dst, O_WRONLY | O_CREAT | mode, 0644)) < 0)
		return -1;

	ret = sendfile(fout, fin, &offset, INT_MAX);

	close(fin);
	close(fout);

	return (ret < 0) ? (-1) : 0;
}
*/

#endif

int
f_mv(char *src, char *dst)
{
	if (rename(src, dst) == -1) {

		/* monster: only source and destination located on different
		   fs do we need to copy & unlink source */

		if (errno == EXDEV) {
			if (f_cp(src, dst, O_TRUNC) == -1 || unlink(src) == -1) {
				return -1;
			} else {
				return 0;
			}
		}
	} else {
		return 0;
	}
	return -1;
}

int
valid_fname(char *str)
{
	char ch;

	while ((ch = *str++) != '\0') {
		if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
		    strchr("0123456789-_", ch) != 0) {
			;
		} else {
			return 0;
		}
	}
	return 1;
}

static int
rm_dir(char *fpath)
{
	struct stat st;
	DIR *dirp;
	struct dirent *de;
	char buf[256], *fname;

	if (!(dirp = opendir(fpath)))
		return -1;

	for (fname = buf; (*fname = *fpath); fname++, fpath++) ;

	*fname++ = '/';

	while ((de = readdir(dirp))) {
		if (strcmp(de->d_name, ".") == 0 ||
		    strcmp(de->d_name, "..") == 0)
			continue;		/* ignore dot and dot-dot */
		
#ifdef SOLARIS
		fpath = (de->d_name) - 2;	/* replaced by quickmouse */
#else
		fpath = de->d_name;
#endif
		if (*fpath) {
			strcpy(fname, fpath);
			if (!stat(buf, &st)) {
				if (S_ISDIR(st.st_mode))
					rm_dir(buf);
				else
					unlink(buf);
			}
		}
	}
	closedir(dirp);

	*--fname = '\0';
	return rmdir(buf);
}

int
f_rm(char *fpath)
{
	struct stat st;

	if (lstat(fpath, &st))
		return -1;

	if (!S_ISDIR(st.st_mode))
		return unlink(fpath);

	if (strstr(fpath, "//") || strstr(fpath, "..") || strchr(fpath, ' '))
		return -1;	/* precaution */
	if (fpath[strlen(fpath) - 1] == '/')
		return -1;

	return rm_dir(fpath);
}

/* following code is extracted from source code of freebsd's mkdir(1) */
int
f_mkdir(char *path, int omode)
{
	struct stat sb;
	mode_t numask, oumask;
	int first, last, retval;
	char *p;

	p = path;
	oumask = 0;
	retval = 0;
	if (p[0] == '/')		/* Skip leading '/'. */
		++p;
	for (first = 1, last = 0; !last ; ++p) {
		if (p[0] == '\0')
			last = 1;
		else if (p[0] != '/')
			continue;
		*p = '\0';
		if (p[1] == '\0')
			last = 1;
		if (first) {
			/*
			 * POSIX 1003.2:
			 * For each dir operand that does not name an existing
			 * directory, effects equivalent to those cased by the
			 * following command shall occcur:
			 *
			 * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
			 *    mkdir [-m mode] dir
			 *
			 * We change the user's umask and then restore it,
			 * instead of doing chmod's.
			 */
			oumask = umask(0);
			numask = oumask & ~(S_IWUSR | S_IXUSR);
			umask(numask);
			first = 0;
		}
		if (last) umask(oumask);
		if (mkdir(path, last ? omode : S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
			if (errno == EEXIST || errno == EISDIR) {
				if (stat(path, &sb) < 0) {
					retval = -1;
					break;
				} else if (!S_ISDIR(sb.st_mode)) {
					if (last) {
						errno = EEXIST;
						retval = 0; /* ignore if target directory exists */
					} else {
						errno = ENOTDIR;
						retval = -1;
					}
					break;
				}
			} else {
				retval = -1;
				break;
			}
		}
		if (!last)
		    *p = '/';
	}
	if (!first && !last)
		umask(oumask);
	return retval;
}

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
f_unlock(int fd)
{
	fl.l_type = F_UNLCK;
	return fcntl(fd, F_SETLKW, &fl);
}

int
filelock(char *fname, int block)
{
	int fd;

	if ((fd = open(fname, O_RDWR | O_CREAT | O_APPEND, 0644)) == -1)
		return -1;

	if (flock(fd, LOCK_EX | ((block) ? 0 : LOCK_NB)) == -1) {
		close(fd);
		return -1;
	}
	return fd;
}

void
fileunlock(int fd)
{
	flock(fd, LOCK_UN);
	close(fd);
}

int
seek_in_file(char *filename, char *seekstr)
{
	FILE *fp;
	char buf[80];
	char *namep;

	if ((fp = fopen(filename, "r")) == NULL)
		return 0;
	while (fgets(buf, 80, fp) != NULL) {
		namep = (char *) strtok(buf, ": \n\r\t");
		if (namep != NULL && strcasecmp(namep, seekstr) == 0) {
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

