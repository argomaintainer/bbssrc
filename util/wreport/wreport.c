#include "bbs.h"
#define WEATHER_FILE	BBSHOME"/etc/weather"
#define RAW_FILE1	BBSHOME"/tmp/weather1.html"
#define RAW_FILE2	BBSHOME"/tmp/weather2.html"
//#define WEATHER_FILE	"weather"
//#define RAW_FILE1	"weather1.html"
//#define RAW_FILE2	"gdfc.txt"


struct wdata {
	char city[6];
	char weather[24];
	char temp[24];
};

struct wdata data[4];
char timeframe[28];

void
getline(FILE *fp, char *buf, int len)
{
	char *ptr;

	if (fgets(buf, len, fp) == NULL) {
		buf[0] = '\0';
	} else {
		if ((ptr = strrchr(buf, '\n')) != NULL) {
			*ptr = '\0';
			if (ptr > buf) {
				ptr--;
				if (*ptr == '\r') *ptr = '\0';
			}
		}
	}
}

int
getint(char *buf)
{
	char *ptr = buf;
	int result = 0;
	int sign = 1;

	while (*ptr != '\0') {
		if (*ptr != ' ' && *ptr != '\t')
			break;
		++ptr;
	}

	if (*ptr == '-') {
		sign = -1;
		++ptr;
	} else if (*ptr == '+') {
		++ptr;
	}

	while (*ptr != '\0' && isdigit(*ptr)) {
		result = 10 * result + (*ptr - '0');
		++ptr;
	}

	return (sign == 1) ? result : -result;
}

void
process(char *buf)
{
	int len;
	char *sstr = buf + 1;
	char *estr = strstr(buf, "∑¢≤º");

	if (estr != NULL) {
		len = ((estr - sstr) > 23) ? 23 : estr - sstr;
		strncpy(timeframe, sstr, len);
		timeframe[27] = '\0';
	} else {
		timeframe[0] = '\0';
	}
}

void
process1(FILE *fp, char *city, int index)
{
	char buf[1024];
	char *sstr, *estr;
	int len;

	strncpy(data[index].city, city, 5);
	data[index].city[5] = '\0';

	getline(fp, buf, 1024);
	sstr = buf + 4;
	estr = strstr(sstr, "</TD>");
	if (estr) {
		len = ((estr - sstr) > 23) ? 23 : estr - sstr;
		strncpy(data[index].weather, sstr, len);
		data[index].weather[len] = '\0';
	} else {
		data[index].weather[0] = '\0';
	}


	getline(fp, buf, 1024);
	sstr = buf + 4;
	estr = strstr(sstr, "</TD>");
	if (estr) {
		len = ((estr - sstr) > 23) ? 23 : estr - sstr;
		strncpy(data[index].temp, sstr, len);
		data[index].temp[len] = '\0';
	} else {
		data[index].temp[0] = '\0';
	}

}
void
process2(char *buf, char *city, int index)
{
	#undef snprintf
	char *sstr, *estr;
	char str[1024];
	char weather[1024];
	char temp[1024];
	char *ptr;
	int low = 0, hi = 0;
	
	if (sscanf(buf, "%s %s %s", str, weather, temp) != 3) return;
	strcpy(data[index].city, city);
	strncpy(data[index].weather, weather, 23);
	data[index].weather[23] = '\0';
	if ((ptr = strstr(temp, "µΩ")) != NULL) {
		*ptr = ' ';
		*(ptr + 1) = ' ';
	}
	sscanf(temp, "%d %d", &low, &hi);
	snprintf(data[index].temp, 24, "%d°Ê--%d°Ê", low, hi);
}

void wreport()
{
	FILE *fp;

	if ((fp = fopen(WEATHER_FILE, "w")) == NULL)
		return;

	fprintf(fp, "\n[1m[36;40m      ©≥©§©∑©≥©§©∑                        [37m                                    \n");
	fprintf(fp, "[36m  ©≥©§©∑  ©¶©¶[37m©≥©∑[36m©§©∑[34m©§©§[36m©≥©§©∑[34m©§©§©§[37m©≥©∑[34m©§©§©§©§©§©§©§©§©§©§©§©§   [37m         \n");
	fprintf(fp, "©≥©∑ÃÏ[36m©¶©§©≥©§[37m©ª©ø‘§[36m©¶©≥[37m©≥©∑±®[36m©¶[37m©≥©∑[34m©§[37m©ª©ø[34m©§©§©§©§©§©§©§©§©§©§©§©§©§ [37m         \n");
	fprintf(fp, "©ª©ø[36;44m©§©ø©≥©¶[37m∆¯[36m©¶©ª©§[37m©≥©∑©ª©ø[36m©§[37m©≥©∑©ø[36m      [37m                 [36m  ®q ®r        [37;40m    \n");
	fprintf(fp, "[36m  [34m©¶[36;44m    ©¶©ª©§©ø    [37m©ª©ø[36m©§©ø  [37m©ª©ø[36m        [37m                 [36m®q     ®r®q ®r [37;40m    \n");
	fprintf(fp, "[36m  [34m©¶[36;44m    ©ª©§©ø[37m≥« –        ÃÏ∆¯              ∆¯Œ¬(…„ œ∂»)   [36m®t ®t©§©§®t©§©§[40m©§®s\n");
	fprintf(fp, "[37m  [34m©¶[37;44m                                                                      [34;40m©¶ [37m \n");
	fprintf(fp, "  [34m©¶[37;44m          [36m%-10s  %-16s  %-16s [37m             [34;40m©¶ [37m \n", data[0].city, data[0].weather, data[0].temp);
	fprintf(fp, "  [34m©¶[37;44m          [36m                                               [37m             [34;40m©¶ [37m \n");
	fprintf(fp, "  [34m©¶[37;44m          [36m%-10s  %-16s  %-16s     [37m         [34;40m©¶ [37m \n", data[1].city, data[1].weather, data[1].temp);
	fprintf(fp, "  [34m©¶[37;44m          [36m                                               [37m             [34;40m©¶ [37m \n");
	fprintf(fp, "  [34m©¶[37;44m          [36m%-10s  %-16s  %-16s     [37m [36m      [37m©≥[40m©∑[36m \n", data[2].city, data[2].weather, data[2].temp); 
	fprintf(fp, "[37m  [34m©¶[37;44m          [36m                                                      ©≥©§[37m©ª[40m©ø[36m©∑\n");
	fprintf(fp, "[37m  [34m©¶[37;44m          [36m%-10s  %-16s  %-16s  ©≥©§©§©§©∑  [40m  ©¶\n", data[3].city, data[3].weather, data[3].temp);
	fprintf(fp, "[37m  [34m©¶[37;44m                                               [36m   [37m       [36m ©¶      ©¶©§[40m©∑©¶\n");
	fprintf(fp, "[37m  [34m©¶[37;44m          ∑¢≤º ±º‰£∫%-28s[36m      ©≥©§©§©∑    ©¶  [40m©¶©ø\n", timeframe);
	fprintf(fp, "[37m    [44m                                                 [36m     ©¶[37m  [36m  ©¶©§©§©ø  [37;40m©≥©∑\n");
	fprintf(fp, "    [44m   [34m                                               [37m©≥©∑[36m©¶    ©¶©¶  [37m©≥©∑[40m©ª©ø\n");
	fprintf(fp, "       [34m ©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§[37m©ª©ø[36m©ª©§©§[37m©≥©∑[36m  [37m©ª©ø[36m©¶[34m©§\n");
	fprintf(fp, "[37m         [34m ©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§©§[37m©≥©∑[34m©§©§©§©§©§©§[37m©ª©ø[36m      ©¶[34m©§\n");
	fprintf(fp, "[37m                                                ©ª©ø[36m              ©ª©§©§©§©ø  [m\n");
	fprintf(fp, "?                                                       \033[1;36mby onmywayhome@argo\033[m\n");
	fclose(fp);
}

void
parse()
{
	FILE *fp;
	char buf[1024];

#ifdef RAW_FILE1
	if ((fp = fopen(RAW_FILE1, "r")) == NULL)
		return;

	do {
		getline(fp, buf, 1024);
		if (strstr(buf, " ±∑¢≤º") != NULL) process(buf); else
		if (strstr(buf, "±±æ©") != NULL) process1(fp, "±±æ©", 0); 
	} while (buf[0]);

	fclose(fp);
#endif

#ifdef RAW_FILE2
	if ((fp = fopen(RAW_FILE2, "r")) == NULL)
		return;

	do {
		getline(fp, buf, 1024);
    if (strstr(buf, "π„÷›") != NULL) process2(buf, "π„÷›", 1); else
		if (strstr(buf, "÷È∫£") != NULL) process2(buf, "÷È∫£", 2); else
    if (strstr(buf, "…Ó€⁄") != NULL) process2(buf, "…Ó€⁄", 3);
	} while (buf[0]);

	fclose(fp);
#endif

	wreport();
}

int
main()
{
	memset(data, 0, sizeof(data));
	parse();
	return 0;
}

