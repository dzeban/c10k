CC := gcc
CFLAGS := -Wall -Werror # -DDEBUG

BUILDDIR := build
BUILDDIR_SERVER := $(BUILDDIR)/server
BUILDDIR_CLIENT := $(BUILDDIR)/client

PACKAGEDIR := package
SERVER_PACKAGE_NAME := c1m-servers
SERVER_PACKAGE_VERSION := 1.0

COMMON_CODE := socket_io.c http_handler.c mongoose/mongoose.c picohttpparser/picohttpparser.c

all: dirs server client

dirs:
	mkdir -p $(BUILDDIR_SERVER) $(BUILDDIR_CLIENT) $(PACKAGEDIR)

server: dirs blocking-single blocking-forking
client: dirs simple-client libevent2-client

server-deb: server
	fpm -s dir -t deb -C $(BUILDDIR_SERVER) --prefix /usr/local/bin -f -n $(SERVER_PACKAGE_NAME) -p $(PACKAGEDIR)/c1m-servers_$(SERVER_PACKAGE_VERSION).deb

server-rpm: server
	fpm -s dir -t rpm -C $(BUILDDIR_SERVER) --prefix /usr/local/bin -f -n $(SERVER_PACKAGE_NAME) -p $(PACKAGEDIR)/c1m-servers_$(SERVER_PACKAGE_VERSION).rpm

blocking-single: blocking_single.c $(COMMON_CODE)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $^ -o $(BUILDDIR_SERVER)/$@

blocking-forking: blocking_forking.c $(COMMON_CODE)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $^ -o $(BUILDDIR_SERVER)/$@

simple-client: client.c $(COMMON_CODE)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $^ -o $(BUILDDIR_CLIENT)/$@

libevent2-client: libevent2-client.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -levent $^ -o $(BUILDDIR_CLIENT)/$@

clean:
	rm -rf *.o $(BUILDDIR) $(PACKAGEDIR)

# Simple test load - 100 concurent clients for 1000 requests
.PHONY: test
test:
	ab -n1000 -c 100 http://0.0.0.0:8282/