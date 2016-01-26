#!/usr/bin/env python3
import socket
import random
import time

colors = [
	b'\x00\x00\xff',
	b'\x00\xff\xff',
	b'\xff\xff\xff',
	b'\xff\x00\xff',
	b'\x00\xff\x00',
	b'\xff\xff\x00',
	b'\xff\x00\x00'
]




s = socket.create_connection(("192.168.1.47",18455))

for i in range(120):
	s.send( random.choice(colors) + random.choice(colors) + random.choice(colors) + random.choice(colors) )
	time.sleep(1/60)



time.sleep(1)
s.close()