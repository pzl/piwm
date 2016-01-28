PiWM
=====

**PiWM** is a Raspberry **Pi** **W**indow **M**anager. It does _not_ function as an X11 window manager. It is designed to be run without X11, creating raw DispmanX<sup>*</sup> windows for your applications to run on. The applications must be purpose-designed to use PiWM, it is not a replacement to run X11 apps without X. Applications using PiWM may draw their windows using raw bitmaps, or use hardware-accelerated OpenVG. In the future, this may include OpenGL ES.

<sup>* DispmanX is unique to Broadcom/Raspberry Pi. It is (very slowly) in the process of being phased out in favor of OpenWF, but I've been hearing that line for upwards of 4 years with no progress.</sup>


Each window may specify a different canvas size to its displayed size on a monitor, and benefit from hardware-accelerating scaling. This means a window may draw itself as a 40x40 bitmap, send the 40x40 pixel data, but ask that the WM draw that window as 200x200 pixels. The GPU will perform this scaling for you. 

Communication to PiWM is done via network sockets, and therefore supports remote clients. This does include some higher overhead than other methods, but has not inhibited 60fps rendering.


License
--------

This work is licensed under the MIT License. See `LICENSE` file for details.

Copyright (c) 2016 Dan Panzarella <alsoelp@gmail.com>