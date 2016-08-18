#ifdef BBSMAIN

/* activeboard.c */
void activeboard_init(void);
void R_monitor(void);
void printacbar(void);

/* announce.c */
void ann_process_item(FILE *fp, char *directory, struct annheader *header, int *last, int level, int onlydir);
void ann_index_traverse(FILE *fp, char *directory, int *last, int level,int onlydir);
int ann_import_article(char *fname, char *title, char *owner, int attached, int batch);
int ann_loadpaths(void);
int ann_savepost(char *key, struct fileheader *fileinfo, int nomsg);
int author_announce(int ent, struct fileheader *fileinfo, char *direct);
int currboard_announce();
int show_announce(char *direct, char *title, int flag);
int show_board_announce(char *bname);
int show_personal_announce(char *userid);

/* bbs.c */
int acction_mode(int ent, struct fileheader *fileinfo, char *direct);
void add_crossinfo(char *filepath, int mode);
void add_edit_mark(char *fname, int mode, char *title);
void add_sysheader(FILE * fp, char *board, char *title);
void add_syssign(FILE * fp);
void board_usage(char *mode, time_t usetime);
int b_notes_passwd(void);
void cancelpost(char *board, char *userid, struct fileheader *fh, int owned, int keepid);
int check_bm(char *userid, char *bm);
int check_max_post(char *board);
int check_readonly(char *board);
int check_stuffmode(void);
int cmpbnames(void *bname_ptr, void *brec_ptr);
int cmpfilename(struct fileheader *fhdr, char *filename);
int cmpfilename2(void *filename, void *fhdr_ptr);
int cmpuids(void *uid, void *up_ptr);
int cmpuids2(int unum, struct user_info *urec);
int control_user(void);
int del_post(int ent, struct fileheader *fileinfo, char *direct);
int del_range(int ent, struct fileheader *fileinfo, char *direct);
int deny_me(char *bname);
int deny_me_fullsite(void);
int denynames(void *userid, void *dh_ptr);
int denyuser(void);
int dodigest(int ent, struct fileheader *fileinfo, char *direct, int delete, int update);
int digest_post(int ent, struct fileheader *fileinfo, char *direct);
int do_cross(int ent, struct fileheader *fileinfo, char *direct);
int do_post(void);
int do_select2(int ent, struct fileheader *fileinfo, char *direct, char *bname, int newent);
void do_quote(char *filepath, char quote_mode);
int do_reply(char *title, char *id, int article_id);
int do_select(int ent, struct fileheader *fileinfo, char *direct);
int do_thread(void);
int edit_post(int ent, struct fileheader *fileinfo, char *direct);
int edit_title(int ent, struct fileheader *fileinfo, char *direct);
int get_a_boardname(char *bname, char *prompt);
int getattachinfo(struct fileheader *fileinfo);
int isowner(struct userec *user, struct fileheader *fileinfo);
int import_post(int ent, struct fileheader *fileinfo, char *direct);
void make_blist(void);
int marked_all(int type);
int mark_post(int ent, struct fileheader *fileinfo, char *direct);
int outgo_post(struct fileheader *fh, char *board);
int postfile(char *filename, char *nboard, char *posttitle, int mode);
int postfile_cross(char *filename, char *qboard, char *nboard, char *posttitle);
int post_article(char *postboard, char *mailid, unsigned int article_id);
int post_cross(char islocal, int mode);
int post_reply(int ent, struct fileheader *fileinfo, char *direct);
int Q_Goodbye(void);
char *readdoent(int num, void *ent_ptr);
void readtitle(void);
int save_post(int ent, struct fileheader *fileinfo, char *direct);
void setbdir(char *buf, char *boardname);
void setrid(int id);
int show_allmsgs(void);
int show_author(int ent, struct fileheader *fileinfo, char *direct);
int show_user_notes(void);
int show_fileinfo(int ent, struct fileheader *fileinfo, char *direct);
int s_msg(void);
int thesis_mode(void);
int t_friends(void);
int underline_post(int ent, struct fileheader *fileinfo, char *direct);
void update_ainfo_title(int import);

/* bbsd.c */
#ifndef OSF
int chkload(int limit);
#else
int chkload(int limit, int mode);
#endif
void kill_abnormal_processes(void);
void print_loadmsg(void);

/* bcache.c */
int apply_boards(int (*func)(struct boardheader *bptr));
int c_mygrp_unread(struct boardheader *bhp);
int getbnum(char *bname);
int haspostperm(char *bname);
int fillbcache(void *fptr, int unused);
struct boardheader *getbcache(char *bname);
int junkboard(void);
int normalboard(char *bname);
void resolve_boards(void);
int setboardlevel(char *bname);
int update_lastpost(char *board);
int update_total_today(char *board);
int board_setcurrentuser(int idx, int num);


/* bm.c */
int addtocombine(FILE * fp1, FILE * fp, struct fileheader *fileinfo);
int addtodeny(char *uident, char *msg, int ischange, int flag, struct fileheader *header);
int bmfunc_del(void *rptr, void *extrarg);
int bmfunc_mark(void *rptr, void *extrarg);
int bmfunc_combine(void *rptr, void *extrarg);
int bmfunc(int ent, struct fileheader *fileinfo, char *direct, int dotype);
int bmfuncs(int ent, struct fileheader *fileinfo, char *direct);
int delfromdeny(char *uident, int flag);
int Rangefunc(int ent, struct fileheader *fileinfo, char *direct);
int update_boardlist(int action, slist *list, char *uident);

/* boards.c */
void brc_addlist(time_t filetime);
int brc_initial(char *boardname);
void brc_insert(int num);
int brc_unread(time_t filetime);
void brc_update(void);
int choose_board(int newflag);
int gettheboardname(int x, char *title, int *pos, struct boardheader *fh, char *bname);
void load_GoodBrd(void);
void load_restrict_boards(void);
int normalboard(char *bname);
void setoboard(char *bname);
int unread_position(char *dirfile, struct boardheader *ptr);

/* chat.c */
int chat_cmd(char *buf, int cfd);
void printchatline(char *str);
void printchatnewline();
void transPERstr(char *str, char *tmpstr);

/* comm_lists.c */
void load_sysconf_image(char *imgfile, int rebuild);
char *sysconf_str(char *key);

/* delete.c */
void getuinfo(FILE *fn, struct userec *userinfo);
void mail_info(char *lastword, int userlevel, int suicide);

/* edit.c */
void addsignature(FILE *fp, int blank);
void display_buffer(void);
void keep_fail_post(void);
int vedit(char *filename, int flag);
void write_header(FILE *fp, int mode, int inmail);

/* endline.c */
void endline_init(void);
void update_endline(void);
void update_annendline(void);
void update_atraceendline(void);

/* goodbye.c */
int countlogouts(char *filename);
void showstuff(char *buf);
void user_display(char *filename, int number, int mode);

/* filter.c */
#ifdef FILTER
int check_text();
int has_filter_inited();
void init_filter();
int regex_strstr(const char *haystack);
#endif

/* five.c */
void five_pk(int fd, int first);

/* help.c */
int mailreadhelp(void);
int mainreadhelp(void);
int registerhelp(void);
#ifndef REPORTD
void show_help(char *fname);
#endif

/* io.c */
void add_flush(void (*flushfunc)());
void add_io(int fd, int timeout);
int ask(char *prompt);
int getdata(int line, int col, char *prompt, char *buf, int len, int echo, int clearlabel);
int igetkey(void);
void init_alarm(void);
char *make_multiline(char* dest, int size, char *buf, int maxcol, char *background);
int multi_getdata(int line, int col, char *prompt, char*buf, int len, int maxcol, int maxline, int clearlabel);
int ochar(int c);
void oflush(void);
void output(char *s, int len);
int show_multiline(int line, int col, char *buf, int maxcol, char *background, int *offset);

/* list.c */
int allusers(void);
int countusers(void *uentp_ptr, int unused);
int do_userlist(void);
int fill_userlist(void);
int hisfriend(struct user_info *uentp);
int isreject(struct user_info *uentp);
int mailto(void *uentp_ptr, int unused);
int mailtoall(int mode);
int myfriend(unsigned short uid);
int onlinesearch(int currnum, int forward);
int printuent(void *uentp_ptr, int unused);
int show_users(void);

/* listedit.c */
int listedit(char *listfile, char *title, int (*callback)(int action, slist *list, char *uident));

/* mail.c */
int bbs_sendmail(char *fname, char *title, char *receiver, int filter);
int check_maxmail(void);
int check_query_mail(char *qry_mail_dir);
int chkmail(int delay);
int doforward(char *direct, struct fileheader *fh, int mode);
#if !defined(CHATD)
int do_send(char *userid, char *title, int check_permission, int id);
#endif
int getmailboxsize(unsigned int userlevel);
int getmailsize(char *userid);
int invalidaddr(char *addr);
int locate_article(char *direct, struct fileheader *header, int ent, int flag, void *arg);
char *maildoent(int num, void *ent_ptr);
int mail_clear(int ent, struct fileheader *fileinfo, char *direct);
int mail_del(int ent, struct fileheader *fileinfo, char *direct);
int mail_del_range(int ent, struct fileheader *fileinfo, char *direct);
int mail_file(char *tmpfile, char *userid, char *title);
int mail_forward(int ent, struct fileheader *fileinfo, char *direct);
int mail_reply(int ent, struct fileheader *fileinfo, char *direct);
int mail_sysfile(char *tmpfile, char *userid, char *title);
int mail_u_forward(int ent, struct fileheader *fileinfo, char *direct);
void m_feedback(int code, char *uident, char *defaultmsg);
int m_func(int ent, struct fileheader *fileinfo, char *direct);

/* main.c */
void abort_bbs(void);
void c_recover(void);
void docmdtitle(char *title, char *prompt);
int dosearchuser(char *userid);
int egetch(void);
void firsttitle(char *title);
void renew_nickcolor(int usertitle);
void renew_uinfo(void);
void safe_kill(int pid);
void set_numofsig(void);
void showtitle(char *title, char *mid);
void sigfault(int signo);
void start_client(void);
void tlog_recover(void);
void u_exit(void);
int check_system_vote();

/* maintain.c */
int m_editboard(struct boardheader *brd, int pos, int readonly);
int checkgroupinfo(void);
int count_same_reg(char *username, char type, int myecho);
int del_register_now(int ent, struct new_reg_rec *reg, char *direct);
int my_queryreg(void);
int my_searchreg(void);
int press_register_now(int ent, struct new_reg_rec *reg, char *direct);
void stand_title(char *title);
int update_reg(int ent, struct new_reg_rec *reg);

/* modetype.c */
char *modetype(int mode);

/* more.c */
int ansimore(char *filename, int promptend);
int ansimore2(char *filename, int promptend, int row, int numlines);
int ansimore3(char *filename, int promptend);
int ansimore4(char *filename, char *attinfo, char *attlink, char *link, int promptend);
int mesgmore(char *filename);

/* namecomplete.c */
int AddToNameList(char *name);
void CreateNameList(void);
int chkstr(char *otag, char *tag, char *name);
int DelFromNameList(char *name);
void FreeNameList(void);
int namecomplete(char *prompt, char *data);
int SeekInNameList(char *name);
int usercomplete(char *prompt, char *data);

/* pass.c */
int checkpasswd(const char *passwd, const char *test);
int checkpasswd2(const char *passwd, struct userec *user);
int checkpasswd3(const char *passwd, const char *test);
int check_notespasswd(void);
int check_systempasswd(void);
void genpasswd(const char *passwd, unsigned char md5passwd[]);
void igenpass(const char *passwd, const char *userid, unsigned char md5passwd[]);
int setpasswd(const char *passwd, struct userec *user);

/* postheader.c */
void check_title(char *title);
int post_header(struct postheader *header);

/* read.c */
int auth_search_down(int ent, struct fileheader *fileinfo, char *direct);
int auth_search_up(int ent, struct fileheader *fileinfo, char *direct);
int cursor_pos(struct keeploc *locmem, int val, int from_top);
void fixkeep(char *s, int first, int last);
struct keeploc *getkeep(char *s, int def_topline, int def_cursline);
void i_read(int cmdmode, char *direct, void init(), void cleanup(),
       void (*dotitle)(), char *(*doentry)(int, void *), void (*doendline)(), struct one_key *rcmdlist,
       int getrecords(char *, void *, int, int, int), int getrecordnum(char *, int), int ssize);
int locate_the_post(struct fileheader *fileinfo, char *query, int offset, int aflag, int newflag);
int post_search_down(int ent, struct fileheader *fileinfo, char *direct);
int post_search_up(int ent, struct fileheader *fileinfo, char *direct);
int search_author(char *direct, int ent, int forward, char *currauthor);
int search_post(char *direct, int ent, int forward);
int search_title(char *direct, int ent, int foward);
int searchpattern(char *filename, char *query);
int search_articles(struct keeploc *locmem, char *query, int offset, int aflag, int newflag);
void setkeep(char *s, int pos);
int sread(int readfirst, int auser, int ent, struct fileheader *ptitle);
int SR_author(int ent, struct fileheader *fileinfo, char *direct);
int SR_first(int ent, struct fileheader *fileinfo, char *direct);
int SR_first_new(int ent, struct fileheader *fileinfo, char *direct);
int SR_last(int ent, struct fileheader *fileinfo, char *direct);
int SR_read(int ent, struct fileheader *fileinfo, char *direct);
int thread_search_up(int ent, struct fileheader *fileinfo, char *direct);
int thread_search_down(int ent, struct fileheader *fileinfo, char *direct);
int title_search_down(int ent, struct fileheader *fileinfo, char *direct);
int title_search_up(int ent, struct fileheader *fileinfo, char *direct);
int get_dir_index(char* direct, struct fileheader* fhdr);
int jump_to_reply(int ent, struct fileheader *fileinfo, char *direct);


/* record.c */
int append_record(char *filename, void *record, int size);
int apply_record(char *filename, int (*fptr)(void *, int), int size);
int delete_file(char *direct, int ent, char *filename, int remove);
int delete_range(char *filename, int id1, int id2);
int delete_record(char *filename, int size, int id);
int get_num_records(char *filename, int size);
int get_record(char *filename, void *rptr, int size, int id);
int get_records(char *filename, void *rptr, int size, int id, int number);
int move_record(char *filename, int size, int srcid, int dstid);
int process_records(char *filename, int size, int id1, int id2, int (*filecheck)(void *rptr, void *extrarg), void *extrarg);
int safe_substitute_record(char *direct, struct fileheader *fhdr, int ent, int sorted);
int safewrite(int fd, void *buf, int size);
int search_record_forward(char *filename, void *rptr, int size, int start, int (*fptr)(void *, void *), void *farg);
int set_safe_record(void);
int sort_records(char *filename, int size, int (*compare)(const void *, const void *));
int substitute_record(char *filename, void *rptr, int size, int id);
void tmpfilename(char *filename, char *tmpfile, char *deleted);
int search_record_bin(char *filename, void *rptr, int size, int start, int (*fptr)(void *, void *), void *farg);

/* register.c */
int check_register_ok(void);
void check_register_info(void);
void clear_userdir(char *userid);
int compute_user_value(struct userec *urec);
int invalid_email(char *addr);
void new_register(void);
void send_regmail(struct userec *trec);
int countmails(void *uentp_ptr, int unused);
int get_code(struct userec *trec);

/* report.c */
void b_report(char *str);
void autoreport(char *title, char *str, int toboard, char *userid, char *attachfile);
#ifdef MSGQUEUE
void do_report(int msgtype, char *str);
void do_reprot2(int msgtype, char *fmt, ...);
#else
void do_report(char *filename, char *str);
void do_reprot2(char *filename, char *fmt, ...);
#endif
void do_securityreport(char *str, struct userec *userinfo, int fullinfo, char *addinfo);
void log_usies(char *mode, char *msg);
void report(char *fmt, ...);
void securityreport(char *str);
void securityreport2(char *str, int fullinfo, char *addinfo);
void sec_report_level(char *str, unsigned int oldlevel, unsigned int newlevel);

/* thread.c */
int make_thread(char *board, int force_refresh);

/* screen.c */
void clear(void);
void clrtoeol(void);
void clrtobot(void);
void getyx(int *y, int *x);
void initscr(void);
void move(int y, int x);
int num_ans_chr(char *str);
void outc(unsigned char c);
void outs(char *str);
void outns(char *str, int n);
void prints(char *fmt, ...);
void redoscr(void);
void refresh(void);
void rscroll(void);
void saveline(int line, int mode);
void saveline_buf(int line, int mode, struct screenline *buf);
void scroll(void);
void standend(void);
void standout(void);

/* sendmsg.c */
int canmsg(struct user_info *uin);
void count_msg(int signo);
int dowall(struct user_info *uin);
int do_sendmsg(struct user_info *uentp, char msgstr[256], int mode, int userpid);
int get_msg(char *uid, char *msg, int line);
char msgchar(struct user_info *uin);
void r_msg(void);
void r_msg_sig(int signo);
void r_msg2(void);

/* shm.c */
void *attach_shm(char *shmstr, int shmsize);
void deattach_shm(void);
int fill_shmfile(int mode, char *fname, char *shmkey);
void show_goodbyeshm(void);
void show_issue(void);
void show_shmfile(struct FILESHM *fh);
int show_statshm(char *fh, int mode);
void show_welcomeshm(void);

/* stat.c */
int num_useshell(void);
int num_active_users(void);
int num_user_logins(char *uid);
int num_visible_users(void);
int num_alcounter(void);
int count_ip(char *fromhost);
int count_self(void);

/* stuff.c */
int askyn(char *str, int def, int gobottom);
void bell(void);
void check_calltime(void);
int countln(char *fname);
int getattach(char *board, char *fname, struct attacheader *ah);
int getdatestring(time_t now);
int getdirname(char *basedir, char *dirname);
int getfilename(char *basedir, char *filename, int flag, unsigned int *id);
void pressanykey(void);
void presskeyfor(char *msg);
void pressreturn(void);
void printdash(char *msg);
void show_message(char *msg);
int check_host(char *fname, char *name, int nofile);
void count_perm_unauth(void);

/* talk.c */
int addtooverride(char *uident);
int cmpfnames(void *userid_ptr, void *uv_ptr);
void creat_list(void);
int deleteoverride(char *uident, char *filename);
int del_from_file(char *filename, char *str);
int do_talk(int fd);
void endmsg(int signo);
int friend_add(int ent, struct override *fh, char *direct);
int friend_dele(int ent, struct override *fh, char *direct);
int friend_edit(int ent, struct override *fh, char *direct);
int friend_help(void);
int friend_login_wall(struct user_info *pageinfo);
int friend_mail(int ent, struct override *fh, char *direct);
int friend_query(int ent, struct override *fh, char *direct);
int getfriendstr(void);
int getrejectstr(void);
const char *idle_str(struct user_info *uent);
int maildeny_add(int ent, struct override *fh, char *direct);
int maildeny_dele(int ent, struct override *fh, char *direct);
int maildeny_edit(int ent, struct override *fh, char *direct);
int maildeny_help(void);
int maildeny_query(int ent, struct override *fh, char *direct);
int num_alcounter(void);
void override_title(void);
char *override_doentry(int num, void *ent_ptr);
int override_edit(int ent, struct override *fh, char *direc);
int override_add(int ent, struct override *fh, char *direct);
int override_dele(int ent, struct override *fh, char *direct);
char pagerchar(int friend, int pager);
int reject_add(int ent, struct override *fh, char *direct);
int reject_dele(int ent, struct override *fh, char *direct);
int reject_edit(int ent, struct override *fh, char *direct);
int reject_help(void);
int reject_query(int ent, struct override *fh, char *direct);
int servicepage(int line, char *mesg);
int show_one_file(char *filename);
int talkreply(void);
int talk(struct user_info *userinfo);
int t_cmpuids(int uid, struct user_info *up);
struct user_info *t_search(char *sid, int pid);

/* term.c */
void do_move(int destcol, int destline, int (*outc)(int));
int term_init(char *term);

/* ucache.c */
int apply_ulist(int (*fptr)(struct user_info *u));
int getnewutmpent(struct user_info *up);
int getuser(char *userid, struct userec *lookupuser);
void getuserid(char *userid, unsigned short uid);
void resolve_utmp(void);
int searchnewuser(void);
int search_ulist(struct user_info *uentp, int (*fptr)(int, struct user_info *), int farg);
int search_ulistn(struct user_info *uentp, int (*fptr)(int, struct user_info *), int farg, int unum);
int searchuser(char *userid);
void setuserid(int num, char *userid);
int t_search_ulist(struct user_info *uentp, int (*fptr)(int, struct user_info *), int farg, int show, int doTalk);
void update_ulist(struct user_info *uentp, int uent);
void update_utmp(void);
char *u_namearray(char (*buf)[IDLEN + 2], int *pnum, char *tag);
int who_callme(struct user_info *uentp, int (*fptr)(int , struct user_info *), int farg, int me);

/* userinfo.c */
void check_uinfo(struct userec *u, int MUST);
int cmpregrec(void *username_ptr, void *rec_ptr);
int confirm_userident(char *operation);
void display_userinfo(struct userec *u);
void getusertitlestr(unsigned char title, char *name);
int uinfo_query(struct userec *u, int real, int unum);
int auth_fillform(struct userec *u, int unum);

/* xyz.c */
int gettheuserid(int x, char *title, int *id, struct userec *lookupuser);
int heavyload(void);
int is_birth(struct userec user);
void modify_user_mode(int mode);
unsigned int setperms(unsigned int pbits, char *prompt, int numbers, unsigned int (*showfunc)(unsigned int, int, int));
unsigned int showperminfo(unsigned int pbits, int i, int flag);
unsigned int showtitleinfo(unsigned int pbits, int i, int flag);
int x_lockscreen_silent(void);
int x_keyquery(void);

/* vote.c */
int b_closepolls(void);
int b_notes_edit(void);
int b_results(void);
int b_vote(void);
int b_vote_maintain(void);
int can_post_vote(char *board);
int catnotepad(FILE *fp, char *fname);
int dele_vote(int num);
int mk_result(int num);
int show_votes(void);
int vote_flag(char *bname, char val, int mode);

/* other */
#ifdef ZMODEM
int bbs_zsendfile(char *filename, char *remote);
#endif

#endif
