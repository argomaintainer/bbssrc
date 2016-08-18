#define MSQ_SIZE        8192
#define BBSMSGTYPE	17248
#define LOGDIR          BBSHOME"/reclog"

#define LOG_FILENUM	4

#define LOG_TRACE       0
#define LOG_USIES       1
#define LOG_BOARD       2
#define LOG_DEBUG	3

struct loglist {
	char fname[STRLEN];
	int index;
};

#ifdef REPORTD
struct loglist logfiles[] = {
	{ "trace",      LOG_TRACE },
	{ "usies",      LOG_USIES },
	{ "use_board",  LOG_BOARD },
	{ "debug",	LOG_DEBUG },
	{ { '\0' },	0         }
};
#endif

struct bbsmsg {
	long mtype;
	int msgtype;
	char message[256];
};
