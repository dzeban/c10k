#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "config.h"
#include "http_handler.h"

int main(int argc, const char *argv[])
{
    int sock_listen;
    int rc = EXIT_SUCCESS;
    struct sockaddr_in address, peer_address;
    socklen_t peer_address_len;

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = 8282;
    if (!inet_aton("0.0.0.0", &address.sin_addr)) {
        perror("inet_aton");
        rc = EXIT_FAILURE;
        goto exit_rc;
    }

    sock_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listen < 0) {
        perror("socket");
        rc = EXIT_FAILURE;
        goto exit_rc;
    }

    int yes = 1;
    if (setsockopt(sock_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        rc = EXIT_FAILURE;
        goto exit_socket;
    }


    if (bind(sock_listen, (struct sockaddr *)&address, sizeof(address))) {
        perror("bind");
        rc = EXIT_FAILURE;
        goto exit_socket;
    }

    if (listen(sock_listen, BACKLOG)) {
        perror("listen");
        rc = EXIT_FAILURE;
        goto exit_socket;
    }

    while (1) {
        int sock_client;
        struct handler_ctx *ctx;
        sock_client = accept(sock_listen, (struct sockaddr *)&peer_address, &peer_address_len);

        fprintf(stderr, "Client accept\n");

        ctx = handler_init();
        http_handler(sock_client, ctx);
        handler_destroy(ctx);

        close(sock_client);
    }

exit_socket:
    close(sock_listen);
exit_rc:
    exit(rc);
}
