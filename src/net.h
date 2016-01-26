#ifndef _PIWM_NET_H
#define _PIWM_NET_H

#include <sys/socket.h> //for sockaddr_storage type

int setup_socket(void);
void get_client_name(struct sockaddr_storage *remote);

#endif
