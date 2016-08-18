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

#ifdef OSF
struct rtentry;
struct mbuf;			/* shut up gcc on OSF/1 4.0 */

#include <sys/table.h>
#endif

#define SOCKET_QLEN     32

#define TH_LOW         	100
#define TH_HIGH        	500

#define PID_FILE                BBSHOME"/reclog/bbsd.pid"
#define LOG_FILE                BBSHOME"/reclog/bbsd.log"
#define BAD_HOST                BBSHOME"/etc/bad_host"
#define NOLOGIN                 BBSHOME"/etc/NOLOGIN"

#ifdef  CHECKLOAD
#define BANNER  "\n\033[1;36mBBS 最近 \033[33m(1,10,15)\033[36m 分钟平均负荷为\033[33m %s \033[36m(目前上限 = %d). \033[m\n"
#define HEAVY_BANNER "\n很抱歉, 目前系统负荷过重, 请稍后再来\n"
#endif

jmp_buf byebye;

char fromhost[60];	// sizeof(fromhost) should be larger than INET6_ADDRSTRLEN (46)
char raw_fromhost[60];
char status[64];

#ifdef CHECKLOAD
char loadstr[64];
#endif

//#ifdef SETPROCTITLE
int bbsport;
//#endif

/* Pudding: IP验证 */
#ifdef AUTHHOST
#define AUTH_HOST	BBSHOME"/etc/auth_host"
unsigned int valid_host_mask = 0;
unsigned int perm_unauth = 0;
#endif

extern struct user_info uinfo;

static void
telnet_init(void)
{
	/* Henry: some telnet client, such as FreeBSD's, refuses WILL
	 * TELOPT_BINARY options if not serves at stardard telnet port
	 * sends DO instead of WILL to solve this problem
	 */
	static unsigned char svr[] = {
		IAC, WILL, TELOPT_ECHO,
		IAC, WILL, TELOPT_SGA,
		IAC, DO, TELOPT_BINARY
	};

	send(0, svr, sizeof(svr), 0);
}


/* 
 * monster: we use getnameinfo because it is  AF-indepentdent,
 *          if getnameinfo fails, the function will not further
 *          resolve the host name. 
 */

static void
getremotename(struct sockaddr *ss, int addrlen, char *rhost, int len)
{
	getnameinfo(ss, addrlen, rhost, len, NULL, 0, 0);
	if (strncmp(rhost, "::ffff:", 7) == 0)
		memmove(rhost, rhost + 7, len - 7);
}


#ifdef  CHECKLOAD
#ifndef OSF

int
chkload(int limit)
{
	double cpu_load[3];
	int i;

#ifdef BSD44
	getloadavg(cpu_load, 3);
#elif defined(LINUX)
	FILE *fp;

	fp = fopen("/proc/loadavg", "r");
	if (!fp)
		cpu_load[0] = cpu_load[1] = cpu_load[2] = 0;
	else {
		float av[3];

		fscanf(fp, "%g %g %g", av, av + 1, av + 2);
		fclose(fp);
		cpu_load[0] = av[0];
		cpu_load[1] = av[1];
		cpu_load[2] = av[2];
	}
#else

#include <nlist.h>

#ifdef SOLARIS
#define VMUNIX  "/dev/ksyms"
#define KMEM    "/dev/kmem"
	static struct nlist nlst[] = { {"avenrun"}, {0} };
#else
#define VMUNIX  "/vmunix"
#define KMEM    "/dev/kmem"
	static struct nlist nlst[] = { {"_avenrun"}, {0} };
#endif
	long avenrun[3];
	static long offset = -1;
	int kmem;

	if ((kmem = open(KMEM, O_RDONLY)) == -1)
		return (1);
	if (offset < 0) {
		nlist(VMUNIX, nlst);
		if (nlst[0].n_type == 0)
			return (1);
		offset = (long) nlst[0].n_value;
	}
	if (lseek(kmem, offset, L_SET) == -1) {
		close(kmem);
		return (1);
	}
	if (read(kmem, (char *) avenrun, sizeof (avenrun)) == -1) {
		close(kmem);
		return (1);
	}
	close(kmem);
#define loaddouble(la) ((double)(la) / (1 << 8))
	for (i = 0; i < 3; i++)
		cpu_load[i] = loaddouble(avenrun[i]);
#endif

	i = cpu_load[0];
	if (i < limit)
		i = 0;

	sprintf(loadstr, "%.2f %.2f %.2f", cpu_load[0], cpu_load[1], cpu_load[2]);
	return i;
}
#endif

#ifdef OSF			/* OSF */
int
chkload(int limit, int mode)
{
	struct tbl_loadavg tbl;
	double cpu_load[3];
	int i;

	if (table(TBL_LOADAVG, 0, &tbl, 1, sizeof (struct tbl_loadavg)) != 1) {
		cpu_load[0] = cpu_load[1] = cpu_load[2] = 0;
	} else if (tbl.tl_lscale) {
		/* in long */
		for (i = 0; i < 3; i++)
			cpu_load[i] = (double) tbl.tl_avenrun.l[i] / tbl.tl_lscale;
	} else {
		/* in double */
		for (i = 0; i < 3; i++)
			cpu_load[i] = tbl.tl_avenrun.d[i];
	}

	i = (cpu_load[0] < limit) ? 0 : cpu_load[0];

	if (mode == 0) {
		sprintf(loadstr, "%.2f %.2f %.2f", cpu_load[0], cpu_load[1], cpu_load[2]);
//		do_report("reclog/bbsload", loadstr);

		/* monster: try to kill abnormal processes when load is higher than 20 */
		if (cpu_load[0] >= 20) {
			kill_abnormal_processes();
			report("abnormal processes killed");
		}
	}

	return i;
}
#endif

#endif

static void
reapchild(int signo)
{
	int state, pid;

	while ((pid = waitpid(-1, &state, WNOHANG | WUNTRACED)) > 0) ;
}

static void
start_daemon(void)
{
	int n;
	char buf[80];

	/*
	 * More idiot speed-hacking --- the first time conversion makes the C
	 * library open the files containing the locale definition and time
	 * zone. If this hasn't happened in the parent process, it happens in
	 * the children, once per connection --- and it does add up.
	 */

	time_t dummy = time(NULL);
	struct tm *dummy_time = localtime(&dummy);

	strftime(buf, 80, "%d/%b/%Y:%H:%M:%S", dummy_time);
	n = getdtablesize();

	if (fork())
		exit(0);
	setsid();
	if (fork())		/* one more time */
		exit(0);

	while (n)
		close(--n);
	for (n = 1; n < NSIG; n++)
		signal(n, SIG_IGN);
}

static void
close_daemon(int signo)
{
	exit(0);
}

static void
bbsd_log(char *str)
{
	char buf[256];
	time_t mytime;
	struct tm *tm;

	mytime = time(NULL);
	tm = localtime(&mytime);
	snprintf(buf, sizeof(buf), "%.2d/%.2d/%.2d %.2d:%.2d:%.2d bbsd[%d]: %s",
		tm->tm_year % 100, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, getpid(), str);
	file_append(LOG_FILE, buf);
}


static int
bind_port(char *port, socklen_t *addrlen)
{
	int sock;
	fd_set readfds;
	const int on = 1;
	char buf[STRLEN];
	struct sockaddr_in6 sin;
/* 	#ifndef SETPROCTITLE */
/* 	int bbsport; */
/* 	#endif */

	if ((bbsport = atoi(port)) <= 0)
		return -1;

	if ((sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0)
		return -1;

	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &on, sizeof (on));

	memset(&sin, 0, sizeof(sin));
	sin.sin6_family = AF_INET6;
	sin.sin6_port = htons(bbsport);
	*addrlen = sizeof(sin);

	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
		snprintf(buf, sizeof(buf), "bind_port can't bind to %s [%s]\n", port, strerror(errno));
		bbsd_log(buf);
		return -1;
	}

	if (listen(sock, SOCKET_QLEN) < 0) {
		snprintf(buf, sizeof(buf), "bind_port can't listen to %s [%s]\n", port, strerror(errno));
		bbsd_log(buf);
		return -1;
	}

	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);

	snprintf(buf, sizeof(buf), "started on port %s\n", port);
	bbsd_log(buf);

	return sock;
}

static int
bad_host(char *name)
{
	return check_host(BAD_HOST, name, 0);
}

#ifdef CHECKLOAD
void
print_loadmsg(void)
{
	prints(BANNER, loadstr, TH_LOW);
}
#endif

/* monster: kill abnormal processes */
void
kill_abnormal_processes(void)
{
	FILE *fp;
	char buf[80], buf2[6];
	int pid, cpu_time;
	static int last_executed;

	if (time(NULL) - last_executed < 180)
		return;
	if (dashf("kproc.tmp"))
		return;
	last_executed = time(NULL);

	system("top>kproc.tmp");
	fp = fopen("kproc.tmp", "r");

	if (fp) {
		while (!feof(fp)) {
			fgets(buf, 80, fp);
			if (strstr(buf, "bbsd")) {
				strncpy(buf2, buf, 5);
				buf2[5] = 0;
				pid = atoi(buf2);

				buf2[0] = buf[43];
				buf2[1] = buf[44];
				buf2[2] = buf[46];
				buf2[3] = buf[47];
				buf2[4] = 0;
				if (buf2[0] == ' ')
					buf2[0] = '0';
				cpu_time = atoi(buf2);

				if (cpu_time >= 100)
					safe_kill(pid);
			}
		}
	}

	unlink("kproc.tmp");
}

#ifdef BBSMAIN
int
main(int argc, char **argv, char **envp)
{
	int msock, csock;	/* socket for Master and Child */
	int nfds;		/* number of sockets */
	int overload;
	pid_t pid;
	time_t uptime;
	socklen_t addrlen;
	char buf[STRLEN];
	struct timeval tv;
	struct sockaddr_in6 sin;
	struct sockaddr *cliaddr = (struct sockaddr *)&sin;


	/* --------------------------------------------------- */
	/* setup standalone                                    */
	/* --------------------------------------------------- */

	start_daemon();

	signal(SIGCHLD, reapchild);
	signal(SIGTERM, close_daemon);

	/* --------------------------------------------------- */
	/* port binding                                        */
	/* --------------------------------------------------- */

	if (argc < 2 || (msock = bind_port(argv[1], &addrlen)) < 0)
		return 1;

	nfds = msock + 1;

#ifndef CYGWIN

#ifdef SET_EFFECTIVE_ID
	/* --------------------------------------------------- */
	/* Set appropriate effective gid/uid                   */
	/* --------------------------------------------------- */
	if (setegid(BBSGID) == -1 || seteuid(BBSUID) == -1) {
		bbsd_log("failed to set effective gid/uid");
		return -1;
	}
#else
	/* --------------------------------------------------- */
	/* Give up root privileges                             */
	/* --------------------------------------------------- */
	if (setgid(BBSGID) == -1 || setuid(BBSUID) == -1) {
		bbsd_log("failed to set gid/uid");
		return -1;
	}
#endif

#endif
	
	if (chdir(BBSHOME) == -1) {
		bbsd_log("cannot access bbs home directory");
		return -1;
	}

	umask(022);
	unlink(PID_FILE);
	snprintf(buf, sizeof(buf), "%d", getpid());
	file_append(PID_FILE, buf);

#ifdef SETPROCTITLE
	init_setproctitle(argc, argv, envp);
	setproctitle("%s: accepting connections (port %d)", "bbsd", bbsport);
#endif

	/* --------------------------------------------------- */
	/* main loop                                           */
	/* --------------------------------------------------- */

	tv.tv_sec = 60 * 30;
	tv.tv_usec = 0;

	overload = uptime = 0;

	while (1) {
#ifdef  CHECKLOAD
		pid = time(NULL);
		if (pid > uptime) {
#ifndef OSF
			overload = chkload(overload ? TH_LOW : TH_HIGH);
#else
			overload = chkload(overload ? TH_LOW : TH_HIGH, 0);
#endif
			uptime = pid + overload + 45;
			/* 短时间内不再检查 system load */
		}
#endif

	      again:

		do {
			memset(&sin, 0, sizeof(sin));
			addrlen = sizeof(struct sockaddr_in6);
			csock = accept(msock, cliaddr, &addrlen);
		} while (csock < 0 && errno == EINTR);

		if (csock < 0) {
			reapchild(SIGCHLD);
			goto again;
		}

#ifdef  CHECKLOAD
		if (overload) {
			write(csock, HEAVY_BANNER, strlen(HEAVY_BANNER));
			sleep(3);
			close(csock);
			continue;
		}
#endif

#ifdef NOLOGIN
		{
			FILE *fp;
			char buf[256];

#define MYBANNER "\r\n系统处于\033[1;33m暂停登陆\033[m状态\r\n\033[1;32m[本站程序维护可以删除 \'\033[36m~bbs/etc/NOLOGIN\033[32m\' 后解除该状态]\033[m\r\n\r\n＝＝＝＝＝＝关于系统进入暂停登陆状态的【公告】＝＝＝＝＝＝\r\n"

			if ((fp = fopen(NOLOGIN, "r")) != NULL) {
				write(csock, MYBANNER, strlen(MYBANNER));
				while (fgets(buf, 255, fp) != 0) {
					strcat(buf, "\r");
					write(csock, buf, strlen(buf));
				}
				fclose(fp);
				sleep(5);
				close(csock);
				continue;
			}
		}
#endif

		pid = fork();

		if (!pid) {
			while (--nfds >= 0)
				close(nfds);

			dup2(csock, 0);
			close(csock);
			dup2(0, 1);
			dup2(0, 2);	/* monster: 处理掉stderr, 防止system调用破坏文件 */

			getremotename(cliaddr, addrlen, fromhost, sizeof(fromhost));
			inet_ntop(AF_INET6, &sin.sin6_addr, raw_fromhost, sizeof(raw_fromhost));
			if (strncmp(raw_fromhost, "::ffff:", 7) == 0)
				memmove(raw_fromhost, raw_fromhost + 7, sizeof(raw_fromhost) - 7);
			if (fromhost[0] == '\0') {
				strcpy(fromhost, raw_fromhost);
			}
			/* ban 掉 bad host / bad user */
			if (bad_host(fromhost))
				exit(1);
			if (raw_fromhost[0] != '\0' && bad_host(raw_fromhost))
				exit(1);

			/* 记录ip验证情况 */
#ifdef AUTHHOST
			if (check_host(AUTH_HOST, raw_fromhost, 1) ||
			    check_host(AUTH_HOST, fromhost, 1))
				valid_host_mask = HOST_AUTH_YEA;
			else
				valid_host_mask = HOST_AUTH_NA;
#endif
			telnet_init();
			nice(3);	/* lower priority .. */
			memset(&uinfo, 0, sizeof(uinfo));
			start_client();
		}

		close(csock);
	}
}
#endif
