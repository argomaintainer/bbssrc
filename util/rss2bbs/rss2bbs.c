/** rss2bbs.c
    Author: Yuheng Kuang <is03kyh@student.zsu.edu.cn>

    将 RSS feed 中的内容贴到版面上
    (程序不负责 RSS feed 的抓取, 可以由 wget 等工具获取文件)

    配置文件: rss2bbs.ini
    SourceName	feed 文件路径	转贴版面	Author
 */

#include "rss2bbs.h"

feed_t currfeed;
feeditem_t item[MAX_ITEMS];
int num_items;
FILE *fhis;

void
h_xmlstr(void *data, const XML_Char *str, int len)
{
	feedstat_t *info = (feedstat_t*)data;	
	static char buf_in[BUFSIZE];
	static char buf_out[BUFSIZE];

	if (!(info->in_parse) || !(info->in_item)) return;

	if (!(info->curr_buf)) return;

	get_str(buf_in, (char*)str, sizeof(buf_in), len);
	convert_str(buf_out, buf_in, sizeof(buf_out));
	strlcat(info->curr_buf, buf_out, info->curr_size);	
}

void
h_elstart(void *data, const XML_Char *el, const XML_Char **attr)
{
	feedstat_t *feedstat = (feedstat_t*)data;
	if (!(feedstat->in_parse)) return;

	if (strcmp(el, "item") == 0) {
		if (num_items >= MAX_ITEMS) {
			feedstat->in_parse = NA;
			return;
		}
		feedstat->in_item = YEA;
		feedstat->curr_buf = NULL;
		feedstat->curr_size = 0;
		memset(item + num_items, 0, sizeof(item[num_items]));
	}

	if (!(feedstat->in_item)) return;			/* 忽略 item 以外的部分 */

	if (strcmp(el, "title") == 0) {
		feedstat->curr_buf = item[num_items].title;
		feedstat->curr_size = sizeof(item[num_items].title);
		feedstat->curr_buf[0] = '\0';
	} else if (strcmp(el, "pubDate") == 0) {
		feedstat->curr_buf = item[num_items].pubDate;
		feedstat->curr_size = sizeof(item[num_items].pubDate);
		feedstat->curr_buf[0] = '\0';
	} else	if (strcmp(el, "link") == 0) {
		feedstat->curr_buf = item[num_items].link;
		feedstat->curr_size = sizeof(item[num_items].link);
		feedstat->curr_buf[0] = '\0';
	} else	if (strcmp(el, "description") == 0) {
		feedstat->curr_buf = item[num_items].desc;
		feedstat->curr_size = sizeof(item[num_items].desc);
		feedstat->curr_buf[0] = '\0';
	} else if (strcmp(el, "author") == 0) {
		feedstat->curr_buf = item[num_items].author;
		feedstat->curr_size = sizeof(item[num_items].author);
		feedstat->curr_buf[0] = '\0';
	}
}

void
h_elend(void *data, const XML_Char *el)
{
	feedstat_t *feedstat = (feedstat_t*)data;
	char *link;
	if (!(feedstat->in_parse)) return;

	if (strcmp(el, "item") == 0) {
		link = item[num_items].link;
		if (link[0] != '\0') {
			if (!in_history(link))
				num_items++;
			fprintf(fhis, "%s\n", link);
		}
		feedstat->in_item = 0;
	} else {
		if (feedstat->in_item) {
			feedstat->curr_buf = '\0';
			feedstat->curr_size = 0;
		}
	}
}

int
parse_feed(void)
{
	XML_Parser parser;
	feedstat_t feedstat;
	char buf[1024];	
	FILE *fp = fopen(currfeed.filename, "r");	
	if (!fp) return -1;

	num_items = 0;
	parser = XML_ParserCreate("UTF-8");
	/* Set Handler */
	XML_SetCharacterDataHandler(parser, h_xmlstr);
	XML_SetElementHandler(parser, h_elstart, h_elend);
	/* Init feedstat */
	memset(&feedstat, 0, sizeof(feedstat));
	feedstat.in_parse  = YEA;
	XML_SetUserData(parser, &feedstat);

	/* Read an Parse */
	while (feedstat.in_parse && fgets(buf, sizeof(buf), fp))
		XML_Parse(parser, buf, strlen(buf), NA);
	fclose(fp);
	XML_ParserFree(parser);

	return 0;
}

int
main(int argc, char **argv)
{
	FILE *fin;
	char fname[256];
	
	iconv_init();

//	setuid(BBSUID);
//	setgid(BBSGID);
	chdir(BBSHOME);

	
	fin = fopen(ETCFILE, "r");
	if (!fin) return -1;

	while (fscanf(fin, "%s %s %s %s",
		      currfeed.sitename, currfeed.filename,
		      currfeed.board, currfeed.author) != EOF) {

		load_history(currfeed.filename);
		snprintf(fname, sizeof(fname), "%s.history", currfeed.filename);
		fhis = fopen(fname, "w");
		if (!fhis) continue;		/* ooops */
		if (parse_feed() == 0)
			post_items();
		fclose(fhis);
	}
	
	fclose(fin);
	return 0;
}
