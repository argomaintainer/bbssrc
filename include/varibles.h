#ifdef BBSMAIN

/* announce.c */
extern struct anninfo ainfo;

/* bbs.c */
extern char *currBM;
extern char currboard[BFNAMELEN + 1];
extern int current_bm;
extern struct userec currentuser;
extern int digestmode;
extern int local_article;
extern char genbuf[BUFLEN];
extern struct postheader header;
extern char quote_title[TITLELEN], quote_board[BFNAMELEN + 1];
extern char quote_file[PATH_MAX + 1], quote_user[IDLEN + 2];
extern char save_title[TITLELEN];
extern int selboard;
#ifdef INBOARDCOUNT
extern int is_inboard;
#endif

/* bbsd.c */
//#ifdef SETPROCTITLE
extern int bbsport;
//#endif

#ifdef AUTHHOST
extern unsigned int valid_host_mask;
extern unsigned int perm_unauth;
#endif

/* bcache.c */
extern struct BCACHE *brdshm;
extern struct boardheader *bcache;
extern int numboards;

/* boards.c */
extern int boardlevel;
extern int brdnum;
extern char *restrict_boards;

/* edit.c */
extern int editansi;

/* io.c */
extern int KEY_ESC_arg;
extern int enabledbchar;

/* main.c */
extern char BoardName[STRLEN];
extern jmp_buf byebye;
extern int count_friends;
extern int count_users;
extern int guestuser;
extern char fromhost[60];
extern char raw_fromhost[60];

extern int iscolor;
extern time_t login_start_time;
extern int numofsig;
extern int showansi;
extern sigjmp_buf jmpbuf;
extern struct user_info uinfo;
extern char ULIST[STRLEN];
extern int utmpent;

/* maintain.c */
extern int usernum;

/* read.c */
extern int previewmode;
extern char currdirect[PATH_MAX + 1];

/* report.c */
#ifdef MSGQUEUE
extern int msqid;
#endif

/* shm.c */
extern struct BCACHE *brdshm;
extern struct UCACHE *uidshm;
extern struct UTMPFILE *utmpshm;
extern struct FILESHM *welcomeshm;
extern struct FILESHM *goodbyeshm;
extern struct FILESHM *issueshm;
extern struct STATSHM *statshm;
extern struct ACSHM *movieshm;
extern struct ELSHM *endline_shm;

/* stuff.c */
extern char datestring[30];

/* talk.c */
extern int talkidletime;
extern int talkrequest;

/* term.c */
extern int t_lines;
extern int t_columns;
extern int t_realcols;

/* ucache.c */
extern int usernumber;

#endif
