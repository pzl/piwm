#include <stdio.h>
#include <string.h> // memset
#include <unistd.h> //close

#include <pthread.h>

#include "common.h"
#include "command.h"
#include "gfx.h"
#include "net.h"


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

	printf("Command from packet: %02X\n", command);
	switch (command) {
		case PIWM_CMD_OPEN: running = win_open(client,gfx,data,datalen); break;
		case PIWM_CMD_DRAW: running = draw(client,gfx,data,datalen); break;
		case PIWM_CMD_CLOSE: running=0; break;
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
