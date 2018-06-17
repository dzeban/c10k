#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>

#include <uv.h>

#define log(fmt, ...) do { \
    fprintf(stderr, "location=\"%s:%d:%s\" level=INFO msg=\"" fmt "\"\n", __FILE__, __LINE__,  __FUNCTION__, ##__VA_ARGS__); \
} while(0)

#define err(fmt, ...) do { \
    fprintf(stderr, "location=%s:%d:%s level=ERROR msg=\"" fmt "\"\n", __FILE__,  __LINE__, __FUNCTION__, ##__VA_ARGS__); \
} while(0)

#define goto_uv_err(status, label) do { \
    if (status < 0) { \
        fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, uv_err_name(status), uv_strerror(status)); \
        goto label; \
    } \
} while(0)

#define return_uv_err(status) do { \
    if (status < 0) { \
        fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, uv_err_name(status), uv_strerror(status)); \
        return; \
    } \
} while(0)

#define return_rc_uv_err(status, rc) do { \
    if (status < 0) { \
        fprintf(stderr, "%s:%d: %s: %s\n", __FILE__, __LINE__, uv_err_name(status), uv_strerror(status)); \
        return rc; \
    } \
} while(0)

// Set "Connection: close" header to avoid HTTP keep-alive and hint the server
// to close connection. Without it we'll wait for EOF until keepalive timeout.
// The other option here is to use HTTP/1.0
char *GET_REQUEST = "GET /index.html HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";

static void empty_close_cb(uv_handle_t *handle __attribute__((unused)))
{
    log("invoked close_cb");
}

static void alloc_cb(uv_handle_t *handle __attribute__((unused)), size_t suggested_size, uv_buf_t *buf)
{
    char *b = calloc(1, suggested_size);
    if (!b) {
        // Trigger UV_ENOBUFS error in read cb
        buf->base = NULL;
        buf->len = 0;
    }

    buf->base = b;
    buf->len = suggested_size;
}

static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf)
{
    if (nread > 0) {
        printf("%s", buf->base);
    } else if (nread == UV_EOF) {
        uv_read_stop(stream);
    } else {
        return_uv_err(nread);
    }

    free(buf->base);
}

static void on_write(uv_write_t *req, int status)
{
    return_uv_err(status);

    uv_read_start(req->handle, alloc_cb, on_read);
}

static void timer_cb(uv_timer_t *timer)
{
    uv_connect_t *connection = uv_handle_get_data((uv_handle_t *)timer);
    if (!connection) {
        err("invalid timer data: no connection handle");
        return;
    }

    free(timer);

    uv_buf_t bufs[] = {
        { .base = GET_REQUEST, .len = strlen(GET_REQUEST) },
    };

    log("sending http request");
    uv_write_t write_req;
    uv_write(&write_req, connection->handle, bufs, 1, on_write);
}

static void on_connect(uv_connect_t *connection, int status)
{
    int rc = 0;

    return_uv_err(status);
    log("connected");

    uv_loop_t *loop = uv_default_loop();
    if (!loop) {
        err("no loop in connection handle");
        return;
    }

    // Setup timer to for sleep
    uv_timer_t *sleep_timer = malloc(sizeof(*sleep_timer));
    if (!sleep_timer) {
        perror("malloc");
        return;
    }

    rc = uv_timer_init(loop, sleep_timer);
    return_uv_err(rc);

    uv_handle_set_data((uv_handle_t *)sleep_timer, connection);

    log("starting timer");
    int *delay = uv_handle_get_data((uv_handle_t *)connection);
    if (!delay) {
        err("invalid connection data: no delay");
        return;
    }
    rc = uv_timer_start(sleep_timer, timer_cb, *delay * 1000, 0);
    return_uv_err(rc);
}

int main(int argc, const char *argv[])
{
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "Usage: %s <addr> [delay]\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "   addr  - where to connect like 127.0.0.1\n");
        fprintf(stderr, "   delay - delay between connection and request in seconds (default is 5)\n");
        return -1;
    }

    int delay = 5;
    if (argc == 3) {
        delay = strtol(argv[2], NULL, 0);
    }

    int rc = -1;

    uv_loop_t *loop = uv_default_loop();

    uv_tcp_t *socket = malloc(sizeof(*socket));
    if (!socket) {
        perror("malloc");
        return -1;
    }

    rc = uv_tcp_init(loop, socket);
    goto_uv_err(rc, exit);

    uv_connect_t *connection = malloc(sizeof(*connection));
    if (!connection) {
        perror("malloc");
        goto exit;
    }

    uv_handle_set_data((uv_handle_t *)connection, &delay);

    struct sockaddr_in dest;
    rc = uv_ip4_addr(argv[1], 80, &dest);
    goto_uv_err(rc, exit);

    rc = uv_tcp_connect(connection, socket, (const struct sockaddr *)&dest, on_connect);
    goto_uv_err(rc, exit);

    rc = uv_run(loop, UV_RUN_DEFAULT);

exit:
    uv_loop_close(loop);

    if (connection) {
        free(connection);
    }

    if (!uv_is_closing((uv_handle_t *)socket)) {
        uv_close((uv_handle_t *)socket, empty_close_cb);
    }

    if (socket) {
        free(socket);
    }

    return rc;
}
