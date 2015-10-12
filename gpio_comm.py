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
	if not GPIO.input(pin):
		GPIO.wait_for_edge(pin, GPIO.RISING)
	start1 = time.time()
	GPIO.wait_for_edge(pin, GPIO.FALLING)
	start0 = time.time()
	cyclecount1 = start0 - start1
	GPIO.wait_for_edge(pin, GPIO.RISING)
	cyclecount0 = time.time() - start0
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

