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

#ifndef _BBS_H_
#define _BBS_H_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <setjmp.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <utime.h>
#include <dirent.h>
#include <netdb.h>
#include <limits.h>
#include <fnmatch.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#ifndef CYGWIN
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#endif
#include <sys/un.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include "curlbuild32.h"

#include <libgen.h>

#include <term.h>
#undef bell
#undef lines
#undef insert_character
#undef delete_line

#include "config.h"
#include "functions.h"		/* you can enable functions that you want */

#ifdef CYGWIN
#undef CHECKLOAD
#undef MSGQUEUE
#endif

#include "consts.h"
#include "permissions.h"
#include "shmkey.h"
#include "modes.h"		/* the list of valid user modes */
#include "struct.h"
#include "reportd.h"
#include "macros.h"
#include "sysdep.h"
#include "libSystem.h"

#ifndef WWW_CODE
#include "prototypes.h"
#endif
#include "comm_lists.h"
#include "varibles.h"

#endif				/* of _BBS_H_ */
