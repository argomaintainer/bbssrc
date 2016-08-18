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

#ifndef BBSMAIN

sigjmp_buf jmpbuf;

void
sigfault(int signo)
{
        /* recover from memory violation */
        siglongjmp(jmpbuf, signo);
}
 
static void
report(char *fmt, ...)
{
}

#endif

int
safewrite(int fd, void *buf, int size)
{
	int cc, sz = size, origsz = size;

	do {
		cc = write(fd, buf, sz);
		if ((cc < 0) && (errno != EINTR)) {
			report("safewrite: write error");
			return -1;
		}
		if (cc > 0) {
			buf += cc;
			sz -= cc;
		}
	} while (sz > 0);

	return origsz;
}

/* 返回filename里有多少个record, 每个record的大小是size */
int
get_num_records(char *filename, int size)
{
	struct stat st;

	if (stat(filename, &st) == -1)
		return 0;
	return (st.st_size / size);
}

int
append_record(char *filename, void *record, int size)
{
	int fd, err = 0;

	if ((fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1) {
		report("append_record: cannot open %s", filename);
		return -1;
	}
	f_exlock(fd);
	if (safewrite(fd, record, size) == -1) {
		report("append_record: cannot append record to %s", filename);
		err = -1;
	}
	close(fd);
	return err;
}

/* monster: fptr should not modify pointer specified by first parameter */
int
apply_record(char *filename, int (*fptr)(void *, int), int size)
{
	void *buf, *buf1;
	int fd, i = 0, id = 0;
	struct stat st;

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0 || st.st_size < 0) {
		close(fd);
		return -1;
	}

	if (st.st_size == 0) {
		close(fd);
		return 0;
	}

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);
	if (buf == MAP_FAILED) {
		return -1;
        }

	buf1 = buf;
	TRY
		while (i < st.st_size) {
			if ((*fptr) (buf1, ++id) == QUIT) {
				BREAK;
				munmap(buf, st.st_size);
				return QUIT;
			}
			i += size;
			buf1 += size;
		}
	END 

	munmap(buf, st.st_size);
	return 0;
}

int
search_record_forward(char *filename, void *rptr, int size, int start, int (*fptr)(void *, void *), void *farg)
{
	int fd, id = 1;
	void *buf, *buf1, *buf2;
	struct stat st;

	if (start <= 0)
		return 0; 

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return 0;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return 0;
	}

	buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
	close(fd);
	if (buf == MAP_FAILED || st.st_size <= 0) {
		return 0;
	}

	buf1 = buf + (start - 1) * size;
	buf2 = buf + st.st_size;

	TRY
		while (buf1 < buf2) {
			if ((*fptr) (farg, buf1)) {
				memcpy(rptr, buf1, size);
				BREAK;
				munmap(buf, st.st_size);
				return id;
			}
			buf1 += size;
			id++;
		}
	END

	munmap(buf, st.st_size);
	return 0;
}

/* write by babydragon. rewrite by betterman */
int search_record_bin(char *filename, void *rptr, int size, int start, int (*fptr)(void *, void *), void *farg)
{
        int fd;
        void *buf;
	int left, middle, right, ret;
        struct stat st;

        if (start <= 0)
                return 0;

        if ((fd = open(filename, O_RDONLY, 0)) == -1)
                return 0;

        if (fstat(fd, &st) < 0) {
                close(fd);
                return 0;
        }

        buf = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0);
        close(fd);
        if (buf == MAP_FAILED || st.st_size <= 0) {
                return 0;
        }
	
	left = start - 1;
	right = st.st_size / size;

        TRY
		while (left <= right) {
			middle = left + (right - left) / 2;
			ret = (*fptr)(farg, buf + size * middle);
			if (ret == 0) {
				memcpy(rptr, buf + size * middle, size);
				BREAK;
				munmap(buf, st.st_size);
				return middle + 1;
			} else if (ret > 0){
				left = middle + 1;
			} else {
				right = middle - 1;
			}
		}
        END

        munmap(buf, st.st_size);
        return 0;

}

int
move_record(char *filename, int size, int srcid, int dstid)
{
	int fd, result = 0;
	char *buf, tmprec[size];
	struct stat st;

	if (srcid == dstid)
		return 0;

	if (srcid <= 0 || dstid <= 0 || size <= 0)
		return -1;

	if ((fd = open(filename, O_RDWR, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0 || srcid * size > st.st_size || dstid * size > st.st_size) {
		close(fd);
		return -1;
	}

	f_exlock(fd);
	buf = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	if (buf == MAP_FAILED || st.st_size <= 0) {
		return -1;
	}

	TRY
		memmove(tmprec, buf + size * (srcid - 1), size);
		if (srcid > dstid) {
			memmove(buf + size * dstid, buf + size * (dstid - 1), (srcid - dstid) * size);
		} else {
			memmove(buf + size * (srcid - 1), buf + size * srcid, (dstid - srcid) * size);
		}
		memmove(buf + size * (dstid - 1), tmprec, size);
	CATCH
		result = -1;
	END

	munmap(buf, st.st_size);
	close(fd);
	return 0;
}

int
delete_record(char *filename, int size, int id)
{
	int fd, result = 0;
	char *buf;
	struct stat st;

	if (id <= 0 || size <= 0)
		return -1;

	if ((fd = open(filename, O_RDWR, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return -1;
	}

	f_exlock(fd);
	buf = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	if (buf == MAP_FAILED || st.st_size <= 0 || id * size > st.st_size) {
		close(fd);
		return -1;
	}

	if (id * size < st.st_size) {
		TRY
			memmove(buf + size * (id - 1), buf + size * id, st.st_size - size * id);
		CATCH
			result = -1;
		END
	}

	munmap(buf, st.st_size);
	ftruncate(fd, size * (st.st_size / size - 1));
	close(fd);
	return result;
}

int
process_records(char *filename, int size, int id1, int id2, int (*filecheck)(void *rptr, void *extrarg), void *extrarg)
{
	int fd, endpoint, rcount = 0, result = 0;
	struct stat st;
	void *buf, *fhdr, *lasthdr, *preservation;

	if (id1 <= 0 || id2 < id1)
		return -1;

	if ((fd = open(filename, O_RDWR, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return -1;
	}
	f_exlock(fd);

	/* validate the start point and end point*/
	if (id1 * size > st.st_size) {
		close(fd);
		return -1;
	}

	if (id2 * size > st.st_size) {
		endpoint = st.st_size;
	} else {
		endpoint = size * id2;
	}

	buf = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	if (buf == MAP_FAILED || st.st_size <= 0) {
		return -1;
	}

	preservation = fhdr = buf + (id1 - 1) * size;
	lasthdr = buf + endpoint;

	TRY
		while (fhdr < lasthdr) {
			if (filecheck(fhdr, extrarg) == KEEPRECORD) {
				if (fhdr != preservation) {
					memmove(preservation, fhdr, size);
				}
				preservation += size;
			} else {
				++rcount;
			}
			fhdr += size;
		}

		if (lasthdr != (buf + st.st_size)) {
			memmove(preservation, fhdr, (st.st_size / size - id2) * size);
		}
	CATCH
		result = -1;
	END

	munmap(buf, st.st_size);
	if (rcount > 0) ftruncate(fd, st.st_size - rcount * size);
	close(fd);
	return result;
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
	if (buf == MAP_FAILED || st.st_size <= 0 || st.st_size % size != 0) {
		close(fd);
		return -1;
	}

	TRY
		qsort(buf, st.st_size / size, size, compare);
	CATCH
		result = -1;
	END

	munmap(buf, st.st_size);
	close(fd);
	return result;
}

int
get_record(char *filename, void *rptr, int size, int id)
{
	int fd;

	if ((fd = open(filename, O_RDONLY, 0)) == -1) {
		return -1;
	}
	if (lseek(fd, (off_t) (size * (id - 1)), SEEK_SET) == -1) {
		close(fd);
		return -1;
	}
	if (read(fd, rptr, size) != size) {
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

/* 从filename里, 从第id(下标1开始)个开始, 读取number个record到rptr里
 * 每个record的大小是size */
int
get_records(char *filename, void *rptr, int size, int id, int number)
{
	int fd;
	int n;

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;
	if (lseek(fd, (off_t) (size * (id - 1)), SEEK_SET) == -1) {
		close(fd);
		return 0;
	}
	if ((n = read(fd, rptr, size * number)) == -1) {
		close(fd);
		return -1;
	}
	close(fd);
	return (n / size);
}

int
substitute_record(char *filename, void *rptr, int size, int id)
{
	int fd, err = 0;

	if (id < 1)
		return -1;

	if ((fd = open(filename, O_WRONLY | O_CREAT, 0644)) == -1)
		return -1;

	f_exlock(fd);
	if (lseek(fd, (off_t) (size * (id - 1)), SEEK_SET) == -1) {
		report("substitue_record: seek error in %s, id %d", filename, id);
		err = -1;
	} else if (safewrite(fd, rptr, size) != size) {
		report("substitue_record: cannot substitue record in %s, id %d", filename, id);
		err = -1;
	}
	close(fd);

	return err;
}

#ifdef BBSMAIN

/* monster: an optimized version of safe_substitute_record */
int
safe_substitute_record(char *direct, struct fileheader *fhdr, int ent, int sorted)
{
	int i, fd, result = 0, records, high, low;
	struct stat st;
	struct fileheader *headers;

	if ((fd = open(direct, O_RDWR, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return -1;
	}

	f_exlock(fd);
	headers = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	if (headers == MAP_FAILED || st.st_size <= 0) {
		close(fd);
		return -1;
	}

	records = st.st_size / sizeof(struct fileheader);
	if (ent >= records) ent = records;

	TRY
		if (sorted) {
			/* common case: headers[ent - 1] is what we look for */
			if (headers[ent - 1].filetime == fhdr->filetime && !strcmp(headers[ent - 1].filename, fhdr->filename)) {
				memcpy(&headers[ent - 1], fhdr, sizeof(struct fileheader));
				goto done;
			}

			/* freestyler: binary search variant lower_bound */
			for(low = 0, high = records; low < high; ) {
				i = (high + low) >> 1;
				if (fhdr->filetime <= headers[i].filetime) 
					high = i;
				else 
					low = i + 1;
			}
			// now low is the least i for which p(i) is true,
			// where p(i):= ( headers[i].filetime >= fhdr->filetime )

			for (i = low; i < records; i++) {
				if (headers[i].filetime != fhdr->filetime) {
					result = -1;
					break;
				}

				if (!strcmp(headers[i].filename, fhdr->filename)) {
					memcpy(&headers[i], fhdr, sizeof(struct fileheader));
					break;
				}
			}
		} else {
			/* 2-passes of linear search */
			for (i = ent - 1; i >= 0; i--) {
				if (headers[i].filetime == fhdr->filetime && !strcmp(headers[i].filename, fhdr->filename)) {
					memcpy(&headers[i], fhdr, sizeof(struct fileheader));
					goto done;
				}
			}

			for (i = ent; i < records; i++) {
				if (headers[i].filetime == fhdr->filetime && !strcmp(headers[i].filename, fhdr->filename)) {
					memcpy(&headers[i], fhdr, sizeof(struct fileheader));
					break;
				}
			}
		}
	CATCH
		result = -1;
	END

done:
	munmap(headers, st.st_size);
	close(fd);
	/* freestyler: let st_mtime = now */
	utime(direct, NULL);
	return result;
}

#if 0
int
safe_substitute_record(char *direct, struct fileheader *fhdr, int ent, int sorted)
{
	int fd, result = 0;
	struct stat st;
	struct fileheader *header;
	void *buf, *buf1;

	if ((fd = open(direct, O_RDWR, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return -1;
	}

	f_exlock(fd);
	buf = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	if (buf == MAP_FAILED || st.st_size <= 0 || sizeof(struct fileheader) * ent > st.st_size) {
		close(fd);
		return -1;
	}

	TRY
		buf1 = buf + sizeof(struct fileheader) * (ent - 1);
		while (buf1 >= buf) {
			header = (struct fileheader *)buf1;
			if (!strcmp(header->filename, fhdr->filename)) {
				memcpy(header, fhdr, sizeof(struct fileheader));
				break;
			}
			buf1 -= sizeof(struct fileheader);				
		}
	CATCH
		result = -1;
	END

	munmap(buf, st.st_size);
	close(fd);
	return result;
}
#endif

#endif

int
delete_file(char *direct, int ent, char *filename, int remove)
{
	int fd, result = 0;
	struct stat st;
	char path[PATH_MAX + 1], fname[PATH_MAX + 1];
	struct fileheader *header;
	void *buf, *buf1, *buf2;
	char *ptr;

	strlcpy(path, direct, sizeof(path));
	if ((ptr = strrchr(path, '/')) == NULL)
		return -1;
	*ptr = '\0';
	snprintf(fname, sizeof(fname), "%s/%s", path, filename);

	if ((fd = open(direct, O_RDWR, 0)) == -1)
		return -1;

	if (fstat(fd, &st) < 0) {
		close(fd);
		return -1;
	}

	f_exlock(fd);
	buf = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FILE, fd, 0);
	if (buf == MAP_FAILED || st.st_size <= 0) {
		close(fd);
		return -1;
	}

	/* first determine whether ent is a valid entry point */
	header = (struct fileheader *)(buf + sizeof(struct fileheader) * (ent - 1));
	if (ent > 0 && st.st_size >= sizeof(struct fileheader) * ent && !strcmp(header->filename, filename))
		goto move_records;

	buf1 = buf;
	buf2 = buf + st.st_size;

	TRY
		ent = 1;
		while (buf1 < buf2) {
			header = (struct fileheader *)buf1;
			if (!strcmp(header->filename, filename))
				goto move_records;
			buf1 += sizeof(struct fileheader);
			++ent;
		}
	END
		
	/* failed to search specified record */
	munmap(buf, st.st_size);
	close(fd);
	return -1;

move_records:
	TRY
		memmove(buf + sizeof(struct fileheader) * (ent - 1), 
			buf + sizeof(struct fileheader) * ent, st.st_size - sizeof(struct fileheader) * ent);
	CATCH
		result = -1;
	END

	munmap(buf, st.st_size);
	ftruncate(fd, sizeof(struct fileheader) * (st.st_size / sizeof(struct fileheader) - 1));
	if (remove) unlink(fname);

	close(fd);
	return result;
}
