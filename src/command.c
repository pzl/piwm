#include <stdio.h>
#include "command.h"
#include "net.h"
#include "gfx.h"


int win_open(Client *client, ClientWindow *gfx, char *data, uint32_t datalen) {
	uint16_t native_width = 0,
			 native_height= 0,
			 x = 0,
			 y = 0,
			 scale_width = 0,
			 scale_height= 0;


	if (gfx->window != 0){
		fprintf(stderr, "ignoring duplicate window\n");
		return 1;
	}

	if (datalen > 1) {
		native_width = read16(data);
	}
	if (datalen > 3) {
		native_height = read16(data+2);
	}
	if (datalen > 5){
		x = read16(data+4);
	}
	if (datalen > 7) {
		y = read16(data+6);
	}
	if (datalen > 9) {
		scale_width = read16(data+8);
	}
	if (datalen > 11) {
		scale_height = read16(data+10);
	}


	/*
		@todo: WM placement decisions
	*/

	printf("creating window %hux%hu at %hu,%hu  scaled to %hux%hu\n",
	    	native_width, native_height, x, y, scale_width, scale_height);

	*gfx = create_window(client->screen, native_width, native_height, x, y, scale_width, scale_height);
	return 1;
}


int draw(Client *client, ClientWindow *gfx, char *data, uint32_t datalen) {
	uint32_t img_data[datalen];
	uint32_t i;
	(void) client;

	if (gfx->mode == MODE_BITMAP){
		//@todo: pitch alignment (16)

		//pack bytes into RGBA32 pixels
		// sent as RGB, but bitmap is BGR
		for (i=0; i<datalen; i+=3){
			img_data[i/3] = data[i+2] | data[i+1] << 8 | data[i] << 16;
		}
		window_update_graphics(gfx, img_data);
	} else {
		//OpenVG mode
	}


	return 1;
}

int enable_openvg(Client *client, ClientWindow *gfx, char *data, uint32_t datalen) {
	(void) client;
	(void) data;
	(void) datalen;
	if (gfx->mode == MODE_OPENVG) {
		return 1; //already enabled, nothing to do
	}
	gfx->mode = MODE_OPENVG;

	//delete resources

	//create surfaces, contexts, etc

	return 1;
}

int disable_openvg(Client *client, ClientWindow *gfx, char *data, uint32_t datalen) {
	(void) client;
	(void) data;
	(void) datalen;
	if (gfx->mode == MODE_BITMAP){
		return 1; //already disabled, nothing to do
	}
	gfx->mode = MODE_BITMAP;

	//delete and clear up any OpenVG resources

	//recreate resources

	return 1;
}
