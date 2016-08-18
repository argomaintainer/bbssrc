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

#define REPORTD

#include "bbs.h"

#ifdef MSGQUEUE

int msqid;
int fd[LOG_FILENUM];
int flag_force = 0;

void
show_help()
{
	printf("Usage: reportd [options]\n");
	printf("Options:\n");
	printf("  -f		force reportd to create a new message queue\n");
	printf("  -h		display this information\n");
	printf("  -v		display version information\n");
	exit(0);
}

void
show_version()
{
	printf("reportd for %s\n", BBSVERSION);
	exit(0);
}

void
fatal(char *str)
{
	printf("%s\n", str);
	exit(1);
}

void
init(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "fhv")) != -1) {
		switch(c) {
			case 'f':
				flag_force = 1;
				break;
			case 'h':
				show_help();
				break;
			case 'v':
				show_version();
				break;
		}
	}
}

void
init_msq()
{
	struct msqid_ds d;

	if ((msqid = msgget(MSQKEY, IPC_CREAT | IPC_EXCL | 0644)) == -1) {
		if (errno != EEXIST) {
			perror("reportd");
			exit(1);
		}

		if (flag_force == 0) {
			fatal("reportd: message queue already exists");
		} else {
			if ((msqid = msgget(MSQKEY, IPC_CREAT | 0644)) == -1) {
				perror("reportd");
				exit(1);
			}
		}
	}

	/* tries to reset max # of bytes on the queue */
	if (msgctl(msqid, IPC_STAT, &d) == 0) {
		d.msg_qbytes = MSQ_SIZE;
		msgctl(msqid, IPC_SET, &d);
	}
}

void
init_daemon()
{
	switch (fork()) {
		case -1:
			exit(-1);
		case  0:
			break;
		default:
			exit(0);
	}
	setsid();

	chdir(LOGDIR);
	umask(022);
}

void
init_logfiles()
{
	int count = 0;

	while (logfiles[count].fname[0] != '\0') {
		fd[logfiles[count].index] = open(logfiles[count].fname, O_WRONLY | O_CREAT | O_APPEND, 0644);
		++count;
	}
}

int main(int argc, char **argv)
{
	struct bbsmsg msg;

	setgid((gid_t)BBSGID);
	setuid((uid_t)BBSUID);

	init(argc, argv);
	init_msq();
	init_daemon();
	init_logfiles();

	while (1) {
		if (msgrcv(msqid, &msg, sizeof(msg), BBSMSGTYPE, 0) != -1) {
			if (msg.msgtype >= 0 && msg.msgtype < LOG_FILENUM && fd[msg.msgtype] > 0)
				write(fd[msg.msgtype], msg.message, strlen(msg.message));
		}
	}

	return 0;
}

#else /* MSGQUEUE */

/* Henry: define a null main function here */
int main()
{
	return 0;
}

#endif /* MSGQUEUE */
