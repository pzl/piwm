#include <stdio.h>
#include <string.h> // memset
#include <unistd.h> //close, gethostname
#include <errno.h>

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
	DISPMANX_DISPLAY_HANDLE_T display;


	memset(img_data, 0, 32*sizeof(uint32_t));
	display = setup_graphics();
	sock = setup_socket();

	// ----------------------

	// Network update vars and setup
	struct sockaddr_storage remote_addr;
	socklen_t addr_size = sizeof(remote_addr);


	// GRAPHICS DRAWING/UPDATE VARS and SETUP
	VC_RECT_T rect;
	ClientWindow gfx;
	DISPMANX_UPDATE_HANDLE_T update;

	memset(img_data, 0, 32*sizeof(uint32_t));
	vc_dispmanx_rect_set(&rect, 0, 0, 2, 2);



	while ( (client = accept(sock, (struct sockaddr *)&remote_addr, &addr_size)) > 0){

		printf("got a connection\n");
		get_client_name(&remote_addr);

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
