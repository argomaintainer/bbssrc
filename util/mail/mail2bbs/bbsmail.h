/*
 *	Exit status codes for bbsmail mailer (from sendmail)
 *
 *      EX_DATAERR -- The input data was incorrect in some way.
 *              This should only be used for user's data & not
 *              system files.
 *      EX_NOUSER -- The user specified did not exist.  
 *      EX_CANTCREAT -- A (user specified) output file cannot be    
 *              created.
 *      EX_NOPERM -- You did not have sufficient permission to
 *              perform the operation.  This is not intended for
 *              file system problems, which should use NOINPUT or
 *              CANTCREAT, but rather for higher level permissions.   
 */

#define EX_OK          0       /* successful termination */
#define EX_DATAERR     65      /* data format error */
#define EX_NOUSER      67      /* addressee unknown */
#define EX_CANTCREAT   73      /* can't create (user) output file */ 
#define EX_NOPERM      77      /* permission denied */

int mailcheck(char *title, char *email);
