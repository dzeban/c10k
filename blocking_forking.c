#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "config.h"
#include "http_handler.h"
#include "logging.h"

static void sigchld_handler(int sig, siginfo_t *si, void *unused)
{
	// Signals are not queued, meaning that SIGCHLD from multiple child
	// processes will be discarded. To prevent zombies, we have to call waitpid
	// in a loop for all pids
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main(int argc, const char *argv[])
{
    int sock_listen;
    int rc = EXIT_SUCCESS;
    struct sockaddr_in address, peer_address;
    socklen_t peer_address_len;
	struct sigaction sa;

    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(8282);
	address.sin_addr.s_addr = htonl(INADDR_ANY);

	// Setup signal handler to catch SIGCHLD and avoid zombie children
	sa.sa_flags = SA_SIGINFO;
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = sigchld_handler;
	if (sigaction(SIGCHLD, &sa, NULL)) {
		perror("sigaction");
		rc = EXIT_FAILURE;
		goto exit_rc;
	}

	// Make listen socket binded to port 8282
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
		pid_t child;

        sock_client = accept(sock_listen, (struct sockaddr *)&peer_address, &peer_address_len);
		if (sock_client < 0) {
			// "accept" is a "slow" system call, if signal handling happened
			// while accept was working we'll receive EINTR. And it's ok.
			// On Linux it can be automatically restarted by providing
			// SA_RESTART to signal handler and NOT specifying SO_RCVTIMEO.
			// But we opt to make more portable and restart it by hand.
			if (errno != EINTR) {
				perror("accept");
			}
			continue;
		}
		debug("Accept from %s, sock %d\n", inet_ntoa(peer_address.sin_addr), sock_client);

		child = fork();
		switch (child) {
			case 0: // Child process
				if (close(sock_listen)) {
					perror("close sock_listen");
				}

				ctx = handler_init();
				http_handler(sock_client, ctx);
				handler_destroy(ctx);

				if (close(sock_client)) {
					perror("close");
				}

				debug("About to exit from %d\n", getpid());
				exit(EXIT_SUCCESS);
				break;

			case -1: // Error
				perror("fork");
				if (close(sock_client)) {
					perror("close sock_client on error");
				}
				break;

			default: // Parent
				// Parent process
				if (close(sock_client)) {
					perror("close sock_client in parent");
				}
				break;
		}
    }

exit_socket:
    close(sock_listen);
exit_rc:
    exit(rc);
}
