/* The following is the implementation of shm, flock and cfmakeraw
   in cygwin environment */

#ifdef CYGWIN
#include "sysdep.h"
#include <windows.h>
#include <string.h>

/*
    Firebird BBS for Windows NT
    Copyright (C) 2000, COMMAN,Kang Xiao-ning, kxn@student.cs.tsinghua.edu.cn

  A dirty implementation of share memory on Win32 by COMMAN <kxn@263.net>
*/

int
shmget(int key, int size, int flag)
{
        HANDLE hMap;
        char fname[9];

        sprintf(fname, "SHM%05X", key);
        if (flag & IPC_CREAT) {
                hMap = CreateFileMapping((HANDLE)0xFFFFFFFF,
                                         NULL,
                                         PAGE_READWRITE,
                                         0,
                                         size,
                                         fname);
        } else {
                hMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, fname);
        }

        return (hMap == 0) ? -1 : (int) hMap;
}

void *
shmat(int shmid, void *addr, int flag)
{
        return MapViewOfFile((HANDLE)shmid, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}

int
shmdt(void *addr)
{
        return (UnmapViewOfFile(addr) == 0) ? -1 : 0;
}

/*
   a flock implemention
   Firebird BBS for Windows NT
   Copyright (C) 2000, COMMAN,Kang Xiao-ning, kxn@student.cs.tsinghua.edu.cn

*/

#include <fcntl.h>
#include <errno.h>

int flock(int fd,int operation)
{
	struct flock lock;

	memset(&lock, 0, sizeof(lock));
	if (operation & LOCK_EX) lock.l_type = F_WRLCK;
	if (operation & LOCK_UN) lock.l_type = F_UNLCK;
	if (operation & LOCK_NB)
		return fcntl(fd,F_SETLK,&lock);
	else
		return fcntl(fd,F_SETLKW,&lock);
}

/*
  Impletation of cfmakeraw, import from cygwin mail archive by Henry
  Author: Alan Evans <Alan_Evans at imv dot com>
  http://www.cygwin.com/ml/cygwin/2002-09/msg01371.html
*/

void cfmakeraw(struct termios *termios_p) {

    termios_p->c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
    termios_p->c_oflag &= ~OPOST;
    termios_p->c_lflag &= ~(ECHO|ECHONL|ICANON|ISIG|IEXTEN);
    termios_p->c_cflag &= ~(CSIZE|PARENB);
    termios_p->c_cflag |= CS8;
}

#endif /* CYGWIN */
