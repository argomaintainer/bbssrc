/*
 * Author: Tatu Ylonen <ylo@cs.hut.fi>
 * Copyright (c) 1995 Tatu Ylonen <ylo@cs.hut.fi>, Espoo, Finland
 *                    All rights reserved
 * Created: Sat Mar 18 05:11:38 1995 ylo
 * Password authentication.  This file contains the functions to check whether
 * the password is valid for the user.
 */

#include "includes.h"
//#include "packet.h"
#include "ssh.h"
//#include "servconf.h"
//#include "xmalloc.h"
//
extern struct userec currentuser;
extern char fromhost[60];
extern char raw_fromhost[60];

/*
 * Tries to authenticate the user using password.  Returns true if
 * authentication succeeds.
 */
int 
auth_password(const char *user, const char *password)
{
	struct sockaddr_in sin;
	char *host;
	int sinlen;

	if (!user || !password) return 0;
	resolve_ucache();
	resolve_utmp();

	if (!dosearchuser(user)) return 0;

	if (!checkpasswd2(password, &currentuser)) return 0;
	getpeername(packet_get_connection_in(), (struct sockaddr *) &sin,
		(void *) &sinlen);
	host = (char *) inet_ntoa(sin.sin_addr);
	if (host) strlcpy(fromhost, host, sizeof(fromhost));
}
