/* Generel Purpose functions */

#include "rss2bbs.h"

iconv_t utf2gbk;

void iconv_init(void)
{
	utf2gbk = iconv_open("gbk", "utf-8");
}

char*
convert_str(char *dest, char *src, int size)
{
	
	size_t in_top = strlen(src);
	size_t out_top = size - 1;
	memset(dest, 0, size);
	while (*src != '\0' && in_top > 0) {
		iconv(utf2gbk, &src, &in_top, &dest, &out_top);
		if (*src != '\0') {
			src++;	/* skip invalid token */
			in_top--;
		}
	}
	
	return dest;
}

char*
get_str(char *dest, char *src, int size, int len)
{
	strncpy(dest, src, min(size - 1, len));
	dest[min(size - 1, len)] = '\0';
	return dest;
}

