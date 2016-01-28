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
static int run_command(uint8_t command, char *data, uint32_t datalen, Client *, ClientWindow *);

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
	ClientWindow gfx = { 0 };
	int running = 1;

	//for reading data into
	Buffer buf;
	char *packet=NULL;
	uint32_t packlen;
	uint8_t cmd;

	printf("got a connection ");
	get_client_name(&(client->addr));


	while (running) {
		// 1. get single "packet"
		printf("asking for a packet\n");
		packlen = get_packet(client->sock, &buf, &packet);
		printf("got a packet. Length: %zu\n", packlen);
		if (packlen == 0){
			running = 0;
			break;
		}

		// 2. parse packet and perform command
		cmd = (uint8_t) *packet++;
		running = run_command(cmd, packet, packlen-1, client, &gfx);

		// 3. repeat or cleanup
	}

	// X. cleanup and leave
	printf("destroying window, closing socket.\n");	
	destroy_window(&gfx); //graphics cleanup
	close(client->sock);  //network cleanup
	free(buf.buf);
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

static int run_command(uint8_t command, char *data, uint32_t datalen, Client *client, ClientWindow *gfx) {
	int running=1;
	uint32_t img_data[32];

	printf("Command from packet: %02X\n", command);
	switch (command) {
		case PIWM_CMD_OPEN:
			if (gfx->window != 0){ fprintf(stderr, "ignoring duplicate window\n"); break; }
			*gfx = create_window(client->screen);
			break;
		case PIWM_CMD_DRAW:
			if (datalen < 12){
				fprintf(stderr, "not enough bytes to draw. Got %zu, expected 12\n",datalen);
				break;
			} else if (datalen > 12){
				fprintf(stderr, "got byte overflow (%d). expected 12\n", datalen);
			}

			// sent as RGB, but bitmap is BGR
			img_data[0] = data[2] | data[1] << 8 | data[0] << 16;
			img_data[1] = data[5] | data[4] << 8 | data[3] << 16;
			img_data[16]= data[8] | data[7] << 8 | data[6] << 16;
			img_data[17]= data[11] | data[10] << 8 | data[9] << 16;
			window_update_graphics(gfx, img_data);
			break;
		case PIWM_CMD_CLOSE:
			running=0;
			break;
		case PIWM_CMD_RESIZE:
		case PIWM_CMD_VGENABLE:
		case PIWM_CMD_VGDISABLE:
		case PIWM_CMD_RESERVED:
		case PIWM_CMD_VGCMD:
			printf("command not implemented\n");
			break;
		default:
			printf("invalid command byte: %02X\n", command);
			running=0;
			break;
	}

	return running;
}
