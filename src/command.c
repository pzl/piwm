#include <stdio.h>
#include "command.h"
#include "gfx.h"


int win_open(Client *client, ClientWindow *gfx, char *data, uint32_t datalen) {
	(void) data;
	(void) datalen;
	if (gfx->window != 0){
		fprintf(stderr, "ignoring duplicate window\n");
		return 1;
	}
	*gfx = create_window(client->screen);
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
