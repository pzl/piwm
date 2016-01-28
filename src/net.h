#ifndef _PIWM_NET_H
#define _PIWM_NET_H

#include <sys/socket.h> //for sockaddr_storage type

typedef struct {
	char *buf; /* actual storage, needs to be alloc'd */
	char *buf_end; //pointer to last read byte
	char *next_boundary; // pointer to next un-gotten packet
} Buffer;


int setup_socket(void);
void get_client_name(struct sockaddr_storage *remote);
uint32_t get_packet(int sock, Buffer *, char **packet_pointer);


#endif
