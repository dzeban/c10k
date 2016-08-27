#ifndef _LOGGING_H
#define _LOGGING_H

#include <stdio.h>
#include <unistd.h>

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "=%05d= DEBUG %s:%d " M, getpid(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

#define info(M, ...) fprintf(stderr, "=%05d= INFO %s:%d " M, getpid(), __FILE__, __LINE__, ##__VA_ARGS__)

#endif
