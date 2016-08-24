#include <unistd.h>
#include <string.h>

#include "mongoose/mongoose.h"
#include "http_handler.h"

int read_request(int sock_client, struct handler_ctx *ctx)
{
    int nleft, nread;
    char *buf;

    buf = ctx->buf;
    nleft = ctx->bufsize;
    while (nleft > 0) {
        nread = read(sock_client, buf, nleft);
        fprintf(stderr, "nread %d\n", nread);
        if (nread < 0) {
            if (errno == EINTR) {
                nread = 0; // call read again
            } else {
                return -1;
            }
        } else {
            // Detect end of request
            if (strstr(buf, "\r\n\r\n") != NULL)
                break;
        }

        nleft -= nread;
        buf += nread;
    }
    return nread;
}

int write_response(int sock_client, const char *response, size_t n)
{
    int nleft, nwritten;
    const char *ptr;

    ptr = response;
    nleft = n;
    while (nleft > 0) {
        nwritten = write(sock_client, ptr, nleft);
        if (nwritten < 0 && errno == EINTR) {
            nwritten = 0; // call write again
        }

        nleft -= nwritten;
        ptr += nwritten;
    }

    fprintf(stderr, "Successfully written %d\n", nwritten);
    return 0;
}

struct handler_ctx *handler_init()
{
    struct handler_ctx *ctx = malloc(sizeof(*ctx));
    if (!ctx) {
        perror("malloc");
        return NULL;
    }

    ctx->bufsize = BUFSIZE;
    ctx->buf = malloc(ctx->bufsize);
    if (!ctx->buf) {
        perror("malloc");
        free(ctx);
        return NULL;
    }

    return ctx;
}

void handler_destroy(struct handler_ctx *ctx)
{
    free(ctx->buf);
    free(ctx);
}

int http_handler(int sock_client, struct handler_ctx *ctx)
{
    struct http_message request, response;

    int nread;
    nread = read_request(sock_client, ctx);
    if (nread < 0) {
        return -1;
    }
    fprintf(stderr, "Read %d\n", nread);

    nread = mg_parse_http(ctx->buf, nread, &request, 1);
    if (nread <= 0) {
        return -1;
    }
    fprintf(stderr, "Successfully parsed request\n");

    // We response for any "GET" request
    return write_response(sock_client, RESPONSE_BODY, strlen(RESPONSE_BODY));
}

