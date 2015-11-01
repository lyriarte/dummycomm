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
 * single wire dummycomm
 */
#define BTX 13
#define ERROR -1

/* 
 * ultra sonic telemeter mesure
 */
#define INECHO 2
#define TRIGGER 3
#define ECHO_TIMEOUT 100000
#define ECHO2CM(x) (x/60)
#define MAX_CM 1000

/* 
 * control loop frequency
 */
#define POLL 1000



/* **** **** **** **** **** ****
 * Global variables
 * **** **** **** **** **** ****/

unsigned long mesureCm;

int sleepms;
byte iobyte;
byte bytebits[8];

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
 * single wire dummycomm
 */
byte bytebitsGet() {
	byte value=0;
	byte power=1;
	byte i=0;
	while (i<8) {
		if (bytebits[i++])
			value = value + power;
		power = power * 2;
	}
	return value;
}

void bytebitsSet(byte value) {
	byte i=0;
	while (i<8) {
		if (value & 1)
			bytebits[i++] = 1;
		else
			bytebits[i++] = 0;
		value = value >> 1;
	}
}

void sendBytebits()
{
	byte i=8;
	while (i) {
		if (bytebits[--i])
			send1(BTX, sleepms);
		else
			send0(BTX, sleepms);
	}
}

void sendCarrier(int pin, int sleepms)
{
	digitalWrite(pin, HIGH);
	delay(sleepms);
	digitalWrite(pin, LOW);
	delay(3 * sleepms);	
}

void send0(int pin, int sleepms)
{
	digitalWrite(pin, HIGH);
	delay(2 * sleepms);
	digitalWrite(pin, LOW);
	delay(2 * sleepms);	
}

void send1(int pin, int sleepms)
{
	digitalWrite(pin, HIGH);
	delay(3 * sleepms);
	digitalWrite(pin, LOW);
	delay(sleepms);	
}

void sendByte(byte value) {
	byte i;
	digitalWrite(BTX, LOW);
	for(i=0; i<16; i++) {
		sendCarrier(BTX, sleepms);
	}
	bytebitsSet(value);
	sendBytebits();
	sendCarrier(BTX, sleepms);
	sendCarrier(BTX, sleepms);
}


/* 
 * ultra sonic telemeter mesure
 */
void telemeterMesure() {
	unsigned long echoDuration;
	digitalWrite(TRIGGER, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGGER, LOW);
	echoDuration = pulseIn(INECHO, HIGH, ECHO_TIMEOUT);
	mesureCm = echoDuration ? ECHO2CM(echoDuration) : MAX_CM;
	iobyte = mesureCm < 255 ? mesureCm : 255;
}


/* 
 * loop
 */
void loop() {
	/* control loop frequency */
	unsigned long timeLoopStart;
	unsigned long timeLoop;
	timeLoopStart = millis();
	/* telemeter mesure */
	telemeterMesure();
	/* dummy comm */
	sendByte(iobyte);
	Serial.print("mesure: ");
	Serial.println(mesureCm);
	/* control loop frequency */
	timeLoop = millis() - timeLoopStart;
	if (timeLoop < POLL)
		delay(POLL - timeLoop);
}
