#ifndef _HTTP_HANDLER_H
#define _HTTP_HANDLER_H

#define BUFSIZE 4096
#define GET "GET"
#define RESPONSE_BODY "HTTP/1.1 200 OK\r\nCache: no-cache\r\nContent-Type: text/html\r\nContent-Length: 44\r\n\r\n<html><body><h1>It works!</h1></body></html>"

struct handler_ctx {
    char *buf;
    size_t bufsize;
};

struct handler_ctx *handler_init();
void handler_destroy(struct handler_ctx *ctx);
int http_handler(int sock_client, struct handler_ctx *ctx);
int http_handler_loop(int sock_client);

#endif
