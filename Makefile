CC := gcc
CFLAGS := -Wall -Werror

COMMON_CODE := socket_io.c http_handler.c mongoose/mongoose.c
CLIENT_COMMON_CODE := socket_io.c

all: blocking_single

blocking_single: blocking_single.c $(COMMON_CODE)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $^ -o $@

blocking_forking: blocking_forking.c $(COMMON_CODE)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $^ -o $@


client: client.c $(COMMON_CODE)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) $^ -o $@

clean:
	rm -f *.o blocking_single blocking_forking

# Simple test load - 100 concurent clients for 1000 requests
.PHONY: test
test:
	ab -n1000 -c 100 http://0.0.0.0:8282/
