#ifndef _PIWM_GFX_H
#define _PIWM_GFX_H

#include <bcm_host.h>
#include <stdint.h> //uint32_t type

#define UPDATE_PRIORITY 10

typedef struct {
	DISPMANX_ELEMENT_HANDLE_T window;
	DISPMANX_RESOURCE_HANDLE_T rsrc[2];
	int next;
} ClientWindow;

DISPMANX_DISPLAY_HANDLE_T setup_graphics(void);
ClientWindow create_window(DISPMANX_DISPLAY_HANDLE_T display);
void destroy_window(ClientWindow *c);
void window_update_graphics(ClientWindow *c, uint32_t *data);


#endif
