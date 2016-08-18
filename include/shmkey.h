#define SHMKEY_BASE     31700

struct _shmkey {
	char key[16];
	int value;
};

const static struct _shmkey shmkeys[] = {
	{ "BCACHE_SHMKEY",  SHMKEY_BASE       },
	{ "UCACHE_SHMKEY",  SHMKEY_BASE +  10 },
	{ "UTMP_SHMKEY",    SHMKEY_BASE +  20 },
	{ "ACBOARD_SHMKEY", SHMKEY_BASE +  30 }, /* activeboard */
	{ "ISSUE_SHMKEY",   SHMKEY_BASE +  40 }, /* ½øÕ¾»­Ãæ */
	{ "GOODBYE_SHMKEY", SHMKEY_BASE +  50 },
	{ "WELCOME_SHMKEY", SHMKEY_BASE +  60 },
	{ "STAT_SHMKEY",    SHMKEY_BASE +  70 },
	{ "CONV_SHMKEY",    SHMKEY_BASE +  80 }, /* gb to big5 */
	{ "MCACHE_SHMKEY",  SHMKEY_BASE +  90 },
	{ "EL_SHMKEY",      SHMKEY_BASE + 100 },
	{ {0}, 0 }
};
