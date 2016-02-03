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
	uint32_t img_data[32];
	(void) client;

	if (datalen < 12){
		fprintf(stderr, "not enough bytes to draw. Got %zu, expected 12\n",datalen);
		return 0;
	} else if (datalen > 12){
		fprintf(stderr, "got byte overflow (%d). expected 12\n", datalen);
	}

	// sent as RGB, but bitmap is BGR
	img_data[0] = data[2] | data[1] << 8 | data[0] << 16;
	img_data[1] = data[5] | data[4] << 8 | data[3] << 16;
	img_data[16]= data[8] | data[7] << 8 | data[6] << 16;
	img_data[17]= data[11] | data[10] << 8 | data[9] << 16;
	window_update_graphics(gfx, img_data);

	return 1;
}
