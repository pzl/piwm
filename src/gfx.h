#ifndef _PIWM_GFX_H
#define _PIWM_GFX_H

#include <bcm_host.h>
#include <stdint.h> //uint32_t type

#define UPDATE_PRIORITY 10

typedef struct {
	DISPMANX_ELEMENT_HANDLE_T window;
	DISPMANX_RESOURCE_HANDLE_T rsrc[2];
	uint8_t next;
	enum {MODE_BITMAP, MODE_OPENVG} mode;
	uint16_t native_w;
	uint16_t native_h;
	uint16_t x;
	uint16_t y;
	uint16_t scaled_w;
	uint16_t scaled_h;
} ClientWindow;

typedef struct {
	DISPMANX_DISPLAY_HANDLE_T display;
	int width;
	int height;
} Display;

Display setup_graphics(void);
ClientWindow create_window(Display d, uint16_t native_w, uint16_t native_h, uint16_t x, uint16_t y, uint16_t scaled_w, uint16_t scaled_h);
void destroy_window(ClientWindow *c);
void window_update_graphics(ClientWindow *c, uint32_t *data);
void window_resize(ClientWindow *c, uint16_t native_w, uint16_t native_h, uint16_t x, uint16_t y, uint16_t scaled_w, uint16_t scaled_h);


#endif
