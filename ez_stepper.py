#!/usr/bin/python

#### 8 steps stepper on ezboard PIC18F252
#
## inputs
# pinAL on A4: step
# pinAH on A5: select clkw / cclw
# pinCTRL on C0
#
## outputs
# stepper on A0..A3
# led on C0: control state, set pinAL to step
# led on C1: clockwise
# led on C7: stepping
#

import sys,time
import RPi.GPIO as GPIO


#### #### #### ####

usage = "Usage: ez_stepper.py <pinAL> <pinAH> <pinCTRL> <ms>"

if len(sys.argv) < 5 :
	print usage
	exit(1)

pinAL = int(sys.argv[1])
pinAH = int(sys.argv[2])
pinCTRL = int(sys.argv[3])
sleepms = int(sys.argv[4])



#### #### #### ####


# restart from state 0 with pinAL and pinAH High
def reset():
	# state 8,9,10,18 x 11 -> state 0
	GPIO.output(pinAL, True)
	GPIO.output(pinAH, True)
	if not GPIO.input(pinCTRL):
		GPIO.wait_for_edge(pinCTRL, GPIO.RISING)


# select direction from state 0
def selectDirection(clkw):
	# state 8,9,10,18 x 11 -> state 0
	reset()
	# step clkw from state 0
	if clkw: 
		return
	# state 0 x 10 -> state 9
	GPIO.output(pinAL, False)
	if GPIO.input(pinCTRL):
		GPIO.wait_for_edge(pinCTRL, GPIO.FALLING)
	# state 9 x 00 -> state 10
	GPIO.output(pinAH, False)
	# step cclw from state 10
	if not GPIO.input(pinCTRL):
		GPIO.wait_for_edge(pinCTRL, GPIO.RISING)


# pinAL High then Low
# states 0->1..8->0 or 10->11..18->10
def step():
	# state 0,10 x 01 -> state 1,11
	GPIO.output(pinAH, False)
	GPIO.output(pinAL, True)
	time.sleep(0.001 * sleepms)
	GPIO.output(pinAL, False)
	time.sleep(0.001 * sleepms)




#### #### #### ####


GPIO.setwarnings(False)
GPIO.setmode(GPIO.BCM) 
GPIO.setup(pinAL, GPIO.OUT, initial=False)
GPIO.setup(pinAH, GPIO.OUT, initial=False)
GPIO.setup(pinCTRL, GPIO.IN)

steps = -1
while steps != 0:
	try:
		steps = int(raw_input("steps: "))
	except Exception:
		steps = 0
	# negative steps for counter clockwise
	if steps >= 0:
		selectDirection(True)
		print(str(steps) + " steps clockwise")
	else:
		selectDirection(False)
		steps = -steps
		print(str(steps) + " steps counter clockwise")
	# perform required number of steps
	for i in range(0,steps):
		# wait for control state before next step
		if not GPIO.input(pinCTRL):
			GPIO.wait_for_edge(pinCTRL, GPIO.RISING)
		step()


GPIO.cleanup()
