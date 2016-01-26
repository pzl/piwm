#include <stdio.h>
#include <string.h> // memset
#include <unistd.h> //close

#include <pthread.h>

#include "gfx.h"
#include "net.h"


typedef struct {
	int sock;
	struct sockaddr_storage addr;
	Display screen;
} Client; /* for passing the info to a thread */

void on_connect(int client_socket, struct sockaddr_storage remote_addr, Display screen);
void *handle_client_thread(void *);

int main (int argc, char **argv) {
	(void) argc;
	(void) argv;

	// network
	int sock;
	int client;

	//GRAPHICS VARS
	Display screen;

	screen = setup_graphics();
	sock = setup_socket();


	// ----------------------

	// Network update vars and setup
	struct sockaddr_storage remote_addr;
	socklen_t addr_size = sizeof(remote_addr);


	while ( (client = accept(sock, (struct sockaddr *)&remote_addr, &addr_size)) > 0){
		on_connect(client, remote_addr, screen);
		//@todo: method to break here, or catch signals
		//and end up in code below, closing socket and cleaning up GPU
		//would need to close threads and clients
	}

	close(sock);
	vc_dispmanx_display_close(screen.display);


	return 0;
}


void *handle_client_thread(void *ptr) {
	Client *client = (Client *)ptr;


	char buf[2048];
	ssize_t n_recvd;
	uint32_t img_data[32];
	ClientWindow gfx;

	printf("got a connection\n");
	get_client_name(&(client->addr));

	printf("creating element\n");
	gfx = create_window(client->screen);

	memset(img_data, 0, 32*sizeof(uint32_t));
	while ( (n_recvd = recv(client->sock, buf, 2048, 0)) > 0) {
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
		
		window_update_graphics(&gfx, img_data);
	}

	
	destroy_window(&gfx); //graphics cleanup
	close(client->sock);  //network cleanup
	free(client);
	//pthread_exit or return
	return NULL;
}

void on_connect(int client, struct sockaddr_storage addr, Display screen) {
	Client *c;
	pthread_t thread;
	pthread_attr_t attrs;

	c = malloc(sizeof(Client));
	c->sock = client;
	c->addr = addr;
	c->screen = screen;

	pthread_attr_init(&attrs);
	pthread_attr_setdetachstate(&attrs, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread, &attrs, handle_client_thread, (void *)c);
}
