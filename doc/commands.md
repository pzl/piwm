Commands
=========

This document outlines the network communications required to be a client to PiWM.

General Packet Structure
-------------------------

All packets should be sent with the following format:

```
    <4-byte SIZE> <1-byte CMD> [<N-byte DATA>]
```

**Size**:  All packets must start with the **SIZE** header, which is a 32-bit (4-byte) network-order number indicating the length of the packet, in bytes, _including_ the size header itself. So a packet that consists of only `SIZE` and `CMD` without any data would be described as 5 bytes: `0x00 0x00 0x00 0x05 CMD`. Size is always required.

**CMD**: The next field is a single byte describing the command to be performed. The table describing what bytes correspond to what command can be found below. The command byte is always required.

**DATA**: `DATA` is the last field, and is sometimes optional. The format, size, and requirements of the `DATA` field is governed by each individual command. Some commands (e.g. CLOSE) require no data, and this field may be omitted entirely. This size of this field must be accounted for in the `SIZE` header. 


Commands
---------

|  Command            |  Byte  | Data     | Parameters |
|---------------------|--------|----------|------------|
| [Window Open][1]    | `0x00` | optional | Window position, size, scaling |
| [Window Close][2]   | `0x01` | None     | |
| [Window Resize][3]  | `0x02` | required | Window position, size, scaling |
| [Window Draw][4]    | `0x03` | optional | Bitmap data, when not in OpenVG mode |
| [Enable OpenVG][5]  | `0x04` | None     | |
| [Disable OpenVG][6] | `0x05` | None     | |
| _Reserved_          | `0x06` | -        | | 
| [Perform VG CMD][7] | `0x07` | required | VG command and associated parameters |


### Window Open

Requests a visible window from the window manager.

**Data**: This field is optional for this command. When present, requests a certain window size, coordinates (of upper left corner), and any hardware scaling. PiWM does not guarantee any of these fields to be what was requested, or even that the call will succeed. PiWM will respond to this call with: whether the call succeeded and your app has a window to draw to, and what it's actual size, position, and scaling are on the display. PiWM will try to honor size requests when possible. When omitted, will create a window in any available space.

**Data Format**: each field must be in network byte order. Fields must be sent in the order they appear below. Any omitted bytes at the end are assumed to be `0`.

```
    <2-byte width> <2-byte height> <2-byte X-coord> <2-byte Y-coord> <2-byte scale-W> <2-byte scale-H>
```

an example open packet (including all headers), of a 600x400 window at 5,10, with no hardware scaling

```
    0x00 0x00 0x00 0x0e 0x00 0x02 0x58 0x01 0x90 0x05 0x00 0x05 0x00 0x0a
```


`0x0000000e` is the SIZE header (14 bytes). `0x00` is the Window Open command. `0x0258` is the requested Width (600 in decimal). `0x0190` is the Height (400). `0x0005` is the X-coordinate, `0x000a` is the Y (10). Hardware scaling was omitted (sending `0`'s or the window's requested size would also mean natural scaling).



### Window Close

Closes your app's window and cleans up any server-side resources. This command is optional, and is otherwise automatically performed when you disconnect from the socket. This command accepts no data field.

### Window Resize

Requests a change in size, position, or hardware scaling of your window. Requires an open window be present. Any data sent are requests, and are not necessarily honored. PiWM will respond with the actual size, position, and scale your window was given.

The data and data format for this field are identical to those from [Window Open][1].


### Draw

Repaints your window. This command behaves differently depending on whether OpenVG is enabled for your window. When disabled, expects bitmap data to paint your window with. When enabled, displays results of all OpenVG commands since last repaint.

**Data**: This field is optional.

### Enable OpenVG

### Disable OpenVG


### Perform OpenVG Command


[1]: #Window-Open
[2]: #Window-Close
[3]: #Window-Resize
[4]: #Draw
[5]: #Enable-OpenVG
[6]: #Disable-OpenVG
[7]: #Perform-OpenVG-Command
