/* config.h.  Generated automatically by configure.  */
/* config.h.in.  Generated automatically from configure.in by autoheader.  */
#ifndef _CONFIG_H
#define _CONFIG_H

/* Generated automatically from acconfig.h by autoheader. */
/* Please make your changes there */


/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define if you want to disable PAM support */
#define DISABLE_PAM

/* Define if you want to disable lastlog support */
/* #undef DISABLE_LASTLOG */

/* Location of lastlog file */
#define LASTLOG_LOCATION "/var/log/lastlog"

/* If lastlog is a directory */
/* #undef LASTLOG_IS_DIR */

/* Location of random number pool  */
#define RANDOM_POOL "/dev/urandom"

/* Are we using the Entropy gathering daemon */
/* #undef HAVE_EGD */

/* Define if your ssl headers are included with #include <ssl/header.h>  */
/* #undef HAVE_SSL */

/* Define if your ssl headers are included with #include <openssl/header.h>  */
#define HAVE_OPENSSL 1

/* struct utmp and struct utmpx fields */
#define HAVE_HOST_IN_UTMP 1
/* #undef HAVE_HOST_IN_UTMPX */
/* #undef HAVE_ADDR_IN_UTMP */
/* #undef HAVE_ADDR_IN_UTMPX */
/* #undef HAVE_ADDR_V6_IN_UTMP */
/* #undef HAVE_ADDR_V6_IN_UTMPX */
/* #undef HAVE_SYSLEN_IN_UTMPX */
/* #undef HAVE_PID_IN_UTMP */
/* #undef HAVE_TYPE_IN_UTMP */
/* #undef HAVE_TV_IN_UTMP */
/* #undef HAVE_ID_IN_UTMP */

/* Define if you want to use utmpx */
/* #undef USE_UTMPX */

/* Define is libutil has login() function */
#define HAVE_LIBUTIL_LOGIN 1

/* Define if libc defines __progname */
#define HAVE___PROGNAME 1

/* Define if you want Kerberos 4 support */
/* #undef KRB4 */

/* Define if you want AFS support */
/* #undef AFS */

/* Define if you want S/Key support */
/* #undef SKEY */

/* Define if you want TCP Wrappers support */
/* #undef LIBWRAP */

/* Define if your libraries define login() */
#define HAVE_LOGIN 1

/* Define if your libraries define daemon() */
#define HAVE_DAEMON 1

/* Define if your libraries define getpagesize() */
#define HAVE_GETPAGESIZE 1

/* Define if xauth is found in your path */
/* #undef XAUTH_PATH */

/* Define if rsh is found in your path */
#define RSH_PATH "/usr/bin/rsh"

/* Define if you want to allow MD5 passwords */
/* #undef HAVE_MD5_PASSWORDS */

/* Define if you want to disable shadow passwords */
/* #undef DISABLE_SHADOW */

/* Define if you want have trusted HPUX */
/* #undef HAVE_HPUX_TRUSTED_SYSTEM_PW */

/* Define if you have an old version of PAM which takes only one argument */
/* to pam_strerror */
/* #undef HAVE_OLD_PAM */

/* Set this to your mail directory if you don't have maillock.h */
#define MAIL_DIRECTORY "/var/mail"

/* Data types */
#define HAVE_INTXX_T 1
#define HAVE_U_INTXX_T 1
/* #undef HAVE_UINTXX_T */
#define HAVE_SOCKLEN_T 1
#define HAVE_SIZE_T 1
#define HAVE_STRUCT_SOCKADDR_STORAGE 1
#define HAVE_STRUCT_ADDRINFO 1
#define HAVE_STRUCT_IN6_ADDR 1
#define HAVE_STRUCT_SOCKADDR_IN6 1

/* Fields in struct sockaddr_storage */
#define HAVE_SS_FAMILY_IN_SS 1
/* #undef HAVE___SS_FAMILY_IN_SS */

/* Define if you have /dev/ptmx */
/* #undef HAVE_DEV_PTMX */

/* Define if you have /dev/ptc */
/* #undef HAVE_DEV_PTS_AND_PTC */

/* Define if you need to use IP address instead of hostname in $DISPLAY */
/* #undef IPADDR_IN_DISPLAY */

/* Specify default $PATH */
/* #undef USER_PATH */

/* Specify location of ssh.pid */
#define PIDDIR "/var/run"

/* Use IPv4 for connection by default, IPv6 can still if explicity asked */
/* #undef IPV4_DEFAULT */

/* getaddrinfo is broken (if present) */
/* #undef BROKEN_GETADDRINFO */

/* Workaround more Linux IPv6 quirks */
/* #undef DONT_TRY_OTHER_AF */

/* Detect IPv4 in IPv6 mapped addresses and treat as IPv4 */
/* #undef IPV4_IN_IPV6 */

/* The number of bytes in a char.  */
#define SIZEOF_CHAR 1

/* The number of bytes in a int.  */
#define SIZEOF_INT 4

/* The number of bytes in a long int.  */
#define SIZEOF_LONG_INT 4

/* The number of bytes in a long long int.  */
#define SIZEOF_LONG_LONG_INT 8

/* The number of bytes in a short int.  */
#define SIZEOF_SHORT_INT 2

/* Define if you have the _getpty function.  */
/* #undef HAVE__GETPTY */

/* Define if you have the arc4random function.  */
#define HAVE_ARC4RANDOM 1

/* Define if you have the bindresvport_af function.  */
/* #undef HAVE_BINDRESVPORT_AF */

/* Define if you have the freeaddrinfo function.  */
#define HAVE_FREEADDRINFO 1

/* Define if you have the gai_strerror function.  */
#define HAVE_GAI_STRERROR 1

/* Define if you have the getaddrinfo function.  */
#define HAVE_GETADDRINFO 1

/* Define if you have the getnameinfo function.  */
#define HAVE_GETNAMEINFO 1

/* Define if you have the innetgr function.  */
#define HAVE_INNETGR 1

/* Define if you have the md5_crypt function.  */
/* #undef HAVE_MD5_CRYPT */

/* Define if you have the mkdtemp function.  */
#define HAVE_MKDTEMP 1

/* Define if you have the openpty function.  */
#define HAVE_OPENPTY 1

/* Define if you have the rresvport_af function.  */
#define HAVE_RRESVPORT_AF 1

/* Define if you have the setenv function.  */
#define HAVE_SETENV 1

/* Define if you have the seteuid function.  */
#define HAVE_SETEUID 1

/* Define if you have the setlogin function.  */
#define HAVE_SETLOGIN 1

/* Define if you have the setproctitle function.  */
#define HAVE_SETPROCTITLE 1

/* Define if you have the setreuid function.  */
#define HAVE_SETREUID 1

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1

/* Define if you have the strlcat function.  */
#define HAVE_STRLCAT 1

/* Define if you have the strlcpy function.  */
#define HAVE_STRLCPY 1

/* Define if you have the updwtmpx function.  */
/* #undef HAVE_UPDWTMPX */

/* Define if you have the vsnprintf function.  */
#define HAVE_VSNPRINTF 1

/* Define if you have the <bstring.h> header file.  */
/* #undef HAVE_BSTRING_H */

/* Define if you have the <endian.h> header file.  */
/* #undef HAVE_ENDIAN_H */

/* Define if you have the <krb.h> header file.  */
/* #undef HAVE_KRB_H */

/* Define if you have the <lastlog.h> header file.  */
/* #undef HAVE_LASTLOG_H */

/* Define if you have the <login.h> header file.  */
/* #undef HAVE_LOGIN_H */

/* Define if you have the <maillock.h> header file.  */
/* #undef HAVE_MAILLOCK_H */

/* Define if you have the <netdb.h> header file.  */
#define HAVE_NETDB_H 1

/* Define if you have the <netgroup.h> header file.  */
/* #undef HAVE_NETGROUP_H */

/* Define if you have the <paths.h> header file.  */
#define HAVE_PATHS_H 1

/* Define if you have the <poll.h> header file.  */
#define HAVE_POLL_H 1

/* Define if you have the <pty.h> header file.  */
/* #undef HAVE_PTY_H */

/* Define if you have the <security/pam_appl.h> header file.  */
#define HAVE_SECURITY_PAM_APPL_H 1

/* Define if you have the <shadow.h> header file.  */
/* #undef HAVE_SHADOW_H */

/* Define if you have the <stddef.h> header file.  */
#define HAVE_STDDEF_H 1

/* Define if you have the <sys/bitypes.h> header file.  */
/* #undef HAVE_SYS_BITYPES_H */

/* Define if you have the <sys/bsdtty.h> header file.  */
/* #undef HAVE_SYS_BSDTTY_H */

/* Define if you have the <sys/cdefs.h> header file.  */
#define HAVE_SYS_CDEFS_H 1

/* Define if you have the <sys/poll.h> header file.  */
#define HAVE_SYS_POLL_H 1

/* Define if you have the <sys/select.h> header file.  */
#define HAVE_SYS_SELECT_H 1

/* Define if you have the <sys/stropts.h> header file.  */
/* #undef HAVE_SYS_STROPTS_H */

/* Define if you have the <sys/sysmacros.h> header file.  */
/* #undef HAVE_SYS_SYSMACROS_H */

/* Define if you have the <sys/time.h> header file.  */
#define HAVE_SYS_TIME_H 1

/* Define if you have the <sys/ttcompat.h> header file.  */
/* #undef HAVE_SYS_TTCOMPAT_H */

/* Define if you have the <util.h> header file.  */
/* #undef HAVE_UTIL_H */

/* Define if you have the <utmp.h> header file.  */
#define HAVE_UTMP_H 1

/* Define if you have the <utmpx.h> header file.  */
/* #undef HAVE_UTMPX_H */

/* Define if you have the dl library (-ldl).  */
/* #undef HAVE_LIBDL */

/* Define if you have the krb library (-lkrb).  */
/* #undef HAVE_LIBKRB */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the resolv library (-lresolv).  */
/* #undef HAVE_LIBRESOLV */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Define if you have the z library (-lz).  */
#define HAVE_LIBZ 1

/* ******************* Shouldn't need to edit below this line ************** */

#include "defines.h"

#endif /* _CONFIG_H */
