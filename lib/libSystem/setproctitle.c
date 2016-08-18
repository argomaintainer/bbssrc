#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "libSystem.h"

#ifndef HAVE_SETPROCTITLE
char **Argv = NULL;
char *LastArgv = NULL;

void
init_setproctitle(int argc, char **argv, char **envp)
{
	extern char **environ;

	int i, envpsize;
	char **p;

	for (i = envpsize = 0; envp[i] != ((void *) 0); i++)
		envpsize += strlen(envp[i]) + 1;

	if ((p =
	     (char **) malloc((i + 1) * sizeof (char *))) !=
	    ((void *) 0)) {
		environ = p;

		for (i = 0; envp[i] != ((void *) 0); i++) {
			if ((environ[i] =
			     malloc(strlen(envp[i]) + 1)) != ((void *) 0))
				strcpy(environ[i], envp[i]);
		}

		environ[i] = ((void *) 0);
	}

	Argv = argv;

	for (i = 0; i < argc; i++) {
		if (!i || (LastArgv + 1 == argv[i]))
			LastArgv = argv[i] + strlen(argv[i]);
	}

	for (i = 0; envp[i] != ((void *) 0); i++) {
		if ((LastArgv + 1) == envp[i])
			LastArgv = envp[i] + strlen(envp[i]);
	}

}

void
setproctitle(const char *fmt, ...)
{
	va_list msg;
	static char statbuf[8192];

	char *p;
	int i, maxlen = (LastArgv - Argv[0]) - 2;

	va_start((msg), fmt);

	memset(statbuf, 0, sizeof (statbuf));
	vsnprintf(statbuf, sizeof (statbuf), fmt, msg);
	va_end(msg);

	i = strlen(statbuf);
	snprintf(Argv[0], maxlen, "%s", statbuf);
	p = &Argv[0][i];

	while (p < LastArgv)
		*p++ = '\0';
	Argv[1] = ((void *) 0);
}
#endif
