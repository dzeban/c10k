CC := gcc

COMMON_CODE := http_handler.c mongoose/mongoose.c

all: blocking_single

blocking_single: blocking_single.o
	$(CC) $^ $(COMMON_CODE) -o $@

clean:
	rm -f *.o blocking_single
