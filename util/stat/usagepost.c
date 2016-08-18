#include "bbs.h"

#define FILE_FLAG (FILE_MARKED + FILE_NOREPLY)

char datestring[STRLEN];


int main()
{
	chdir(BBSHOME);
  char fname[100] = "0Announce/bbslist/board2.lastweek";
  if(dashf(fname)) 
    postfile(fname, "Board_Assessment", "上周讨论区使用率统计", BBSID, FILE_FLAG);
  return 0;
}
