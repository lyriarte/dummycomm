#!/usr/bin/python

import serial, sys

if len(sys.argv) < 2:
	print "Usage:", sys.argv[0], "<device> <speed>"
	exit()

device = sys.argv[1]
speed = 9600
if len(sys.argv) == 3 :
	speed = int(sys.argv[2])

tty = serial.Serial(device, speed)
istr = ""
chat = True
while chat:
	istr += tty.read()
	if (len(istr) >= 2 and istr[len(istr)-2:] == ": "):
		print istr
		ostr = sys.stdin.readline().strip('\n')
		if (ostr == ""):
			chat = False
		else:
			tty.write(ostr)
		istr = ""

while True:
	sys.stdout.write(tty.read())

