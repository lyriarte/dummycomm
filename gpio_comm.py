#### 4-ticks transmission frames
# carrier : 1000
# zero: 1100
# one: 1110

import sys,time
import RPi.GPIO as GPIO


#### #### #### ####
CARRIER=2
ERROR=-1


#### #### #### ####

# decode a frame
def readFrame(cyclecount0, cyclecount1):
	if cyclecount0 < cyclecount1 :
		if cyclecount1 < 2 * cyclecount0 :
			return 0 # c0 < c1 < 2.c0 : 1100
		return 1 # 2.c0 < c1 : 1110
	else :
		if cyclecount0 < 2 * cyclecount1 :
			return 0 # c1 < c0 < 2.c1 : 1100
		return CARRIER # 2.c1 < c0 : 1000
	return ERROR # never reach

# receive a frame
def getFrame(pin):
	starttime = time.time()
	cyclecount1=0
	cyclecount0=0
	while GPIO.input(pin):
		cyclecount1 = cyclecount1 + 1
		if time.time() > starttime + 30 :
			return ERROR
	while not GPIO.input(pin):
		cyclecount0 = cyclecount0 + 1
                if time.time() > starttime + 30 :
                        return ERROR
	return readFrame(cyclecount0, cyclecount1)


#### #### #### ####

def sendCarrier(pin, sleepms):
	GPIO.output(pin, True)
	time.sleep(0.001 * sleepms)
	GPIO.output(pin, False)
	time.sleep(0.003 * sleepms)

def send0(pin, sleepms):
	GPIO.output(pin, True)
	time.sleep(0.002 * sleepms)
	GPIO.output(pin, False)
	time.sleep(0.002 * sleepms)

def send1(pin, sleepms):
	GPIO.output(pin, True)
	time.sleep(0.003 * sleepms)
	GPIO.output(pin, False)
	time.sleep(0.001 * sleepms)


#### #### #### ####

