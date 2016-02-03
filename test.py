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

CMD_OPEN = b"\x00"
CMD_DRAW = b"\x03"

s = socket.create_connection(("192.168.1.47",18455))


def send(data):
	packlen = len(data) + 4 #header size
	msg = packlen.to_bytes(4,'big') + data
	print("sending %s" % (" ".join([hex(i) for i in msg])))
	s.send(msg)


def win_open(w=16,h=2,x=5,y=9,sw=16,sh=2):
	send(CMD_OPEN + w.to_bytes(2,'big') + h.to_bytes(2,'big') + x.to_bytes(2,'big') + y.to_bytes(2,'big') + sw.to_bytes(2,'big') + sh.to_bytes(2,'big'))

def r(times=1):
	return b"".join([random.choice(colors) for i in range(times)])

def rainbow(length=120):
	for i in range(length):
		send(CMD_DRAW + r(4))
		time.sleep(1/60)




code.interact(local=locals())