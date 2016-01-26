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


#define PIWM_CMD_OPEN		0x00
#define PIWM_CMD_CLOSE		0x01

#define PIWM_CMD_DRAW		0x02

#define PIWM_CMD_RESIZE		0x03

#define PIWM_CMD_VGENABLE	0x04
#define PIWM_CMD_VGDISABLE	0x05

#define PIWM_CMD_RESERVED	0x06

#define PIWM_CMD_VGCMD		0x07


void *handle_client_thread(void *ptr) {
	Client *client = (Client *)ptr;


	char buf[2048];
	ssize_t n_recvd;
	uint32_t img_data[32];
	ClientWindow gfx = { 0 };

	printf("got a connection ");
	get_client_name(&(client->addr));


	while ( (n_recvd = recv(client->sock, buf, 2048, 0)) > 0) {
		/*
		printf("< ");
		for (int i=0; i<n_recvd; i++){
			printf("[%d] ",buf[i]);
		}
		printf(">\n");
		*/

		switch (buf[0]) {
			case PIWM_CMD_OPEN:
				if (gfx.window != 0){ fprintf(stderr, "ignoring duplicate window\n"); break; }
				gfx = create_window(client->screen);
				break;
			case PIWM_CMD_DRAW:
				//currently expects 1 cmd byte, and 12 (3*4) data bytes
				if (n_recvd < 13){
					fprintf(stderr, "not enough bytes to draw. Got %d, expected 13\n",n_recvd);
					break;
				} else if (n_recvd > 13){
					fprintf(stderr, "got byte overflow (%d). expected 13\n", n_recvd);
				}

				// sent as RGB, but bitmap is BGR
				img_data[0] = buf[3] | buf[2] << 8 | buf[1] << 16;
				img_data[1] = buf[6] | buf[5] << 8 | buf[4] << 16;
				img_data[16]= buf[9] | buf[8] << 8 | buf[7] << 16;
				img_data[17]= buf[12] | buf[11] << 8 | buf[10] << 16;
				window_update_graphics(&gfx, img_data);
				break;
			case PIWM_CMD_CLOSE:
			case PIWM_CMD_RESIZE:
			case PIWM_CMD_VGENABLE:
			case PIWM_CMD_VGDISABLE:
			case PIWM_CMD_RESERVED:
			case PIWM_CMD_VGCMD:
				printf("command not implemented\n");
				break;
			default:
				printf("invalid command byte: %02X\n", buf[0]);
				break;
		}
	}

	printf("destroying window, closing socket.\n");	
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
