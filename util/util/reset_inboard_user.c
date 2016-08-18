/* 
 *  author: freestyler.bbs@bbs.sysu.edu.cn
 *
 *  2008.09.13: reset the number of inboard user to 0   
 *  2008.09.16: 增加清空指定版面人数为0
*/

#include "bbs.h"

struct BCACHE *boardshm = NULL;
int nboards = -1;	/* 讨论区数目 */


/* freestyler: */
int main(int argc, char** argv)
{
	boardshm = attach_shm("BCACHE_SHMKEY", 3693, sizeof(*boardshm));
	nboards = boardshm->number;
	int fd = filelock("inboarduser.lock", 1);
	int i;

	if( argc == 1 ) { /* 无参数: 清空所有版面在线人数 */
		if( fd > 0 ) {
			for( i = 0; i < nboards; i++)  
				 boardshm -> inboard[i]  = 0;
			 fileunlock(fd);
		}
	} else {
		char* board = argv[1];
		if ( fd > 0 ) {
			for( i = 0; i < nboards; i++) {
				if ( !strcmp(boardshm -> bcache[i].filename, board) ) {
					boardshm->inboard[i] = 0;
					break;
				}
			}
			fileunlock(fd);
		}
	}
	shmdt(boardshm);
	return 0;
}
