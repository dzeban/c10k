#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <evhttp.h>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>

#include "config.h"

struct context {
    struct event_base *base;
    struct evhttp_connection **connections;
    long nconnections;
    long delay;
};

void http_request_done(struct evhttp_request *req, void *arg)
{
    struct context *ctx = (struct context *)arg;

    evbuffer_drain(req->input_buffer, 1024);
    sleep(ctx->delay);
    event_base_loopexit(ctx->base, NULL);
}

int main(int argc, char const *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <addr> <port> <connections> <delay>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    const char *addr = argv[1];
    int port = strtol(argv[2], NULL, 0);

    struct context ctx;

    ctx.nconnections = strtol(argv[3], NULL, 0);
    ctx.connections = calloc(ctx.nconnections, sizeof(struct evhttp_connection *));
    ctx.delay = strtol(argv[4], NULL, 0);
    ctx.base = event_base_new();

    struct evhttp_request *req;
    for (size_t i = 0; i < ctx.nconnections; i++) {
        ctx.connections[i] =
            evhttp_connection_base_new(ctx.base, NULL, addr, port);

        req = evhttp_request_new(http_request_done, &ctx);
        evhttp_make_request(ctx.connections[i], req, EVHTTP_REQ_GET, "/");
        evhttp_connection_set_timeout(req->evcon, 600);
    }

    event_base_dispatch(ctx.base);
    for (size_t i = 0; i < ctx.nconnections; i++) {
        evhttp_connection_free(ctx.connections[i]);
    }
    event_base_free(ctx.base);

    free(ctx.connections);
    return 0;
}
