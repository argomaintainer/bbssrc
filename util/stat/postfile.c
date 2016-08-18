#include "bbs.h"

#define FILE_FLAG (FILE_MARKED)

int main(int argc, char* argv[])
{
  if(argc != 5)  {
    return 0;
  }
  
  char* file   = argv[1];
  char* board  = argv[2];
  char* title  = argv[3];
  char* author = argv[4];

  chdir(BBSHOME);
  if(dashf(file))  {
    if(postfile(file, board, title, author, FILE_FLAG)!=-1)
      printf("post succeed\n");
    else
      printf("post fail\n");
  } else
    printf("post fail\n");
  return 0;
}
