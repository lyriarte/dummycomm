/*
 * Copyright (c) 2015, Luc Yriarte
 * License: BSD <http://www.opensource.org/licenses/bsd-license.php>
 */



#include <SoftwareSerial.h>



/* **** **** **** **** **** ****
 * Constants
 * **** **** **** **** **** ****/

/* 
 * software serial comm to esp-01
 */
#define COMMS_BUFFER_SIZE 4096
#define TX2 11
#define RX2 10

/* 
 * control loop frequency
 */
#define POLL 1000



/* **** **** **** **** **** ****
 * Global variables
 * **** **** **** **** **** ****/

int sleepms;

char commsBuffer[COMMS_BUFFER_SIZE];

char *ssid, *passwd, *host, *uri;
int port;

SoftwareSerial serial2(RX2, TX2);



/* **** **** **** **** **** ****
 * Functions
 * **** **** **** **** **** ****/

/* 
 * setup
 */
void setup() {
	pinMode(BTX, OUTPUT);
	pinMode(INECHO, INPUT);
	pinMode(TRIGGER, OUTPUT);
	Serial.begin(9600);
	serial2.begin(9600);
	sleepms = 5;
	iobyte = 128;
	digitalWrite(TRIGGER, LOW);
}


/* 
 * userIO
 */
char * userIO(char * message, boolean readInput) {
	char * input = NULL;
	int nread = 0;
	Serial.print(message);
	if (readInput) {
		while (!(nread = Serial.readBytes(commsBuffer, COMMS_BUFFER_SIZE)));
		if (nread == COMMS_BUFFER_SIZE)
			return input; // buffer overflow
		commsBuffer[nread] = 0;
		input = commsBuffer;
	}
	return input;
}

void saveString(char * srcStr, char ** dstStrP) {
	if (*dstStrP)
		free(*dstStrP);
	*dstStrP = strdup(srcStr);
}




/* 
 * loop
 */
void loop() {
	/* control loop frequency */
	unsigned long timeLoopStart;
	unsigned long timeLoop;
	timeLoopStart = millis();
	/* control loop frequency */
	timeLoop = millis() - timeLoopStart;
	if (timeLoop < POLL)
		delay(POLL - timeLoop);
}
