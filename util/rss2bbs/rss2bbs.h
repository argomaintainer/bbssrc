#ifndef _RSS2BBS_H
#define _RSS2BBS_H

#include "bbs.h"
#include "libBBS.h"
#include <expat.h>
#include <iconv.h>

#define min(a, b)		(((a) < (b)) ? (a) : (b))
#define BUFSIZE			4096

#define MAX_ITEMS		1024

#define ETCFILE			"etc/rss2bbs.ini"
#define TMPFILE			"etc/rss2bbs.tmp"

typedef struct {
	char title[TITLELEN];
	char pubDate[256];
	char desc[BUFSIZE];
	char link[1024];
	char author[256];	
} feeditem_t;

typedef struct {
	int in_parse;
	int in_item;
	char *curr_buf;
	int curr_size;
} feedstat_t;

typedef struct {
	char sitename[256];
	char filename[1024];
	char board[80];
	char author[80];
	int firsttime;
} feed_t;

extern feed_t currfeed;
extern feeditem_t item[MAX_ITEMS];
extern int num_items;

void iconv_init(void);
char* convert_str(char *dest, char *src, int size);
char* get_str(char *dest, char *src, int size, int len);
void post_items(void);
int load_history(char *feedfile);
int in_history(char *link);

#endif
