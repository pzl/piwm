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



s = socket.create_connection(("192.168.1.47",18455))


def send(data):
	packlen = len(data) + 4 #header size
	s.send(packlen.to_bytes(4,'big') + data)


def win_open():
	send(b"\x00")

def r(times=1):
	return b"".join([random.choice(colors) for i in range(times)])

def rainbow(length=120):
	for i in range(length):
		send(b"\x02" + r(4))
		time.sleep(1/60)




code.interact(local=locals())