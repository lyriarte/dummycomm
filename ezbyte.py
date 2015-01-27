#!/usr/bin/python

#### 4-ticks transmission frames
# carrier : 1000
# zero: 1100
# one: 1110

#### 0..F bin strings 
hextable=[
"01111110",
"00010010",
"10111100",
"10110110",
"11010010",
"11100110",
"11101110",
"01110010",
"11111110",
"11110110",
"11111010",
"11001110",
"01101100",
"10011110",
"11101100",
"11101000"
]

import sys,time
import RPi.GPIO as GPIO

from gpio_comm import CARRIER, ERROR, sendCarrier, send0, send1
import gpio_comm

#### #### #### ####
pin = int(sys.argv[1])
value = int(sys.argv[2])
sleepms = int(sys.argv[3])

if value >255:
	print("Error: illegal input")
	exit(1)

strbits="00000010" + hextable[value/16] + hextable[value%16][4:] + hextable[value%16][:4]

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
