#!/usr/bin/python

#### 4-ticks transmission frames
# carrier : 1000
# zero: 1100
# one: 1110

import sys,time
import RPi.GPIO as GPIO

from gpio_comm import CARRIER, ERROR, sendCarrier, send0, send1
import gpio_comm

#### #### #### ####

usage = "Usage: gpio_TX.py <pin> <bits> <ms>"

if len(sys.argv) < 4 :
	print usage
	exit(1)


pin = int(sys.argv[1])
strbits = sys.argv[2]
sleepms = int(sys.argv[3])


#### #### #### ####

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM) 
GPIO.setup(pin, GPIO.OUT)

# send 32 carrier frames
for i in range(0,32):
	sendCarrier(pin, sleepms)

for c in strbits:
	if c == '0':
		send0(pin, sleepms)
	else:
		send1(pin, sleepms)

# send 2 carrier frames to finish
sendCarrier(pin, sleepms)
sendCarrier(pin, sleepms)

GPIO.cleanup()
