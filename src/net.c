#include <stdio.h>
#include <string.h> // memset
#include <unistd.h> //close, gethostname
#include <errno.h>

#include <sys/types.h> //getaddrinfo()
#include <sys/socket.h>
#include <netdb.h>     //getaddrinfo()

#include <arpa/inet.h>

#include "gfx.h"

#define PORT "18455"
#define MAX_BACKED_UP_CLIENTS 10


int setup_socket(void) {
	int sock;
	int result;
	struct addrinfo hints,
					*res;

	// ------ SERVER CONNECTION SETUP

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; // IPv4 or IPV6, whatever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // we will be the server, accept()ing conns

	if ( (result = getaddrinfo(NULL, PORT, &hints, &res)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
		return -1;
	}

	if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
		fprintf(stderr, "failed creating socket");
		return -1;
	}

	int reuse=1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0){
		perror("setsockopt");
		return -1;
	}
	if (bind(sock, res->ai_addr, res->ai_addrlen) < 0){
		perror("failed binding socket");
		return -1;
	}

	if (listen(sock, MAX_BACKED_UP_CLIENTS) < 0){
		fprintf(stderr, "failed to listen()");
		return -1;
	}
	printf("listening on port %s\n", PORT);


	/*
	char myname[2048];
	if (gethostname(myname, 2048) < 0) {
		perror("gethostname");
	}  // uname -m
	*/



	return sock;
}


void get_client_name(struct sockaddr_storage *remote) {
	char clientname[2048];
	int port;


	if (remote->ss_family == AF_INET) { //IPv4
		struct sockaddr_in *s = (struct sockaddr_in *)remote;
		port = ntohs(s->sin_port);
		if (inet_ntop(AF_INET, &s->sin_addr, clientname, 2048) == NULL){
			perror("inet_ntop");
		}
	} else {
		struct sockaddr_in6 *s = (struct sockaddr_in6 *)remote;
		port = ntohs(s->sin6_port);
		if (inet_ntop(AF_INET6, &s->sin6_addr, clientname, 2048) == NULL){
			perror("inet_ntop 6");
		}
	}

	printf("from %s:%d\n", clientname,port);
}
