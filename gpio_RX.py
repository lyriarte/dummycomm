#!/usr/bin/python

#### 4-ticks transmission frames
# carrier : 1000
# zero: 1100
# one: 1110

import sys,time
import RPi.GPIO as GPIO

from gpio_comm import CARRIER, ERROR, getFrame
import gpio_comm

#### #### #### ####

pin = int(sys.argv[1])

GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM) 
GPIO.setup(pin, GPIO.IN)

frame = getFrame(pin)
# pass carrier frames
while frame == CARRIER :
	frame = getFrame(pin)

# print data frames
while frame != CARRIER :
	if frame == ERROR :
		print "ERROR"
		exit(1)
	print frame
	frame = getFrame(pin)

GPIO.cleanup()
