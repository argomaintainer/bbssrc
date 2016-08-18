/* crypt.c */
char *crypt_des(char *buf, char *salt);

/* fileio.c */
int file_append(char *fpath, char *msg);
int file_appendfd(char *fpath, char *msg, int *fd);
int file_appendline(char *fpath, char *msg);
int dashf(char *fname);
int dashd(char *fname);
int dash(char *fname);
int part_cp(char *src, char *dst, char *mode);
int f_cp(char *src, char *dst, int mode);
int valid_fname(char *str);
int touchfile(char *filename);
int f_rm(char *fpath);
int f_mv(char *src, char *dst);
int f_mkdir(char *path, int omode);
int f_exlock(int fd);
int f_unlock(int fd);
int filelock(char *filename, int block);
int fileunlock(int fd);
int seek_in_file(char *filename, char *seekstr);

/* net.c */
int async_connect(char *address, int port, int timeout);

/* string.c */
char *substr(char *string, int from, int to);
char *stringtoken(char *string, char tag, int *log);
void strtolower(char *dst, char *src);
void strtoupper(char *dst, char *src);
int killwordsp(char *str);
int is_alpha(int ch);
void my_ansi_filter(char *source);
char *ansi_filter(char *source);
char *Cdate(time_t * clock);
char *strstr2(char *s, char *s2);
char *strstr2n(char *s, char *s2, size_t size);
void fixstr(char *str, char *fixlist, char ch);
void trim(char *str);
size_t strlcpy(char *dst, const char *src, size_t siz);
size_t strlcat(char *dst, const char *src, size_t siz);
char *strcasestr(const char *phaystack, const char *pneedle);

int inset(char *set, char c);
char* strsect(char *str, char *sect, char *delim);

/* system.c */
int cmd_exec(const char *cmd, ...);

/* snprintf.c */

int portable_snprintf(char *, size_t, const char *, /*args*/ ...);
int portable_vsnprintf(char *, size_t, const char *, /*args*/ ...);

#define snprintf portable_snprintf
#define vsnprintf portable_vsnprintf

/* setproctitle.c */
#ifndef HAVE_SETPROCTITLE
void init_setproctitle(int argc, char **argv, char **envp);
void setproctitle(const char *fmt, ...);
#else
#define init_setproctitle(argc, argv, envp);
#endif

/* stringlist.c */

typedef struct {
	char **strs;
	int length;
	int maxused;
	int alloced;
} slist;

slist *slist_init(void);
void slist_clear(slist *sl);
void slist_free(slist *sl);
int slist_add(slist *sl, const char *str);
int slist_remove(slist *sl, int idx);
int slist_loadfromfile(slist *sl, const char *filename);
int slist_savetofile(slist *sl, const char *filename);
char *slist_next(slist *sl, int *cur);
char *slist_prev(slist *sl, int *cur);
int slist_indexof(slist *sl, const char *str);

/* sphpsiganl.c */
int BBS_SINGAL(const char *format, ...);

