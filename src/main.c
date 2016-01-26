#include <stdio.h>
#include <string.h> // memset
#include <unistd.h> //close

#include "gfx.h"
#include "net.h"



int main (int argc, char **argv) {
	(void) argc;
	(void) argv;

	// network
	int sock;
	int client;

	//GRAPHICS VARS
	uint32_t img_data[32];
	Display screen;


	memset(img_data, 0, 32*sizeof(uint32_t));
	screen = setup_graphics();
	sock = setup_socket();


	// ----------------------

	// Network update vars and setup
	struct sockaddr_storage remote_addr;
	socklen_t addr_size = sizeof(remote_addr);


	// graphics update vars
	ClientWindow gfx;

	while ( (client = accept(sock, (struct sockaddr *)&remote_addr, &addr_size)) > 0){

		printf("got a connection\n");
		get_client_name(&remote_addr);

		printf("creating element\n");
		gfx = create_window(screen);



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
			
			window_update_graphics(&gfx, img_data);
		}

		//graphics cleanup
		destroy_window(&gfx);
		//network cleanup
		close(client);

		//@todo: method to break here, or catch signals
		//and end up in code below, closing socket and cleaning up GPU
	}

	close(sock);
	vc_dispmanx_display_close(screen.display);


	return 0;
}
