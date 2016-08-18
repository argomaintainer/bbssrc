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

static int results[32];
int vote_id = 0;

struct ballot {
	char uid[IDLEN + 1];    /* 投票人       */
	unsigned int voted;     /* 投票的内容   */
	char msg[3][STRLEN];    /* 建议事项     */
	char votehost[16];      /* 投票IP       */
	time_t votetime;        /* 投票时间     */
};

int
apply_record(char *filename, int (*fptr)(), int size)
{
	char abuf[BUFSIZE];
	int fd, id = 0;

	if (size > BUFSIZE) {
		return -1;
	}

	if ((fd = open(filename, O_RDONLY, 0)) == -1)
		return -1;
	while (read(fd, abuf, size) == size)
		if ((*fptr) (abuf, ++id) == QUIT) {
			close(fd);
			return QUIT;
		}
	close(fd);
	return 0;
}

int
count_vote(struct ballot *ptr)
{
	int i;
	time_t t;

	t = ptr->votetime;
	printf("%-8d%-12s    %-16s   %24.24s\n", vote_id, ptr->uid, ptr->votehost, ctime(&t));
	printf("选项：");
	for (i = 0; i < 32; i++) {
		if ((ptr->voted >> i) & 1) {
			printf("%c", i + 'A');
			results[i]++;
		}
	}
	printf("\n");
	if ((ptr->msg)[0][0] != '\0') {
		printf("Comment:\n");
		for (i = 0; i < 3 &&(ptr->msg)[i][0] != '\0'; i++) {
			printf("%s\n", (ptr->msg)[i]);
		}
	}
	printf("\n");
	vote_id++;
	return 0;
}

int
main(int argc, char **argv)
{
	int i;

	if (argc < 2) {
		printf("Usage: %s flag-file\n", argv[0]);
		return -1;
	}
	apply_record(argv[1], count_vote, sizeof (struct ballot));

	for (i = 0; i < 32; i++) {
		if (results[i] > 0)
			printf("%c: %d\n", 'A' + i, results[i]);
	}
	return 0;
}
