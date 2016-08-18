/*
  Find out the birthday users and put them in etc/birthday_today, one user per line.
  Use crontab to run this program.
 */
#include<stdio.h>
#include<time.h>
#include<sys/mman.h>
#include "bbs.h"

#define BIRTHDAY "etc/birthday_today"

struct msgheader{
    time_t when;
    char userid[IDLEN+2]; //who
    char board[BFNAMELEN];
    char filename[FNAMELEN];
    unsigned int flag;
    char type[2]; /*  @:message , r: reply , f: add friend, b: birthday */
    char reserve[8];
};


struct userec *uarr;
struct tm *ptm;
struct stat st;
FILE *fp;
time_t now;
int usize, i, total, fd;
char dir[80];

void add_at(char *userid)
{
    struct msgheader mh;
    snprintf(dir, sizeof(dir), "home/%c/%s/.MSG", mytoupper(userid[0]), userid);
    memset(&mh, 0, sizeof(mh));
    mh.when = time(NULL);
    mh.type[0] = 'b'; //birthday
    append_record(dir, &mh, sizeof(mh));
}
int main()
{
    if(chdir(BBSHOME)<0)
    {
        return 1;
    }
    
    if((fd = open(".PASSWDS", O_RDONLY, 0600)) < 0) {
        return 1;
    }
    if(fstat(fd, &st) < 0) {
        close(fd);
        return 1;
    }

    uarr = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
    usize = st.st_size;
    
    if(uarr == MAP_FAILED) {
        close(fd);
        return 1;
    }
    close(fd);

    time(&now);
    ptm = localtime(&now);
    total = usize / sizeof(struct userec);

    fp = fopen(BIRTHDAY, "w");
    if(fp == NULL) {
        return 1;        
    }
    for(i=0; i<total; i++)
    {
        if(uarr[i].birthmonth == ptm->tm_mon+1 &&   // tm_mon = [0, 11]
           uarr[i].birthday == ptm->tm_mday && uarr[i].userid[0] != 0) {
                //给他们发去生日@
            add_at(uarr[i].userid);
            
            fprintf(fp, "%s\n", uarr[i].userid);
        }
    }
    munmap(uarr, usize);
    fclose(fp);
}

