#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include <uv.h>

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

char *GET_REQUEST = "GET /index.html HTTP/1.1\r\nHost: localhost\r\n\r\n";

static void empty_close_cb(uv_handle_t *handle)
{
    fprintf(stderr, "Invoked close_cb\n");
}

static void alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf)
{
    // uv_buf_init?

    char *b = malloc(suggested_size);
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
    fprintf(stderr, "on_read: nread %zd\n", nread);

    // EOF is not met until keepalive timeout set on nginx
    if (nread == UV_EOF) {
        fprintf(stderr, "nread == EOF\n");
        uv_read_stop(stream);
        fprintf(stderr, "closed stream\n");
        printf("\n");
    }

    return_uv_err(nread);

    printf("%s", buf->base);
    free(buf->base);

    // uv_close((uv_handle_t *)stream, empty_close_cb);
}

static void on_write(uv_write_t *req, int status)
{
    return_uv_err(status);
    fprintf(stderr, "written\n");

    uv_read_start(req->handle, alloc_cb, on_read);
}

static void on_connect(uv_connect_t *connection, int status)
{
    return_uv_err(status);
    fprintf(stderr, "connected\n");

    uv_buf_t bufs[] = {
        { .base = GET_REQUEST, .len = strlen(GET_REQUEST) },
    };

    uv_write_t write_req;
    uv_write(&write_req, connection->handle, bufs, 1, on_write);
}

static void walk_cb(uv_handle_t *handle, void *arg)
{
    fprintf(stderr, "%d: active %d, closing %d, ref %d\n",
        uv_handle_get_type(handle),
        uv_is_active(handle),
        uv_is_closing(handle),
        uv_has_ref(handle));
}

static void idle_handler(uv_idle_t* handle)
{
    uv_walk(handle->loop, walk_cb, NULL);
}

int main(int argc, const char *argv[])
{
    int rc = -1;

    uv_loop_t *loop = uv_default_loop();

    // uv_idle_t idler;
    // uv_idle_init(loop, &idler);
    // uv_idle_start(&idler, idle_handler);

    uv_tcp_t *socket = malloc(sizeof(*socket));
    if (!socket) {
        perror("malloc");
        return -1;
    }

    rc = uv_tcp_init(loop, socket);
    goto_uv_err(rc, exit);

    // This doesn't disable keepalive actually
    rc = uv_tcp_keepalive(socket, 0, 0);
    goto_uv_err(rc, exit);

    uv_connect_t *connect = malloc(sizeof(*connect));
    if (!connect) {
        perror("malloc");
        goto exit;
    }

    struct sockaddr_in dest;
    rc = uv_ip4_addr("127.0.0.1", 80, &dest);
    goto_uv_err(rc, exit);

    rc = uv_tcp_connect(connect, socket, (const struct sockaddr *)&dest, on_connect);
    goto_uv_err(rc, exit);

    rc = uv_run(loop, UV_RUN_DEFAULT);

exit:
    if (!uv_is_closing((uv_handle_t *)connect)) {
        // This uv_close call violates assertion in libuv
        // $ ./libuv-client > /dev/null
        // connected
        // written
        // on_read
        // on_read
        // closed stream
        // libuv-client.c:63: EOF : end of file
        // Invoked close_cb
        // libuv-client: src/unix/core.c:182: uv_close : Assertion `0' failed. Aborted(core dumped)
        //
        // uv_close((uv_handle_t *)connect, empty_close_cb);
    }

    if (connect) {
        free(connect);
    }

    if (!uv_is_closing((uv_handle_t *)socket)) {
        uv_close((uv_handle_t *)socket, empty_close_cb);
    }

    if (socket) {
        free(socket);
    }

    uv_loop_close(loop);

    return rc;
}
