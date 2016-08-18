#include "rss2bbs.h"

#define MAX_HISTORY		512

char hispool[MAX_ITEMS][512];
int nhis;

int
load_history(char *feedfile)
{
	char fname[256];
	FILE *fin;
	char *ptr;

	nhis = 0;
	snprintf(fname, sizeof(fname), "%s.history", feedfile);
	fin = fopen(fname, "r");
	if (!fin) return -1;

	for (nhis = 0; nhis < MAX_ITEMS && fgets(hispool[nhis], sizeof(hispool[nhis]), fin); nhis++) {
		for (ptr = hispool[nhis] + strlen(hispool[nhis]) - 1;
		     ptr >= hispool[nhis] && (*ptr == '\r' || *ptr == '\n');
		     ptr--)
			*ptr = '\0';
		if (ptr < hispool[nhis]) nhis--; /* blank line */
	}
	fclose(fin);

	return 0;
}

int
in_history(char *link)
{
	int i;
	for (i = 0;
	     i < nhis && strcmp(hispool[i], link) != 0;
	     i++);
	return (i < nhis);	
}
