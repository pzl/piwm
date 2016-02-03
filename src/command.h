#ifndef _PIWM_COMMAND_H
#define _PIWM_COMMAND_H

#include "common.h"
#include "gfx.h"

#define PIWM_CMD_OPEN		0x00
#define PIWM_CMD_CLOSE		0x01
#define PIWM_CMD_RESIZE		0x02

#define PIWM_CMD_DRAW		0x03

#define PIWM_CMD_VGENABLE	0x04
#define PIWM_CMD_VGDISABLE	0x05

#define PIWM_CMD_RESERVED	0x06

#define PIWM_CMD_VGCMD		0x07


/*
	All of these functions return a positive integer to denote success.
	A zero or otherwise falsey response means the connection with the client
	should be destroyed
*/

int win_open(Client *, ClientWindow *, char *data, uint32_t datalen);
int draw    (Client *, ClientWindow *, char *data, uint32_t datalen);


#endif
