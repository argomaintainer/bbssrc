/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU

    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
			Guy Vega, gtvega@seabass.st.usm.edu
			Dominic Tynes, dbtynes@seabass.st.usm.edu

    Firebird Bulletin Board System
    Copyright (C) 1996, Hsien-Tsung Chang, Smallpig.bbs@bbs.cs.ccu.edu.tw
			Peng Piaw Foong, ppfoong@csie.ncu.edu.tw

    Firebird2000 Bulletin Board System
    Copyright (C) 1999-2001, deardragon, dragon@fb2000.dhs.org

    Puke Bulletin Board System
    Copyright (C) 2001-2002, Yu Chen, monster@marco.zsu.edu.cn
			     Bin Jie Lee, is99lbj@student.zsu.edu.cn

    Contains codes from YTHT & SMTH distributions of Firebird Bulletin
    Board System.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

#include "bbs.h"
#include "libBBS.h"

static void
sig_hup(int sig_no)
{
	return;		/* ignore SIGHUP signal */
}

int mailmode;
int cmpfnames(void *userid_ptr, void *uv_ptr);

struct perm_t{
	int perm;
	int status;
}perm_status[32];

char *permstrings[] = {
	"PERM_BASIC",
 	"PERM_CHAT",
 	"PERM_PAGE",
 	"PERM_POST",
 	"PERM_LOGINOK",
 	"PERM_DENYPOST",
 	"PERM_CLOAK",
	"PERM_SEECLOAK",
	"PERM_XEMPT",
 	"PERM_WELCOME",
 	"PERM_BOARDS",
 	"PERM_ACCOUNTS",
 	"PERM_CHATCLOAK",
 	"PERM_OVOTE",
 	"PERM_SYSOP",
 	"PERM_POSTMASK",
 	"PERM_ANNOUNCE",
 	"PERM_OBOARDS",
 	"PERM_ACBOARD",
 	"PERM_NOZAP",
 	"PERM_FORCEPAGE",
 	"PERM_EXT_IDLE",
 	"PERM_MESSAGE",
 	"PERM_SENDMAIL",
 	"PERM_SEEIP",
 	"PERM_INTERNAL",
 	"PERM_PERSONAL",
 	"PERM_JUDGE",
 	"PERM_ACHATROOM",
 	"PERM_SUICIDE",
	NULL
};

int
process2(int argc, char **argv){
	int fd;
	int num = 0;
	int i, j;
	char permstring[16];
	int status;	
	char fname[512];
	struct override fh;
	struct userec urec;
	char *ptr = NULL;

	ptr = strtok(argv[5], " \t");
	if(*ptr != '\0'){
	        if(*ptr == '+'){
	                 strcpy(permstring, ptr + 1);
			 status = 1;
		}
		else if(*ptr == '-'){
		         strcpy(permstring, ptr + 1);
		         status = -1;
	        }else{
		         strcpy(permstring, ptr);
			 status = 1;
		}
		for(j = 0;;j++){
		         if(permstrings[j] == NULL)
		                 break;
			 if(strcmp(permstring,permstrings[j]) == 0){
		                 perm_status[num].perm = 1<<j;
				 perm_status[num].status = status;
				 num++;
				 break;
			 }
		}		
	}

	while((ptr = strtok(NULL, " \t")) != NULL){
	        if(*ptr == '\0')
		  continue;

	  	if(*ptr == '+'){
			strcpy(permstring, ptr + 1);
			status = 1;
		}
		else if(*ptr == '-'){
			strcpy(permstring, ptr + 1);
			status = -1;
		}
		else{
			strcpy(permstring, ptr);
			status = 1;
		}
		for(j = 0;;j++){
			if(permstrings[j] == NULL)
				break;
			if(strcmp(permstring,permstrings[j]) == 0){
				perm_status[num].perm = 1<<j;
				perm_status[num].status = status;
				num++;
				break;
			}
		}		
	
	}

	/* return if no rule */
	if(num == 0)
		return 0;

	snprintf(fname, sizeof(fname), "%s/.PASSWDS", BBSHOME);
	if ((fd = open(fname, O_RDONLY)) == -1)
		return -1;

	while (read(fd, &urec, sizeof(urec)) > 0) {
		/* check permission bit */
		for(i = 0; i < num; i++){
			if(perm_status[i].status == 1){
				if((urec.userlevel & perm_status[i].perm) == 0)
					break;
			}else if(perm_status[i].status == -1){
				if((urec.userlevel & perm_status[i].perm) != 0)
					break;
			}
		}
		if(i < num){
			continue;
		}

		/* never send mail to guest */
		if (strcmp(urec.userid, "guest") == 0)
			continue;

		/* check deny list */
		snprintf(fname, sizeof(fname), "%s/home/%c/%s/maildeny",
			 BBSHOME, mytoupper(urec.userid[0]), urec.userid);
		if (search_record(fname, &fh, sizeof (fh), cmpfnames, "argo_mail"))
			continue;

		/* all things goes well, deliver it :-) */
		postmail(argv[2], urec.userid, argv[3], argv[4], 0, NA);
	}
	close(fd);

	unlink(argv[2]);				/* remove mail file */
	return 0;

	
}

int
process(int argc, char **argv)
{
	int fd, perm;
	char fname[512], buf[16];
	struct override fh;
	struct userec urec;
	FILE *fp;

	switch (mailmode) {
		case 0:
			if ((fp = fopen(argv[5], "r")) != NULL) {
				while (fgets(buf, sizeof(buf), fp) != NULL) {
					/* strip tailing newline char. */
					if (buf[strlen(buf) - 1] == '\n')
						buf[strlen(buf) - 1] = 0;

					if (buf[0] == 0)
						continue;

					postmail(argv[2], buf, argv[3], argv[4], 0, YEA);
				}
				fclose(fp);
			}

			unlink(argv[2]);		/* remove mail file */
			return 0;
		case 1:
			perm = PERM_BASIC;
			break;
		case 2:
			perm = PERM_POST;
			break;
		case 3:
			perm = PERM_BOARDS;
			break;
		case 4:
			perm = PERM_CHATCLOAK;
			break;
		case 5:
			process2(argc, argv);
			return 0;
			break;
	}

	snprintf(fname, sizeof(fname), "%s/.PASSWDS", BBSHOME);
	if ((fd = open(fname, O_RDONLY)) == -1)
		return -1;

	while (read(fd, &urec, sizeof(urec)) > 0) {
		/* check permission bit */
		if ((urec.userlevel & perm) == 0)
			continue;

		/* never send mail to guest */
		if (strcmp(urec.userid, "guest") == 0)
			continue;

		/* check deny list */
		snprintf(fname, sizeof(fname), "%s/home/%c/%s/maildeny",
			 BBSHOME, mytoupper(urec.userid[0]), urec.userid);
		if (search_record(fname, &fh, sizeof (fh), cmpfnames, "argo_mail"))
			continue;

		/* all things goes well, deliver it :-) */
		postmail(argv[2], urec.userid, argv[3], argv[4], 0, NA);
	}
	close(fd);

	unlink(argv[2]);                /* remove mail file */
	return 0;
}

void
show_helpmsg()
{
	printf("Send mail to a specific group of users\n\n");
	printf("usage: gsend <mail mode> <mail file> <title> <from> [mail listfile | permissions file]\n\n");
	printf("mail mode can be one of following values: \n\n");
	printf("\t0\tarbitary users specified in mail listfile\n");
	printf("\t1\tusers whose PERM_BASIC bit set true\n");
	printf("\t2\tusers whose PERM_POST bit set true\n");
	printf("\t3\tusers whose PERM_BOARDS bit set true\n");
	printf("\t4\tusers whose PERM_CHATCLOAK bit set true\n");
	printf("\t4\tusers whose filter the permissions file\n");
}

int
main(int argc, char **argv)
{
        FILE *fp;
	fp = fopen("reclog/test","w");
	fprintf(fp, "start gsend\n");
	pid_t	pid;

	if (argc < 5) {
		show_helpmsg();
		return 1;
	}

	mailmode = atoi(argv[1]);
	if (mailmode < 0 || mailmode > 5) {
		printf("invalid mail mode\n");
		return 1;
	}

	if (mailmode == 0 && argc < 6) {
		printf("no listfile specified\n");
		return 2;
	}

	if (mailmode == 5 && argc < 6){
		printf("no permissions file specified\n");
		return 5;
	}	

	if (access(argv[2], R_OK) == -1) {
		printf("mail file is not readable");
		return 3;
	}

	if ((pid = fork()) < 0)
		return 4;

	if (pid == 0) {
		/* move child process into background */
		signal(SIGHUP, sig_hup);
		kill(getpid(), SIGTSTP);
		return process(argc, argv);
	}

	sleep(1);	/* sleep to let child stop itself */
	return 0;
}
