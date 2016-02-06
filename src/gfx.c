#include <stdio.h>
#include <bcm_host.h>
#include "gfx.h"

//Attributes changes flag mask
//from /opt/vc/include/interface/vmcs_host/vc_vchi_dispmanx.h, but can't include
#define ELEMENT_CHANGE_LAYER          (1<<0)
#define ELEMENT_CHANGE_OPACITY        (1<<1)
#define ELEMENT_CHANGE_DEST_RECT      (1<<2)
#define ELEMENT_CHANGE_SRC_RECT       (1<<3)
#define ELEMENT_CHANGE_MASK_RESOURCE  (1<<4)
#define ELEMENT_CHANGE_TRANSFORM      (1<<5)

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

ClientWindow create_window(Display d, uint16_t native_w, uint16_t native_h, uint16_t x, uint16_t y, uint16_t scaled_w, uint16_t scaled_h) {
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


	client.native_w = native_w;
	client.native_h = native_h;
	client.x = x;
	client.y = y;
	client.scaled_w = scaled_w;
	client.scaled_h = scaled_h;

	client.next = 0;
	client.mode = MODE_BITMAP;

	if ((client.rsrc[0] = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, client.native_w, client.native_h, &throwaway_ptr)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "Error creating resource\n");
		exit(1);
	}
	if ((client.rsrc[1] = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, client.native_w,client.native_h, &throwaway_ptr)) == DISPMANX_NO_HANDLE){
		fprintf(stderr, "Error creating resource\n");
		exit(1);
	}


	if (vc_dispmanx_rect_set(&src, 0, 0, client.native_w<<16, client.native_h<<16) != 0){
		fprintf(stderr, "error setting rect\n");
		exit(1);
	}
	if (vc_dispmanx_rect_set(&dst, client.x, client.y, client.scaled_w, client.scaled_h) != 0){
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

	if (c->mode == MODE_OPENVG) {
		fprintf(stderr, "cannot bitmap update in OpenVG mode\n");
		return;
	}

	vc_dispmanx_rect_set(&rect, 0, 0, c->native_w, c->native_h);


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


void window_resize(ClientWindow *c, uint16_t native_w, uint16_t native_h, uint16_t x, uint16_t y, uint16_t scaled_w, uint16_t scaled_h) {
	uint32_t throwaway_ptr;
	uint32_t update_flags = 0;
	DISPMANX_UPDATE_HANDLE_T update;
	VC_RECT_T src,
			  dst;

	// can be left NULL in element_change_attr to signify no change. Will
	// point to their counterparts if the values have changed
	VC_RECT_T *src_p = NULL,
			  *dst_p = NULL;



	if (c->mode == MODE_BITMAP){


		if (c->native_w != native_w || c->native_h != native_h) {
			//source sizes changed
			update_flags |= ELEMENT_CHANGE_SRC_RECT; //set automatically in dispman_change_attributes, but let's be careful

			//disable the current resources
			if ((update = vc_dispmanx_update_start(UPDATE_PRIORITY)) == DISPMANX_NO_HANDLE){
				fprintf(stderr, "error getting update handle\n");
				return;
			}
			if (vc_dispmanx_element_change_source(update, c->window, 0) != 0){
				fprintf(stderr, "error setting element source\n");
			}
			if (vc_dispmanx_update_submit_sync(update) != 0){
				fprintf(stderr, "error submitting source change\n");
			}


			//resize the resources, destroying the old and recreating at new dimensions
			if (vc_dispmanx_resource_delete(c->rsrc[0]) != 0){
				fprintf(stderr, "error deleting first resource\n");
			}
			if (vc_dispmanx_resource_delete(c->rsrc[1]) != 0){
				fprintf(stderr, "error deleting second resource\n");
			}
			if ((c->rsrc[0] = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, native_w, native_h, &throwaway_ptr)) == DISPMANX_NO_HANDLE){
				fprintf(stderr, "Error creating resource\n");
				exit(1);
			}
			if ((c->rsrc[1] = vc_dispmanx_resource_create(VC_IMAGE_RGBA32, native_w, native_h, &throwaway_ptr)) == DISPMANX_NO_HANDLE){
				fprintf(stderr, "Error creating resource\n");
				exit(1);
			}


			//change the source rect, so the element knows its new source size
			if (vc_dispmanx_rect_set(&src, 0, 0, native_w<<16, native_h<<16) != 0){
				fprintf(stderr, "error setting rect\n");
				exit(1);
			}
			src_p = &src;

		}

		//@todo: layer change?
		//@todo: opacity, transform change?

		if (c->scaled_w != scaled_w || c->scaled_h != scaled_h || c->x != x || c->y != y) {
			//position or final window size changed
			update_flags |= ELEMENT_CHANGE_DEST_RECT; //set automatically in dispman_change_attributes, but let's be careful


			if (vc_dispmanx_rect_set(&dst, x, y, scaled_w, scaled_h) != 0){
				fprintf(stderr, "error setting rect\n");
				exit(1);
			}
			dst_p = &dst;
		}


		//update element
		if ((update = vc_dispmanx_update_start(UPDATE_PRIORITY)) == DISPMANX_NO_HANDLE){
			fprintf(stderr, "error getting update handle\n");
			return;
		}
		if (vc_dispmanx_element_change_attributes(update, c->window, 
		    									update_flags,
		    									1/*LAYER*/, 0/*opacity?*/,
		    									dst_p, src_p,
		    									0/*mask??*/, DISPMANX_NO_ROTATE) != 0) {

		}
		if (vc_dispmanx_update_submit_sync(update) != 0){
			fprintf(stderr, "error submitting source change\n");
		}

		
	} else {
		//OpenVG mode
	}



}
