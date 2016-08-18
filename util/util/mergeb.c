/*
 * mergeb.c: merge two .DIR or .DIGEST file into one
 *
 * Author: old.bbs@bbs.sjtu.edu.cn
 *
 * Compilation:
 *    cc -o mergeb mergeb.c
 *
 * Usage:
 *    mergeb source1 source2 target
 *
 * Comments:
 *    Used when merge two boards in Firebird system.
 *
 *
 * Copyright (C) 2000-2001 Sjtu BBS Dev Group. <bbs.sjtu.edu.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * $Id: mergeb.c,v 1.1.1.1 2003-02-20 19:54:46 bbs Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRLEN 80
#define MAXLENGTH 256
struct fileheader {		/* This structure is used to hold data in */
	char filename[STRLEN];	/* the DIR files */
	char owner[STRLEN];
	char title[STRLEN];
	unsigned level;
	unsigned char accessed[12];	/* struct size = 256 bytes */
};

int
cmpname(char *s1, char *s2)
{
	int len1, len2;

	len1 = strlen(s1);
	len2 = strlen(s2);

	if (len1 > len2)
		return 1;
	if (len1 < len2)
		return (-1);

	return (strcmp(s1, s2));
}

int
main(argc, argv)
int argc;
char *argv[];
{
	FILE *fpin1, *fpin2, *fpout;
	struct fileheader fhin1, fhin2;

	if (argc != 4) {
		printf("Useage: Merge source1 source2 target\n");
		return;
	}

	if (!(fpin1 = fopen(argv[1], "r")) || !(fpin2 = fopen(argv[2], "r"))) {
		printf("Soruce file error!\n");
		return 1;
	}
	if (!(fpout = fopen(argv[3], "w"))) {
		printf("Target file error!\n");
		return 1;
	}

	if (fread(&fhin1, sizeof (struct fileheader), 1, fpin1) > 0) {
		if (fread(&fhin2, sizeof (struct fileheader), 1, fpin2) > 0) {
			while (1) {
				if (cmpname(fhin1.filename, fhin2.filename) > 0) {
					fwrite(&fhin2,
					       sizeof (struct fileheader), 1,
					       fpout);
					if (fread
					    (&fhin2, sizeof (struct fileheader),
					     1, fpin2) <= 0) {
						fwrite(&fhin1,
						       sizeof (struct
							       fileheader), 1,
						       fpout);
						break;
					}
				} else {
					fwrite(&fhin1,
					       sizeof (struct fileheader), 1,
					       fpout);
					if (fread
					    (&fhin1, sizeof (struct fileheader),
					     1, fpin1) <= 0) {
						fwrite(&fhin2,
						       sizeof (struct
							       fileheader), 1,
						       fpout);
						break;
					}
				}
			}
		} else {
			fwrite(&fhin1, sizeof (struct fileheader), 1, fpout);
		}
	}

	while (fread(&fhin1, sizeof (struct fileheader), 1, fpin1) > 0) {
		fwrite(&fhin1, sizeof (struct fileheader), 1, fpout);
	}
	while (fread(&fhin2, sizeof (struct fileheader), 1, fpin2) > 0) {
		fwrite(&fhin2, sizeof (struct fileheader), 1, fpout);
	}

	fclose(fpin1);
	fclose(fpin2);
	fclose(fpout);

	return 0;
}
