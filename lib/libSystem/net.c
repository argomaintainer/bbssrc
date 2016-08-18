#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

int
async_connect(char *address, int port, int timeout)
{
	struct sockaddr_in sin;
	struct hostent *h;
	struct timeval to;
	fd_set fds;
	int fd, flag, error = 0;
	socklen_t optlen = sizeof(socklen_t);

	// initialize socket and fill in necessary parameter
	if ((h = gethostbyname((address == NULL) ? "localhost" : address)) == NULL)
		return -1;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = PF_INET;
	memcpy(&sin.sin_addr, h->h_addr, h->h_length);
	sin.sin_port = htons(port);
        if ((fd = socket(sin.sin_family, SOCK_STREAM, 0)) == -1)
		return -1;

	// set timeout parameter
	memset(&to, 0, sizeof(to));
	to.tv_sec = timeout;

	// make the socket non-blocking
	flag = fcntl(fd, F_GETFL);
	fcntl(fd, F_SETFL, flag | O_NONBLOCK);
	if(connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0)
		return fd;

	while (1) {
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		switch (select(fd + 1, NULL, &fds, NULL, &to)) {
			case -1:	// error
				return -1;
			case 0:		// time out
				return -2;
			default:
				// if the socket is writable, possibly connect established
				if (FD_ISSET(fd, &fds))
					goto connect_finish;
				break;
		}
	}

connect_finish:

	getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &optlen);

	if (error == 0) {
		// reset the socket to blocking
		fcntl(fd, F_SETFL, flag);
		return fd;
	}

	return -1;
}
