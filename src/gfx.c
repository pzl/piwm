#include <stdio.h>
#include <bcm_host.h>
#include "gfx.h"


Display setup_graphics(void){
	Display d;
	DISPMANX_MODEINFO_T dispinfo;

	bcm_host_init();

	if ((d.display = vc_dispmanx_display_open(0 /*LCD*/)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "error opening display\n");
		exit(1);
	}

	if (vc_dispmanx_display_get_info(d.display, &dispinfo) != 0){
		fprintf(stderr, "error getting display information\n");
		exit(1);
	}

	printf("Display is %dx%d\n",dispinfo.width, dispinfo.height);
	d.width = dispinfo.width;
	d.height = dispinfo.height;

	return d;
}

ClientWindow create_window(Display d) {
	ClientWindow client;
	uint32_t throwaway_ptr;
	DISPMANX_UPDATE_HANDLE_T update;
	VC_DISPMANX_ALPHA_T alpha = {
		DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS,
		120, // 0->255
		0
	};
	VC_RECT_T src,
			  dst;

	client.next = 0;

	if ((client.rsrc[0] = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, 2,2, &throwaway_ptr)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "Error creating resource\n");
		exit(1);
	}
	if ((client.rsrc[1] = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, 2,2, &throwaway_ptr)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "Error creating resource\n");
		exit(1);
	}


	if (vc_dispmanx_rect_set(&src, 0, 0, 2<<16, 2<<16) != 0){
		fprintf(stderr, "error setting rect\n");
		exit(1);
	}
	if (vc_dispmanx_rect_set(&dst, 5, 5, 200, 200) != 0){
		fprintf(stderr, "error setting rect\n");
		exit(1);
	}

	if ((update = vc_dispmanx_update_start(UPDATE_PRIORITY)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "error getting update handle\n");
		exit(1);
	}

	if ((client.window = vc_dispmanx_element_add(update, d.display, 1/*LAYER*/,
									&dst, 0/*rsrc*/, &src,
									DISPMANX_PROTECTION_NONE,
									&alpha, NULL, DISPMANX_NO_ROTATE)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "Error adding window\n");
		exit(1);
	}

	if (vc_dispmanx_update_submit_sync(update) != 0){
		fprintf(stderr, "error finishing element add update\n");
		exit(1);
	}

	printf("opened window\n");

	return client;
}

void destroy_window(ClientWindow *c) {
	//graphics window cleanup

	DISPMANX_UPDATE_HANDLE_T update;

	if ((update = vc_dispmanx_update_start(UPDATE_PRIORITY)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "error getting update handle for window cleanup\n");
	}
	if (vc_dispmanx_element_remove(update,c->window) != 0){
		fprintf(stderr, "error removing window\n");
	}
	if (vc_dispmanx_update_submit_sync(update) != 0){
		fprintf(stderr, "error submitting update to remove window\n");
	}

	if (vc_dispmanx_resource_delete(c->rsrc[0]) != 0){
		fprintf(stderr, "error deleting first resource\n");
	}
	if (vc_dispmanx_resource_delete(c->rsrc[1]) != 0){
		fprintf(stderr, "error deleting second resource\n");
	}

	c->next=0;
}

void window_update_graphics(ClientWindow *c, uint32_t *data) {
	DISPMANX_UPDATE_HANDLE_T update;
	VC_RECT_T rect;
	vc_dispmanx_rect_set(&rect, 0, 0, 2, 2);


	//upload image data to GPU resource
	if (vc_dispmanx_resource_write_data(c->rsrc[c->next], VC_IMAGE_RGBA32,
	                                16*sizeof(uint32_t),
	                                data, &rect) != 0){
		fprintf(stderr, "error writing resource data\n");
		return;
	}


	if ((update = vc_dispmanx_update_start(UPDATE_PRIORITY)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "error getting update handle\n");
		return;
	}
	if (vc_dispmanx_element_change_source(update, c->window, c->rsrc[c->next]) != 0){
		fprintf(stderr, "error setting element source\n");
	}
	if (vc_dispmanx_update_submit_sync(update) != 0){
		fprintf(stderr, "error submitting source change\n");
	}



	//swap resource buffers, write to the now-unused one
	c->next ^= 1;
}
