#!/usr/bin/env python3
import socket
import random
import time
import code

colors = [
	b'\x00\x00\xff',
	b'\x00\xff\xff',
	b'\xff\xff\xff',
	b'\xff\x00\xff',
	b'\x00\xff\x00',
	b'\xff\xff\x00',
	b'\xff\x00\x00'
]

def r(times=1):
	return b"".join([random.choice(colors) for i in range(times)])


def rainbow(length=120):
	for i in range(length):
		s.send(b"\x02" + r(4))
		time.sleep(1/60)


s = socket.create_connection(("192.168.1.47",18455))
s.send(b"\x00")


code.interact(local=locals())