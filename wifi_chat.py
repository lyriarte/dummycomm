#!/usr/bin/python

import serial, sys

if len(sys.argv) < 2:
	print "Usage:", sys.argv[0], "<device> <speed> [word]*"
	exit()

device = sys.argv[1]
speed = 9600
chatscript = []
if len(sys.argv) >= 3 :
	speed = int(sys.argv[2])
if len(sys.argv) > 3 :
	chatscript = sys.argv[3:]
	chatscript.append("")

tty = serial.Serial(device, speed)
istr = ""
chat = True
while chat:
	istr += tty.read()
	if (len(istr) >= 2 and istr[len(istr)-2:] == ": "):
		print istr
		if (len(chatscript) > 0):
			ostr = chatscript.pop(0)
			print ostr
		else:
			ostr = sys.stdin.readline().strip('\n')
		if (ostr == ""):
			chat = False
		else:
			tty.write(ostr)
		istr = ""

try:
	while True:
		sys.stdout.write(tty.read())
except KeyboardInterrupt:
		pass



