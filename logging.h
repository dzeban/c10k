#ifndef _LOGGING_H
#define _LOGGING_H

#include <stdio.h>

#ifdef DEBUG
#define debug(M, ...) fprintf(stderr, "DEBUG %s:%d " M, __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug(M, ...)
#endif

#endif
