#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "socket_io.h"

#define REQUEST "GET / HTTP/1.1\r\nHost: 0.0.0.0:8282\r\nUser-Agent: curl/7.43.0\r\nAccept: */*\r\n\r\n"

int main(int argc, const char *argv[])
{
    int sock;
    struct sockaddr_in addr_in;
    char *buf;
    const int bufsize = 4096;
    int rc = EXIT_SUCCESS;

    buf = malloc(bufsize);
    if (!buf) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(buf, 0, bufsize);

    memset(&addr_in, 0, sizeof(addr_in));
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = htons(PORT);
    addr_in.sin_addr.s_addr = inet_addr("0.0.0.0");

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        rc = EXIT_FAILURE;
        goto exit_free;
    }

    if (connect(sock, (struct sockaddr *)&addr_in, sizeof(addr_in)) < 0) {
        perror("connect");
        rc = EXIT_FAILURE;
        goto exit_sock;
    }

    if (socket_write(sock, REQUEST, strlen(REQUEST))) {
        perror("socket_write");
        rc = EXIT_FAILURE;
        goto exit_sock;
    }

    if (socket_read(sock, buf, bufsize) < 0) {
        perror("socket_read");
        rc = EXIT_FAILURE;
        goto exit_sock;
    }

    puts(buf);

exit_sock:
    close(sock);
exit_free:
    free(buf);
    exit(rc);
}
