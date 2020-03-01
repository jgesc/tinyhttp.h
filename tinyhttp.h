#ifndef __TINYHTTP_H__
#define __TINYHTTP_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>

// Request enum
enum request {GET, HEAD, POST, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH, INVALID};

void thttp_main(char * root, uint16_t port);

#endif
