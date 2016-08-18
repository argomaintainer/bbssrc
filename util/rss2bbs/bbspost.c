#include "rss2bbs.h"

int
write_post(FILE *fp, feeditem_t *post)
{
	static char buf[256];
	char timebuf[256];
	struct tm *tv;
	time_t now = time(NULL);

	tv = localtime(&now);
	strftime(timebuf, sizeof(timebuf), "%a %b %e %T %Y", tv);
	
	
	fprintf(fp, "发信人: %s (%s), 信区: %s\n", currfeed.author, currfeed.author, currfeed.board);
	fprintf(fp, "标  题: %.56s\n", post->title);
	fprintf(fp, "发信站: %s (%s), 自动发信\n", currfeed.sitename, timebuf);
	fprintf(fp, "\n");

	fprintf(fp, "\033[1;32m作者: \033[m%s\n", ((post->author[0] != '\0') ? post->author : "未知"));
	fprintf(fp, "\033[1;32m时间: \033[m%s\n", ((post->pubDate[0] != '\0') ? post->pubDate : "未知"));	
	fprintf(fp, "\033[1;32m链接: \033[m%s\n\n", post->link);

	fprintf(fp, "\033[1;32m摘要:\033[m\n");

	/* Wrap Print the body */
	char *curr = post->desc;
	char *line = post->desc;
	int len = 0;				/* 前面还有四个空格, 初始化一下 */

	for (; *curr != '\0'; curr++) {
		if (len >= 71) {
			strncpy(buf, line, len);
			buf[len] = '\0';
			fprintf(fp, "%s\n", buf);

			len = 0;
			line = curr;
		}

		if (*curr & 0x80) {
			curr++;
			len += 2;
		} else len++;		
	}
	if (*line != '\0') fprintf(fp, "%s\n", line);
	

	/* Print Footter */
	fprintf(fp, "\n--\n\033[m\033[1;31m※ 来源:．%s %s．[FROM: RSS2BBS]\033[m\n",
		BBSNAME, BBSHOST);
	return 0;
}

void
post_items(void)
{
	int i;
	FILE *fp;
	for (i = num_items - 1; i >= 0; i--) {
		fp = fopen(TMPFILE, "w");
		if (!fp) return;
		write_post(fp, item + i);
		fclose(fp);
		
		postfile(TMPFILE, currfeed.board, item[i].title, currfeed.author, 0);
	}
}

