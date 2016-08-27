#ifndef _SOCKET_IO_H
#define _SOCKET_IO_H

int socket_read(int sock, char *buf, size_t bufsize);
int socket_write(int sock, const char *buf, size_t bufsize);

#endif
