/*******************************************
 *              BBS PARAMETERS             *
 *******************************************/

#define STRLEN          80    	/* length of string buffer */
#define IDLEN           12	/* length of user id. */
#define BUFLEN          1024	/* length of general buffer */
#define TITLELEN        56	/* length of article title */
#define FNAMELEN        16	/* length of filename */
#define NAMELEN         20	/* length of realname */

#ifdef MULTILINE_MESSAGE

#define MSGSIZE			256	/* 一个messsage所占大小 */
#define MSGLINE			3	/* 消息界面所占行数 */
#define MSGLEN			180	/* 用户可输入的信息长度 */
#define MSGHEADP		7

#else  /* MULTILINE_MESSAGE */

#define MSGSIZE			129
#define MSGLINE			1
#define MSGLEN			55
#define MSGHEADP		12

#endif /* MULTILINE_MESSAGE */


#ifdef LONGNICKNAME
#define NICKNAMELEN		40	/* length of long nickname */
#else
#define NICKNAMELEN		20	/* length of normal nickname */
#endif

#define BTITLELEN               40	/* length of board title */
#define BMLEN                   40	/* length of bm list (BMLEN >= 3 * (IDLEN + 1)) */
#define BFNAMELEN               20	/* length of board name */

#define MD5_PASSLEN             16	/* length of encrypted password (MD5) */
#define DES_PASSLEN             14	/* length of encrypted password (DES) */
#define PASSLEN                 40	/* length of password */
#define RNDPASSLEN              10      /* 暗码认证的暗码长度 (适宜范围 4~10) */

#define MULTI_LOGINS		2       /* 同时可上站 ID 数 */
#define MAXGUEST            	256 	/* 最多 guest 帐号上站个数 */
#define MAXPERIP		10	/* 同IP同时可上站 ID 数 */

#define MAXFRIENDS 		200	/* 最大好友个数 */
#define MAXREJECTS 		32	/* 最大坏人个数 */

#define REG_EXPIRED         	180 	/* 重做身份确认期限 */

#define MAX_POSTRETRY        	2000
#define MAX_BOARD_POST  	8000	/* 普通版文章数上限 */
/* #define MAX_BOARD_POSTW 	8000 */	/* 水版文章数上限 */ /* 取消使用, by gcc */

#define MAX_BOARD_POST_II	12000	/* 第二类文章数上限 */

#define MORE_BUFSIZE       	4096
#define FILE_BUFSIZE        	200
#define FILE_MAXLINE         	25
#define MAX_WELCOME          	15 	/* 欢迎画面数 */
#define MAX_GOODBYE          	15 	/* 离站画面数 */
#define MAX_ISSUE            	15 	/* 最大进站画面数 */
#define MAX_DIGEST         	1500 	/* 最大文摘数 */

#ifndef BIGGER_MOVIE
#define MAXMOVIE		6  	/* 活动看版行数 (无边框) */
#else
#define MAXMOVIE		8  	/* 活动看版行数 (有边框) */
#endif

#define MAX_ACBOARD		15	/* 最大活动看版数 */

#define ACBOARD_BUFSIZE     	250
#define ACBOARD_MAXLINE        (MAX_ACBOARD * MAXMOVIE)
#define ENDLINE_BUFSIZE     	250
#define ENDLINE_MAXLINE      	32

#define MAXSIGLINES             6       /* 签名档最大行数 */

#define NUMPERMS		30

#define	MSQKEY			4716	/* key of message queue */

#define BBSNET_CONNECT_TIMEOUT	10
#define BBSNET_NOINPUT_TIMEOUT	300

#define TALK_CONNECT_TIMEOUT	5
#define CHAT_CONNECT_TIMEOUT	10

/*******************************************
 *          BBS RELATED CONSTANTS          *
 *******************************************/

/* filenames */
#define PASSFILE        BBSHOME"/.PASSWDS"
#define BOARDS          BBSHOME"/.BOARDS"
#define VISITLOG        BBSHOME"/reclog/.visitlog"
#define BADLOGINFILE    "logins.bad"
#define SYSU_IP_LIST    BBSHOME"/etc/sysu_ip.lst"

#define DOT_DIR     	".DIR"
#define THREAD_DIR  	".THREAD"
#define DIGEST_DIR  	".DIGEST"
#define MARKED_DIR  	".MARKEDDIR"
#define AUTHOR_DIR  	".AUTHORDIR"
#define KEY_DIR     	".KEYDIR"
#define DELETED_DIR	".DELETED"
#define JUNK_DIR	".JUNK"
#define DENY_DIR	".DENYLIST"

/* fileheader->flag */
#define FILE_READ		0x000001
#define FILE_OWND		0x000002
#define FILE_VISIT		0x000004
#define FILE_MARKED		0x000008	/* article is marked */
#define FILE_DIGEST		0x000010	/* article is added to digest */
#define FILE_FORWARDED		0x000020	/* article restored from recycle bin */
#define MAIL_REPLY		0x000020	/* mail replyed */
#define FILE_NOREPLY		0x000040	/* reply to the article is not allowed */
#define FILE_DELETED		0X000080
#define FILE_SELECTED		0x000100	/* article selected */
#define FILE_ATTACHED		0x000200	/* article comes with attachments */
#define FILE_RECOMMENDED	0x000400	/* article has been recommended */
#define FILE_MAIL		0x000800	/* send mail when replied */
#define FILE_OUTPOST		0x010000

/* fileheader->reserved[0] -- thread flag */
#define THREAD_BEGIN	0
#define THREAD_END	1
#define THREAD_OTHER	2

/* boardheader->flag */
#define VOTE_FLAG       0x000001
#define NOZAP_FLAG      0x000002
#define OUT_FLAG        0x000004
#define ANONY_FLAG      0x000008
#define NOREPLY_FLAG    0x000010
#define READONLY_FLAG   0x000020
#define JUNK_FLAG       0x000040
#define NOPLIMIT_FLAG   0x000080

#define BRD_READONLY    0x000100		  /* 只读 (不能发文, 只能删文和做标记 */
#define BRD_RESTRICT    0x000200		  /* 限制版 */
#define BRD_NOPOSTVOTE  0x000400		  /* 投票结果不公开 */
#define BRD_ATTACH	0x000800		  /* 允许上传附件 */
#define BRD_GROUP	0x001000		  /* 版面列表 */
#define BRD_HALFOPEN	0x002000		  /* changed by freestyler: 激活用户可访问 */
#define BRD_INTERN  	0x004000		  /* Added by betterman :仅限校内访问 */
#define BRD_MAXII_FLAG	0x008000		  /* Added by gcc: 增加第二类文章数上限 */

/* announce.c */
#define ANN_FILE                0x01              /* 普通文件 */
#define ANN_DIR                 0x02              /* 普通目录 */
#define ANN_PERSONAL            0x04              /* 个人文集目录 */
#define ANN_GUESTBOOK           0x08              /* 留言本 */
#define ANN_LINK                0x10              /* Local Link */
#define ANN_RLINK               0x20              /* Remote Link (unused) */
#define ANN_SELECTED            0x100             /* 被选择 */
#define ANN_ATTACHED		0x200		  /* 带有附件 */
#define ANN_RESTRICT            0x010000          /* 限制性文件/目录 */
#define ANN_READONLY            0x020000          /* 只读 (不能修改属性/内容) */

#define ANN_COPY		0		  /* 拷贝 */
#define ANN_CUT			1		  /* 剪切 */
#define ANN_MOVE		2		  /* 改变次序 */
#define ANN_EDIT		3		  /* 编辑文件 */
#define ANN_CREATE		4		  /* 增添条目 */
#define ANN_DELETE		5		  /* 删除条目 */
#define ANN_CTITLE		6		  /* 更改标题 */
#define ANN_ENOTES		7		  /* 编辑备忘录 */
#define ANN_DNOTES		8		  /* 删除备忘录 */
#define ANN_INDEX		9		  /* 生成精华区索引 */

/* read.c */
#define DONOTHING       0       /* Read menu command return states */
#define FULLUPDATE      1       /* Entire screen was destroyed in this oper */
#define PARTUPDATE      2       /* Only the top three lines were not destroyed */
#define DOQUIT          3       /* Exit read menu was executed */
#define NEWDIRECT       4       /* Directory has changed, re-read files */
#define READ_NEXT       5       /* Direct read next file */
#define READ_PREV       6       /* Direct read prev file */
#define GOTO_NEXT       7       /* Move cursor to next */
#define DIRCHANGED      8       /* Index file was changed */
#define MODECHANGED     9       /* ... */
#define NEWDIRECT2      10      /* Directory has changed, re-read files and jump to spec pnt*/
#define PREUPDATE       11      /* post preview */

/* user_info->pager */
#define ALL_PAGER       0x1
#define FRIEND_PAGER    0x2
#define ALLMSG_PAGER    0x4
#define FRIENDMSG_PAGER 0x8

/* userec->flags[0] */
#define PAGER_FLAG      0x01    /* true if pager was OFF last session */
#define CLOAK_FLAG      0x02    /* true if cloak was ON last session */
#define BRDSORT_FLAG    0x20    /* true if the boards sorted alphabetical */
#ifdef INBOARDCOUNT
#define BRDSORT_FLAG2   0x10    /* true if the boards sorted according the number of inboard users  */
#endif

/* apply_record */
#define QUIT            0x666	/* to terminate apply_record */

/* I/O control */
#define I_TIMEOUT   	-2	/* used for the getchar routine select call */
#define I_OTHERDATA 	-333	/* interface, (-3) will conflict with chinese */

/* board rc */
#define BRC_MAXSIZE     50000
#define BRC_MAXNUM      60
#define BRC_STRLEN      BFNAMELEN
#define BRC_ITEMSIZE    (BRC_STRLEN + 1 + BRC_MAXNUM * sizeof( int ))
#define GOOD_BRC_NUM    50

/* display */
#define BBS_PAGESIZE	(t_lines - 4)


/* Pudding: cosnts for AUTHHOST */
#ifdef AUTHHOST
#define HOST_AUTH_YEA		0xffffffff
#define HOST_AUTH_NA		0
#define UNAUTH_PERMMASK		(~(PERM_POST))
#endif

/* betterman: consts for new account system 06.07 */
#define MULTIAUTH 3
#define MAXMAIL 3

/*******************************************
 *                ANSI CODES               *
 *******************************************/

#define   ANSI_RESET    "\033[m"
#define   ANSI_REVERSE  "\033[7m\033[4m"

/*******************************************
 *	  KEYBOARD RELATED CONSTANTS       *
 *******************************************/

#define EXTEND_KEY
#define KEY_TAB         9
#define KEY_ESC         27
#define KEY_UP          0x0101
#define KEY_DOWN        0x0102
#define KEY_RIGHT       0x0103
#define KEY_LEFT        0x0104
#define KEY_HOME        0x0201
#define KEY_INS         0x0202
#define KEY_DEL         0x0203
#define KEY_END         0x0204
#define KEY_PGUP        0x0205
#define KEY_PGDN        0x0206

/*******************************************
 *        SCREEN Related CONSTANTS         *
 *******************************************/

#define LINELEN		256	/* maxinum length of a single line */

/* line buffer modes */
#define MODIFIED	1	/* if line has been modifed, output to screen */
#define STANDOUT	2	/* if this line has a standout region */

/*******************************************
 *        FUNCTION RELATED CONSTANTS       *
 *******************************************/

/* general constants */
#define YEA		1
#define NA		0

#define TRUE		1
#define FALSE		0

#define CRLF		"\r\n"

/* addtodeny & delfromdeny */
#define D_ANONYMOUS     0x01    /* 匿名封禁 */
#define D_NOATTACH      0x02    /* 无附文 (直接封禁) */
#define D_FULLSITE      0x04    /* 封禁全站 */
#define D_NODENYFILE    0x08    /* 不生成封禁记录文件 */
#define D_IGNORENOUSER	0x10	/* 忽略用户不存在的错误 */

/* getdata */
#define DOECHO		1
#define NOECHO		0

/* getfilename/getdirname */
#define GFN_FILE        0x00
#define GFN_LINK        0x01
#define GFN_UPDATEID    0x02	/* update article id */
#define GFN_SAMETIME	0x04
#define GFN_NOCLOSE	0x08

/* locate_article and bm functions */

#define LOCATE_THREAD		0x01
#define LOCATE_AUTHOR		0x02
#define LOCATE_TITLE		0x04
#define LOCATE_TEXT		0x08
#define LOCATE_SELECTED		0x10
#define LOCATE_ANY		0x20

#define LOCATE_FIRST		0x100
#define LOCATE_LAST		0x200
#define LOCATE_PREV		0x400
#define LOCATE_NEXT		0x800
#define LOCATE_NEW		0x1000

/* process_records */
#define KEEPRECORD		0
#define REMOVERECORD		1

/* listedit (used by its callback function */
#define	LE_ADD			0
#define	LE_REMOVE		1

#define MAX_IDLIST		3

/* vedit */
#define EDIT_NONE		0x00
#define	EDIT_SAVEHEADER		0x01
#define EDIT_MODIFYHEADER	0x02
#define EDIT_ADDLOGINFO		0x04

/* more */
#define MORE_NONE		0x00
#define MORE_STUFF		0x01
#define MORE_MSGVIEW		0x02
#define MORE_ATTACHMENT		0x04

/*******************************************
 *           GLOSSARY CONSTANTS            *
 *******************************************/

#ifndef NOEXP

/* 描述经验值等级 */
#define GLY_CEXP0               "没等级"
#define GLY_CEXP1               "新手上路"
#define GLY_CEXP2               "一般站友"
#define GLY_CEXP3               "中级站友"
#define GLY_CEXP4               "高级站友"
#define GLY_CEXP5               "老站友"
#define GLY_CEXP6               "长老级"
#define GLY_CEXP7               "本站元老"
#define GLY_CEXP8               "开国大老"

/* 描述文章数等级 */
#define GLY_CPOST0              "没写文章"
#define GLY_CPOST1              "文采一般"
#define GLY_CPOST2              "文采奕奕"
#define GLY_CPOST3              "文坛高手"
#define GLY_CPOST4              "文坛博士"
#define GLY_CPOST5              "文坛至尊"

/* 描述表现值等级 */
#define GLY_CPERF0              "没等级"
#define GLY_CPERF1              "赶快加油"
#define GLY_CPERF2              "努力中"
#define GLY_CPERF3              "还不错"
#define GLY_CPERF4              "很好"
#define GLY_CPERF5              "优等生"
#define GLY_CPERF6              "太优秀了"
#define GLY_CPERF7              "本站支柱"
#define GLY_CPERF8              "神～～"

#endif
