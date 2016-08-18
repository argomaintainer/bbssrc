#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef OSF
#include <alloca.h>
#endif

#define MAX_ARGUMENT 128

int
cmd_exec(const char *cmd, const char *arg, ...)
{
	pid_t pid;
	int status;
	struct sigaction ignore, saveintr, savequit;
	sigset_t chldmask, savemask;

	size_t argv_max = 1024;
	const char **argv = alloca(argv_max * sizeof (const char *));
	unsigned int i;
	va_list args;

	argv[0] = arg;

	va_start(args, arg);
	i = 0;
	while (argv[i++] != NULL) {
		if (i == argv_max) {
			const char **nptr =
			    alloca((argv_max *=
				    2) * sizeof (const char *));

			if ((char *) nptr + argv_max == (char *) argv) {
				/* Stack grows down.  */
				argv = (const char **) memcpy(nptr, argv, i * sizeof (const char*));
				argv_max += i;
			} else if ((char *) argv + i == (char *) nptr)
				/* Stack grows up.  */
				argv_max += i;
			else
				/* We have a hole in the stack.  */
				argv = (const char **) memcpy(nptr, argv, i * sizeof (const char *));
		}

		argv[i] = va_arg(args, const char *);
	}
	va_end(args);

	ignore.sa_handler = SIG_IGN;	/* ignore SIGINT and SIGQUIT */
	sigemptyset(&ignore.sa_mask);
	ignore.sa_flags = 0;
	if (sigaction(SIGINT, &ignore, &saveintr) < 0)
		return (-1);
	if (sigaction(SIGQUIT, &ignore, &savequit) < 0)
		return (-1);

	sigemptyset(&chldmask);	/* now block SIGCHLD */
	sigaddset(&chldmask, SIGCHLD);
	if (sigprocmask(SIG_BLOCK, &chldmask, &savemask) < 0)
		return (-1);

	if ((pid = fork()) < 0) {
		status = -1;	/* probably out of processes */
	} else if (pid == 0) {	/* child */
		/* restore previous signal actions & reset signal mask */
		sigaction(SIGINT, &saveintr, NULL);
		sigaction(SIGQUIT, &savequit, NULL);
		sigprocmask(SIG_SETMASK, &savemask, NULL);

		execv(cmd, (char *const *)argv);
		_exit(127);	/* exec error */
	} else {		/* parent */
		while (waitpid(pid, &status, 0) < 0)
			if (errno != EINTR) {
				status = -1;	/* error other than EINTR from waitpid() */
				break;
			}
	}

	/* restore previous signal actions & reset signal mask */
	if (sigaction(SIGINT, &saveintr, NULL) < 0)
		return (-1);
	if (sigaction(SIGQUIT, &savequit, NULL) < 0)
		return (-1);
	if (sigprocmask(SIG_SETMASK, &savemask, NULL) < 0)
		return (-1);

	return (status);
}

