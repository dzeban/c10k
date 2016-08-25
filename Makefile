CC := gcc

COMMON_CODE := http_handler.c mongoose/mongoose.c

all: blocking_single

blocking_single: blocking_single.o
	$(CC) $^ $(COMMON_CODE) -o $@

blocking_forking: blocking_forking.o
	$(CC) $^ $(COMMON_CODE) -o $@

clean:
	rm -f *.o blocking_single

# Simple test load - 100 concurent clients for 1000 requests
.PHONY: test
test:
	ab -n1000 -c 100 http://0.0.0.0:8282/
