#ifndef _PIWM_COMMON_H
#define _PIWM_COMMON_H

#include <sys/socket.h> //sockaddr_storage
#include "gfx.h" //need Display struct

typedef struct {
	int sock;
	struct sockaddr_storage addr;
	Display screen;
} Client; /* for passing the info to a thread */



#endif
