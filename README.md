PiWM
=====

**PiWM** is a Raspberry **Pi** **W**indow **M**anager. It does _not_ function as an X11 window manager. It is designed to be run without X11, creating raw DispmanX^* windows for your applications to run on. The applications must be purpose-designed to use PiWM, it is not a replacement to run X11 apps without X. Applications using PiWM may draw their windows using raw bitmaps, or use hardware-accelerated OpenVG. In the future, this may include OpenGL ES.

^* DispmanX is unique to Broadcom/Raspberry Pi. It is (very slowly) in the process of being phased out in favor of OpenWF, but I've been hearing that line for upwards of 4 years with no progress.



License
--------

This work is licensed under the MIT License. See `LICENSE` file for details.

Copyright (c) 2016 Dan Panzarella <alsoelp@gmail.com>