#include <unistd.h>
#include <string.h>

#include "mongoose/mongoose.h"
#include "http_handler.h"
#include "socket_io.h"
#include "logging.h"

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
    struct http_message request;

    int nread;
    nread = socket_read(sock_client, ctx->buf, ctx->bufsize);
    if (nread < 0) {
        return -1;
    }
    debug("Read %d\n", nread);

    nread = mg_parse_http(ctx->buf, nread, &request, 1);
    if (nread <= 0) {
        return -1;
    }
    debug("Successfully parsed request\n");


    if (strncmp(request.method.p, GET, request.method.len) == 0) {
        // Pretend to do some work for 100ms
        usleep(100000);
        return socket_write(sock_client, RESPONSE_BODY, strlen(RESPONSE_BODY));
    } else {
        return -2;
    }
}

