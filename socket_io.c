#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "socket_io.h"
#include "logging.h"

int socket_read(int sock, char *buf, size_t bufsize)
{
    int nleft, nread;
    char *ptr;

    ptr = buf;
    nleft = bufsize;
    while (nleft > 0) {
        nread = read(sock, ptr, nleft);
        debug("nread %d\n", nread);
        if (nread < 0) {
			debug("errno %s\n", strerror(errno));
            if (errno == EINTR) {
                nread = 0; // call read again
            } else {
                return -1;
            }
		} else if (nread == 0) {
			// EOF
			break;
        } else {
            // Detect end of request
            if (strstr(ptr, "\r\n\r\n") != NULL)
                break;
        }

        nleft -= nread;
        ptr += nread;
    }

    debug("Successfully read %d\n", nread);
    return nread;
}

int socket_write(int sock, const char *buf, size_t bufsize)
{
    int nleft, nwritten;
    const char *ptr;

    ptr = buf;
    nleft = bufsize;
    while (nleft > 0) {
        nwritten = write(sock, ptr, nleft);
        debug("nwritten %d\n", nwritten);
        if (nwritten < 0) {
			debug("errno %s\n", strerror(errno));
			if (errno == EINTR) {
				nwritten = 0; // call write again
			} else {
				return -1;
			}
        } else if (nwritten == 0) {
			// From write (2) man page:
			// > If count is zero and fd refers to a file other than a regular
			// > file, the results are not specified.
			// We're writing to socket, so we can't say for sure what's wrong,
			// therefore we'll fail.
			return -1;
		}

        nleft -= nwritten;
        ptr += nwritten;
    }

    debug("Successfully written %d\n", nwritten);
    return 0;
}
