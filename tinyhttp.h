#ifndef __TINYHTTP_H__
#define __TINYHTTP_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>

enum request {GET, HEAD, POST, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH, INVALID};

#endif
