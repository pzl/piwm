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




int main (int argc, char **argv) {
	(void) argc;
	(void) argv;


	// NETWORK VARS
	int sock;
	struct addrinfo hints,
					*res;
	struct sockaddr_storage remote_addr;
	socklen_t addr_size = sizeof(remote_addr);
	int client;



	//GRAPHICS VARS
	uint32_t img_data[32];
	DISPMANX_DISPLAY_HANDLE_T display;

	display = setup_graphics();



	// ------ SERVER CONNECTION SETUP

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6; // IPv4 or IPV6, whatever
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // we will be the server, accept()ing conns

	int result;
	if ( (result = getaddrinfo(NULL, PORT, &hints, &res)) != 0){
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
		return -1;
	}


	if ((sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0){
		fprintf(stderr, "failed creating socket");
		return -1;
	}

	int reuse=1;
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,&reuse,sizeof(int)) < 0){
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



	// GRAPHICS DRAWING/UPDATE VARS and SETUP
	VC_RECT_T rect;
	ClientWindow gfx;
	DISPMANX_UPDATE_HANDLE_T update;

	memset(img_data, 0, 32*sizeof(uint32_t));
	vc_dispmanx_rect_set(&rect, 0, 0, 2, 2);



	while ( (client = accept(sock, (struct sockaddr *)&remote_addr, &addr_size)) > 0){

		printf("got a connection\n");

		char clientname[2048];
		int port;
		if (remote_addr.ss_family == AF_INET) { //IPv4
			struct sockaddr_in *s = (struct sockaddr_in *)&remote_addr;
			port = ntohs(s->sin_port);
			if (inet_ntop(AF_INET, &s->sin_addr, clientname, 2048) == NULL){
				perror("inet_ntop");
			}
		} else {
			struct sockaddr_in6 *s = (struct sockaddr_in6 *)&remote_addr;
			port = ntohs(s->sin6_port);
			if (inet_ntop(AF_INET6, &s->sin6_addr, clientname, 2048) == NULL){
				perror("inet_ntop 6");
			}
		}


		printf("from %s:%d\n", clientname,port);

		printf("creating element\n");
		gfx = create_window(display);



		char buf[2048];
		ssize_t n_recvd;

		while ( (n_recvd = recv(client, buf, 2048, 0)) > 0) {
			/*
			printf("< ");
			for (int i=0; i<n_recvd; i++){
				printf("[%d] ",buf[i]);
			}
			printf(">\n");
			*/

			if (n_recvd < 12){
				printf("byte underflow. Only received %d\n", n_recvd);
				continue;
			}

			if (n_recvd > 12){
				printf("byte overflow. Got %d, wanted 12\n", n_recvd);
			}


			// sent as RGB, but bitmap is BGR
			img_data[0] = buf[2] | buf[1] << 8 | buf[0] << 16;
			img_data[1] = buf[5] | buf[4] << 8 | buf[3] << 16;
			img_data[16]= buf[8] | buf[7] << 8 | buf[6] << 16;
			img_data[17]= buf[11] | buf[10] << 8 | buf[9] << 16;
			

			//upload image data to GPU resource
			if (vc_dispmanx_resource_write_data(gfx.rsrc[gfx.next], VC_IMAGE_RGBA32,
			                                16*sizeof(uint32_t),
			                                img_data, &rect) != 0){
				fprintf(stderr, "error writing resource data\n");
				continue;
			}


			if ((update = vc_dispmanx_update_start(UPDATE_PRIORITY)) == DISPMANX_NO_HANDLE){
				fprintf(stderr, "error getting update handle\n");
				continue;
			}
			if (vc_dispmanx_element_change_source(update, gfx.window, gfx.rsrc[gfx.next]) != 0){
				fprintf(stderr, "error setting element source\n");
			}
			if (vc_dispmanx_update_submit_sync(update) != 0){
				fprintf(stderr, "error submitting source change\n");
			}
			


			//swap resource buffers, write to the now-unused one
			gfx.next ^= 1;
		}

		//graphics cleanup
		destroy_window(gfx);
		//network cleanup
		close(client);

		//@todo: method to break here, or catch signals
		//and end up in code below, closing socket and cleaning up GPU
	}

	close(sock);
	vc_dispmanx_display_close(display);


	return 0;
}
