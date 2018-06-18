#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
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
        err("%s:%d: %s: %s\n", __FILE__, __LINE__, uv_err_name(status), uv_strerror(status)); \
        goto label; \
    } \
} while(0)

#define return_uv_err(status) do { \
    if (status < 0) { \
        err("%s:%d: %s: %s\n", __FILE__, __LINE__, uv_err_name(status), uv_strerror(status)); \
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
int DELAY = 5;

static void free_close_cb(uv_handle_t *handle)
{
    free(handle);
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
        log("close stream");
        uv_connect_t *conn = uv_handle_get_data((uv_handle_t *)stream);
        uv_close((uv_handle_t *)stream, free_close_cb);
        free(conn);
    } else {
        return_uv_err(nread);
    }

    free(buf->base);
}

static void on_write(uv_write_t *req, int status)
{
    return_uv_err(status);

    log("start reading");
    uv_read_start(req->handle, alloc_cb, on_read);
    free(req);
}

static void timer_cb(uv_timer_t *timer)
{
    uv_connect_t *connection = uv_handle_get_data((uv_handle_t *)timer);
    if (!connection) {
        err("invalid timer data: no connection handle");
        return;
    }

    uv_stream_t *stream = connection->handle;

    uv_buf_t bufs[] = {
        { .base = GET_REQUEST, .len = strlen(GET_REQUEST) },
    };

    log("send http request");
    uv_write_t *write_req = calloc(1, sizeof(uv_write_t));
    if (!write_req) {
        err("malloc write request failed: %s", strerror(errno));
        return;
    }
    // Store connection handle in stream to free memory on EOF
    uv_handle_set_data((uv_handle_t *)stream, connection);

    uv_write(write_req, stream, bufs, 1, on_write);

    uv_close((uv_handle_t *)timer, free_close_cb);
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

    // Setup timer for sleep
    uv_timer_t *sleep_timer = malloc(sizeof(*sleep_timer));
    if (!sleep_timer) {
        perror("malloc");
        return;
    }

    rc = uv_timer_init(loop, sleep_timer);
    return_uv_err(rc);

    uv_handle_set_data((uv_handle_t *)sleep_timer, connection);

    log("starting timer");
    rc = uv_timer_start(sleep_timer, timer_cb, DELAY * 1000, 0);
    return_uv_err(rc);
}

struct connection {
    uv_tcp_t *socket;
    uv_connect_t *conn;
    struct sockaddr_in dest;
};

void connections_destroy(struct connection *conns, int n)
{
    for (int i = 0; i < n; i++) {
        struct connection c = conns[i];
        if (c.conn) {
            free(c.conn);
        }

        if (c.socket) {
            if (!uv_is_closing((uv_handle_t*)c.socket)) {
                uv_close((uv_handle_t*)c.socket, free_close_cb);
            }
            free(c.socket);
        }
    }

    free(conns);
}

struct connection *connections_setup(struct sockaddr_in dest, int n, uv_loop_t *loop)
{
    struct connection *conns = calloc(n, sizeof(*conns));
    if (!conns) {
        err("malloc connections failed: %s", strerror(errno));
        return NULL;
    }

    for (int i = 0; i < n; i++) {
        struct connection c = conns[i];
        c.socket = malloc(sizeof(uv_tcp_t));
        if (!c.socket) {
            err("malloc socket failed: %s", strerror(errno));
            goto err;
        }

        int rc = uv_tcp_init(loop, c.socket);
        goto_uv_err(rc, err);

        c.conn = malloc(sizeof(uv_connect_t));
        if (!c.conn) {
            err("malloc uv_connect_t failed: %s", strerror(errno));
            goto err;
        }

        rc = uv_tcp_connect(c.conn, c.socket, (const struct sockaddr*)&dest, on_connect);
        goto_uv_err(rc, err);
    }

    return conns;

err:
    connections_destroy(conns, n);
    return NULL;
}

int main(int argc, const char *argv[])
{
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s <addr> <n> [delay]\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "   addr  - addess to connect like 127.0.0.1\n");
        fprintf(stderr, "   n     - number of connections\n");
        fprintf(stderr, "   delay - delay between connection and request in seconds (default is 5)\n");
        return -1;
    }

    int n = strtol(argv[2], NULL, 0);

    if (argc == 4) {
        DELAY = strtol(argv[3], NULL, 0);
    }

    int rc = -1;

    struct sockaddr_in dest;
    rc = uv_ip4_addr(argv[1], 80, &dest);
    goto_uv_err(rc, exit);

    uv_loop_t *loop = uv_default_loop();

    struct connection *conns = connections_setup(dest, n, loop);
    rc = uv_run(loop, UV_RUN_DEFAULT);

exit:
    rc = uv_loop_close(loop);
    return_rc_uv_err(rc, 1);

    connections_destroy(conns, n);

    return rc;
}
