/* general purposed marcos */
#ifdef ALLOW_CHINESE_ID

#define mytoupper(ch) \
	(((ch) >= 'a' && (ch) <= 'z') ? ((ch) + 'A' - 'a') : \
		((((ch) < 'A' || (ch) > 'Z') ? ((ch) % 26 + 'A') : (ch))))

#else

#define mytoupper(ch) \
	(((ch) >= 'a' && (ch) <= 'z') ? ((ch) + 'A' - 'a') : (ch))

#endif

#define mytolower(ch) \
	(((ch) >= 'A' && (ch) <= 'Z') ? ((ch) - 'A' + 'a') : (ch))

#undef isalpha
#define isalpha(ch) \
	(((ch) >= 'A' && (ch) <= 'Z') || ((ch) >= 'a' && (ch) <= 'z'))

#undef isdigit
#define isdigit(ch) \
	((ch) >= '0' && (ch) <= '9')

#if defined(OSF) || defined(CYGWIN)
#define isblank(ch) \
	(ch == ' ')
#endif

#define Ctrl(ch)	(ch & 037)

#define isprint2(c) (((c) & 0xe0) && ((c)!=0x7f))


#define setfilebuf(fp, buf) \
	setvbuf(fp, buf, _IOFBF, sizeof(buf));

/* bbs purposed marcos */

#define clear_line(l) { move(l, 0); clrtoeol(); }

#define search_record(filename, rptr, size, fptr, farg) \
	 search_record_forward(filename, rptr, size, 1, fptr, farg)

#define setboardfile(buf, bname, filename) \
	sprintf(buf, "boards/%s/%s", bname, filename);

#define sethomefile(buf, userid, filename) \
	sprintf(buf, "home/%c/%s/%s", mytoupper(userid[0]), userid, filename);

#define sethomefilewithpid(buf, userid, filename) \
	sprintf(buf, "home/%c/%s/%s.%d", mytoupper(userid[0]), userid, filename, getpid());

#define sethomepath(buf, userid) \
	sprintf(buf, "home/%c/%s", mytoupper(userid[0]), userid);

#define setmaildir(buf, userid) \
	sprintf(buf, "mail/%c/%s/.DIR", mytoupper(userid[0]), userid);

#define setmailfile(buf, filename) \
	sprintf(buf, "mail/%c/%s/%s", mytoupper(currentuser.userid[0]), currentuser.userid, filename);

#define setmailpath(buf, userid) \
	sprintf(buf, "mail/%c/%s", mytoupper(userid[0]), userid);

#define setquotefile(filepath) \
	strcpy(quote_file, filepath);

#define setuserfile(buf, filename) \
	sprintf(buf, "home/%c/%s/%s", mytoupper(currentuser.userid[0]), currentuser.userid, filename);

#define setvotefile(buf, bname, filename) \
	sprintf(buf, "vote/%s/%s", bname, filename);

/* betterman: consts for new account system 06.07 */
#define 	setdeptfile(buf, graduate) \
	sprintf(buf, "auth/%d/dept", graduate)

#define 	setauthfile(buf, graduate) \
	sprintf(buf, "auth/%d/%d", graduate,graduate)

#define refresh_bcache() { brdshm->uptime = 0; resolve_boards(); }

#define refresh_ucache() { uidshm->uptime = 0; resolve_ucache(); }

/* monster: ensure mail directory exists */
#define create_maildir(uident) { \
	sprintf(maildir, "mail/%c/%s", mytoupper(uident[0]), uident); \
	if (f_mkdir(maildir, 0755) == -1) \
		return -1; \
}

/* monster: following 4 macros may seem a bit odd, but they are essential to mmap operations */

#undef TRY
#undef CATCH
#undef BREAK
#undef END

#define TRY if (sigsetjmp(jmpbuf, 1) == 0) { signal(SIGBUS, sigfault); signal(SIGSEGV, sigfault);

#define CATCH } else {

#define BREAK signal(SIGBUS, SIG_IGN); signal(SIGSEGV, SIG_DFL);

#define END } signal(SIGBUS, SIG_IGN); signal(SIGSEGV, SIG_DFL);
