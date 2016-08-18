#include "bbs.h"
#include <glob.h>

#define HASHBIT 	20
#define HASHSIZE	(1 << HASHBIT)

struct ht {		// hash structure for title rethreading
	int hash;
	int id;
	unsigned char title[TITLELEN];
};

void
get_fileinfo(FILE *fp, char owner[IDLEN + 2], char title[TITLELEN])
{
	char buf[1024], *ptr;

	while (fgets(buf, sizeof(buf), fp)) {
		if (buf[0] == '\n')
			break;

		if (strncmp(buf + 2, "信人", 4) == 0) {
			strlcpy(owner, buf + 8, IDLEN + 2);
			ptr = owner;
			while (*ptr != '\0') {
				if (*ptr == ' ' || *ptr == '(' || *ptr == '\n' || *ptr == '\t') {
					*ptr = '\0';
					break;
				}
				++ptr;
			}
		} else if (strncmp(buf, "标  题", 6) == 0) {
			strlcpy(title, buf + 8, TITLELEN);
			if ((ptr = strrchr(title, '\n')) != NULL)
				*ptr = '\0';
		}
	}
	if (title[0] != '\0' && owner[0] == '\0') /* no owner found, may be BBSID posted it */
		strlcpy(owner, BBSID, IDLEN + 2);
	
}

int
ht_insert(struct ht *table, unsigned char *str, int id)
{
	unsigned int hash = 0, i;
	
	// calculate hash code
	if (!memcmp(str, "Re: ", 4))
		str += 4;

	for (i = 0; str[i]; i++)
		hash = ((hash << 5) - hash) + str[i];
	hash &= (HASHSIZE - 1);		// if HASHSIZE does not the power of 2, use hash %= HASHSIZE instead
	
	// find a proper slot
	i = hash;
	while (table[i].hash != 0) {
		if (table[i].hash == hash && !strcmp(str, table[i].title))
			return table[i].id;
		++i;
		if (i >= HASHSIZE) i = 0;
	}

	// fill in empty slot
	table[i].hash = hash;
	table[i].id = id;
	strlcpy(table[i].title, str, sizeof(table[i].title));
	return id;
}

static int
cmp_filetime(const void *hdr1_ptr, const void *hdr2_ptr)
{
        const struct fileheader *hdr1 = (const struct fileheader *)hdr1_ptr;
        const struct fileheader *hdr2 = (const struct fileheader *)hdr2_ptr;
                
        return (hdr1->filetime - hdr2->filetime);
}

void
sort_records(struct fileheader *fileinfo, int count)
{
	qsort(fileinfo, count, sizeof(struct fileheader), cmp_filetime);
}

void
rethread(struct fileheader *fileinfo, int count)
{
	int i, id;
	struct ht *table = calloc(HASHSIZE, sizeof(struct ht));
	
	for (i = 0; i < count; i++)
		fileinfo[i].id = ht_insert(table, (unsigned char *)fileinfo[i].title, fileinfo[i].filetime);
	free(table);
}

void
add_gmark(struct fileheader *fileinfo, int count,
	  struct fileheader *gfileinfo, int gcount)
{
	int i, j, id, marked;
	
	int *table = calloc(HASHSIZE, sizeof(int));
	
	for (i = 0; i < gcount; i++) {
		id = gfileinfo[i].filetime & (HASHSIZE - 1);
		while (table[id] != 0) {
			++id;
			if (id >= HASHSIZE) id = 0;
		}
		table[id] = gfileinfo[i].filetime;
	}
	
	for (i = 0; i < count; i++) {
		id = fileinfo[i].filetime & (HASHSIZE - 1);
		if (table[id] != 0) {
			marked = 0;
			j = id;
			while (table[j] != 0) {
				if (table[j] == fileinfo[i].filetime) {
					marked = 1;
					break;
				}

				++j;
				if (j >= HASHSIZE) j = 0;
			}

			if (marked)
				fileinfo[i].flag |= FILE_DIGEST;
		}
	}
}

int
addentry_normal(char *filename, struct fileheader *fileinfo)
{
	FILE *fp;

	if ((fp = fopen(filename, "r")) == NULL)
		return -1;
	get_fileinfo(fp, fileinfo->owner, fileinfo->title);
	strcpy(fileinfo->filename, filename);
	fileinfo->filetime = atoi(filename+2);		/* get filetime from filename */
	fileinfo->flag = FILE_READ;
	fclose(fp);
	return 0;
}

int
build_dir(char *filename, char *pattern, int digest)
{
	glob_t g;
	FILE *fp;
	int i, count = 0, gcount = 0;
	struct fileheader *fileheaders, *gfileheaders;
	
	g.gl_pathc = 0;
	g.gl_offs = 0;
	glob(pattern, GLOB_DOOFFS, NULL, &g);

	if (g.gl_pathc <= 0)
		return -1;

	if ((fileheaders = calloc(g.gl_pathc, sizeof(struct fileheader))) == NULL)
		return -1;

	for (i = 0; i < g.gl_pathc; i++)
		if (addentry_normal(g.gl_pathv[i], &fileheaders[count]) == 0)
			++count;

	globfree(&g);

	if (count == 0) {
		free(fileheaders);
		return -1;
	}

	//sort_records(fileheaders, count);
	
	rethread(fileheaders, count);

	if (digest) { 				/* add digest mark */
		g.gl_pathc = 0;
		g.gl_offs = 0;
		glob("G.*", GLOB_DOOFFS, NULL, &g);
		
		if ((gfileheaders = calloc(g.gl_pathc, sizeof(struct fileheader))) == NULL) {
			globfree(&g);
			return -1;
		}
		
		for (i = 0; i < g.gl_pathc; i++)
			if (addentry_normal(g.gl_pathv[i], &gfileheaders[gcount]) == 0)
				++gcount;
		
		add_gmark(fileheaders, count, gfileheaders, gcount);

		globfree(&g);

	}
	
	if ((fp = fopen(filename, "w")) == NULL) {
		free(fileheaders);
		return -1;
	}
	
	fwrite(fileheaders, sizeof(struct fileheader), count, fp);
	fclose(fp);
	free(fileheaders);
	return 0;
}

int
build_normaldir(int mailbox)
{
	glob_t g;
	FILE *fp;
	int i, count = 0;

	if (mailbox == 0) {
		build_dir(".DIR", "M.*", 1);;
		build_dir(".DIGEST", "G.*", 0);
	} else {
		build_dir(".DIR", "M.*", 0);	/* build mailbox dir */
	}

	return 0;
}

int
addentry_ann(char *filename, struct annheader *fileinfo)
{
	FILE *fp;
	struct stat st;

	if (stat(filename, &st) == -1)
		return -1;

	memset(fileinfo, 0, sizeof(struct annheader));

	if (S_ISDIR(st.st_mode)) {
		strlcpy(fileinfo->filename, filename, sizeof(fileinfo->filename));
		snprintf(fileinfo->title, sizeof(fileinfo->title), "目录: %s", filename);
		fileinfo->flag = ANN_DIR;
	} else if (S_ISREG(st.st_mode)) {
		if ((fp = fopen(filename, "r")) == NULL)
			return -1;
		get_fileinfo(fp, fileinfo->owner, fileinfo->title);
		fclose(fp);

		fileinfo->flag = ANN_FILE;
		if (fileinfo->title[0] == '\0')
			snprintf(fileinfo->title, sizeof(fileinfo->title), "文件: %s", filename);
	} else if (S_ISLNK(st.st_mode)) {
		if (lstat(filename, &st) == -1)
			return -1;

		if (S_ISDIR(st.st_mode)) {
			fileinfo->flag = ANN_LINK | ANN_DIR;
		} else if (S_ISREG(st.st_mode)) {
			fileinfo->flag = ANN_LINK | ANN_FILE;
		} else {
			return -1;
		}

		snprintf(fileinfo->title, sizeof(fileinfo->title), "衔接: %s", filename);
	} else {
		return -1;
	}

	fileinfo->mtime = st.st_mtime;
	strlcpy(fileinfo->filename, filename, sizeof(fileinfo->filename));
	return 0;
}

int
build_anndir()
{
	glob_t g;
	FILE *fp;
	int i, count = 0;
	struct annheader *annheaders;
	
	g.gl_pathc = 0;
	g.gl_offs = 0;
	glob("M.*", GLOB_DOOFFS, NULL, &g);
	glob("D.*", GLOB_APPEND, NULL, &g);
	glob("G.*", GLOB_APPEND, NULL, &g);

	if (g.gl_pathc <= 0)
		return -1;

	if ((annheaders = malloc(g.gl_pathc * sizeof(struct annheader))) == NULL)
		return -1;

	for (i = 0; i < g.gl_pathc; i++)
		if (addentry_ann(g.gl_pathv[i], &annheaders[count]) == 0)
			++count;

	globfree(&g);

	if (count == 0) {
		free(annheaders);
		return -1;
	}

	if ((fp = fopen(".DIR", "w")) == NULL) {
		free(annheaders);
		return -1;
	}

	fwrite(annheaders, sizeof(struct annheader), count, fp);
	fclose(fp);
	free(annheaders);
	return 0;
}

void
usage()
{
	fprintf(stderr, "Usage: buildir [-a] [-m] [-d which_dir]\n");
	fprintf(stderr, " -a\n build the announce .dir from current directory\n");
	fprintf(stderr, "\n -m\n build the mailbox .dir from current directory\n");
	fprintf(stderr, "\n -d which_dir\n change current directory to which_dir and build the .dir\n");
	fprintf(stderr, "\n -h\n display this help and exit\n\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int ch, dirtype = 0;
	int result;

	while ((ch = getopt(argc, argv, "amd:h")) != -1) {
		switch (ch) {
		case 'a':			// announce
			dirtype = 1;
			break;
		case 'm':			// mailbox
			dirtype = 2;
			break;
		case 'd':
			if (chdir(optarg) == -1) {
				fprintf(stderr, "could not change current directory to %s\n", optarg);
				return -1;
			}
			break;
		case 'h':
		case '?':
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	switch (dirtype) {
	case 1:
		result = build_anndir();
		break;
	case 2:
		result = build_normaldir(1);
		break;
	default:
		result = build_normaldir(0);
	}

	return result;
}
