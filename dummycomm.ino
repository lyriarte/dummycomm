/*
 * Copyright (c) 2015, Luc Yriarte
 * License: BSD <http://www.opensource.org/licenses/bsd-license.php>
 */



#include <SoftwareSerial.h>



/* **** **** **** **** **** ****
 * Constants
 * **** **** **** **** **** ****/

#define BPS_HOST 9600
#define BPS_WIFI 9600

/* 
 * software serial comm to esp-01
 */
#define COMMS_BUFFER_SIZE 1024
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
boolean connected;

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
	ssid = passwd = host = uri = NULL;
	connected = false;
	Serial.begin(BPS_HOST);
	serial2.begin(BPS_WIFI);
	sleepms = 5;
	iobyte = 128;
	digitalWrite(TRIGGER, LOW);
}


/* 
 * userIO
 */
char * userInput(char * message) {
	char * input = NULL;
	int nread = 0;
	Serial.print(message);
	while (!(nread = Serial.readBytes(commsBuffer, COMMS_BUFFER_SIZE)));
	if (nread == COMMS_BUFFER_SIZE)
		return input; // buffer overflow
	commsBuffer[nread] = 0;
	input = commsBuffer;
	return input;
}

void saveString(char * srcStr, char ** dstStrP) {
	if (*dstStrP)
		free(*dstStrP);
	*dstStrP = strdup(srcStr);
}

void wifiSettingsCleanup() {
	commsBuffer[0] = 0;
	if (ssid) free(ssid);
	if (passwd) free(passwd);
	if (host) free(host);
	if (uri) free(uri);
	ssid = passwd = host = uri = NULL;
}

boolean wifiSettingsInput() {
	char * input = NULL;
	if (!(input = userInput("SSID: ")))
		return false;
	saveString(input, &ssid);
	if (!(input = userInput("Passwd: ")))
		return false;
	saveString(input, &passwd);
	if (!(input = userInput("Host: ")))
		return false;
	saveString(input, &host);
	if (!(input = userInput("Port: ")))
		return false;
	port = atoi(input);
	if (!(input = userInput("URI: ")))
		return false;
	saveString(input, &uri);
	return true;
}


/* 
 * esp-01 wifi comm
 */
char * espInput(char * message) {
	if (message) {
		Serial.print("Sending: ");
		Serial.println(message);
		serial2.print(message);
	}
	int nread = 0;
	while (!(nread = serial2.readBytes(commsBuffer, COMMS_BUFFER_SIZE)) || nread == COMMS_BUFFER_SIZE);
	commsBuffer[nread] = 0;
	Serial.print("Received: ");
	Serial.println(commsBuffer);
	return commsBuffer;
}

boolean suffix(char *str, char *suf) {
	int start = strlen(str) - strlen(suf);
	return ! strcmp(str+start,suf);
}

boolean wifiConnect() {
	char * input = NULL;
	boolean chatReady = false;
	Serial.println("Connecting...");
	while (!chatReady) {
		input = espInput("Hello...");
		if (suffix(input,"SSID: "))
			chatReady = true;
	}
	input = espInput(ssid);
	if (!suffix(input,"Passwd: "))
		return false;
	input = espInput(passwd);
	if (!suffix(input,"Action: "))
		return false;
	input = espInput("QUERY");
	if (!suffix(input,"Host: "))
		return false;
	input = espInput(host);
	if (!suffix(commsBuffer,"Port: "))
		return false;
	Serial.print("Sending Port: ");
	Serial.println(port);
	serial2.print(port);
	input = espInput(NULL);
	if (!suffix(commsBuffer,"URI: "))
		return false;
	return true;
}

boolean wifiSendMesure() {
	strcpy(commsBuffer, uri);
	strcat(commsBuffer, "?mesure=");
	itoa(mesureCm, commsBuffer + strlen(commsBuffer), 10);
	if (!suffix(espInput(commsBuffer),"URI: "))
		return false;
	return true;
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
	/* wifi settings user input */
	if (!uri) {
		connected = false;
		while (!wifiSettingsInput())
			wifiSettingsCleanup();
		connected = wifiConnect();
	}
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
	/* send mesure / wifi get url */
	if (connected)
		connected = wifiSendMesure();
	if (!connected)
		connected = wifiConnect();	
	/* control loop frequency */
	timeLoop = millis() - timeLoopStart;
	if (timeLoop < POLL)
		delay(POLL - timeLoop);
}
