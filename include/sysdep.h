#ifdef OSF

/* missing prototypes from system headers (OSF1) */

int flock(int fd, int operation);
int usleep(useconds_t useconds);

#endif

#ifdef SYSV

/* select based version of usleep provided because SYSV lacks of it */

#define usleep(usec)           {                \
    struct timeval t;                           \
    t.tv_sec = usec / 1000000;                  \
    t.tv_usec = usec % 1000000;                 \
    select( 0, NULL, NULL, NULL, &t);           \
}

#endif

#if !defined(PATH_MAX) && defined(MAXPATHLEN)

#define PATH_MAX MAXPATHLEN

#endif


#ifdef CYGWIN

/* winshm.c */
#define IPC_CREAT 0x80000000
#define IPC_EXCL  0x40000000
int shmget(int key, int size, int flag);
void *shmat(int shmid, void *addr, int flag);
int shmdt(void *addr);

/* flock.c */
#define LOCK_SH 1       /* shared lock */
#define LOCK_EX 2       /* exclusive lock */
#define LOCK_NB 4       /* don't block when locking */
#define LOCK_UN 8       /* unlock */

int flock(int fd,int operation);

/* termios.c */
#include <termios.h>
void cfmakeraw(struct termios *);

/* remove things that CYGWIN doesn't support */
#undef DUALSTACK
#undef MSGQUEUE

#endif /* CYGWIN */
