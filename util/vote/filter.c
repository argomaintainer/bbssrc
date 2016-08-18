/* Pudding: 过滤不良的附言 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFSIZE 4096
#define IDLEN 13
#define STRLEN 80
#define QUIT 666

int vote_id = 0;

struct ballot {
	char uid[IDLEN + 1];    /* 投票人       */
	unsigned int voted;     /* 投票的内容   */
	char msg[3][STRLEN];    /* 建议事项     */
	char votehost[16];      /* 投票IP       */
	time_t votetime;        /* 投票时间     */
};

int to_del[10000];
int total_to_del;

int
parse_del_file(char* filename)
{
	FILE* fp;

	int i, j, temp;

	fp = fopen(filename, "r");
	if (!fp) return -1;
	for (total_to_del = 0;
	     (total_to_del < 10000 &&
	      fscanf(fp, "%d", &(to_del[total_to_del])) != EOF);
	     total_to_del++);
	fclose(fp);

	/* Sort it */
	for (i = 0; i < total_to_del - 1; i++) {
		for (j = i + 1; j < total_to_del; j++) {
			if (to_del[i] > to_del[j]) {
				temp = to_del[i];
				to_del[i] = to_del[j];
				to_del[j] = temp;
			}
		}
	}

	return 0;
}

int
do_filter(int in_fd, int out_fd)
{
	int fid = 0;
	int id = 0;
	struct ballot data;
	while (read(in_fd, &data, sizeof(data)) == sizeof(data)) {
		if (fid < total_to_del && to_del[fid] == id) {
			(data.msg)[0][0] = '\0';
			fid++;
		}
		write(out_fd, &data, sizeof(data));
		id++;
	}
	return 0;
}


int
main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s op-file\n", argv[0]);
		return -1;
	}
	if (parse_del_file(argv[1]) != 0) {
		fprintf(stderr, "Error while opening op-file\n");
		return -1;
	}
	return do_filter(0, 1);
}
