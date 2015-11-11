/*
 * Copyright (c) 2015, Luc Yriarte
 * License: BSD <http://www.opensource.org/licenses/bsd-license.php>
 */


#include <ESP8266WiFi.h>



/* **** **** **** **** **** ****
 * Constants
 * **** **** **** **** **** ****/

#define BPS_HOST 9600
#define COMMS_BUFFER_SIZE 4096


/* 
 * single wire dummycomm
 */
#define BTX 2
#define BRX 0
#define ERROR -1

/* 
 * automaton states
 */
enum {
	START,
	CLEANUP,
	IN_PASSWD,
	IN_ACTION,
	IN_HOST,
	IN_PORT,
	IN_URI,
	OUT_PAGE,
	OUT_CLIENT
};



/* **** **** **** **** **** ****
 * Global variables
 * **** **** **** **** **** ****/

/* 
 * automaton status
 */
int currentState;

/* 
 * dummycomm IO
 */
int sleepms = 5;
byte bytebits[8];
byte bytesbuf[256];


#define HELLO_SIZE 10
byte hello[] = {
142,
16,
129,
4,
240,
16,
129,
8,
1,
0
};

/* 
 * serial comms buffer
 */
char commsBuffer[COMMS_BUFFER_SIZE];

/* 
 * WiFi comms
 */
char *ssid, *passwd, *host, *uri;
int port;
WiFiClient wifiClient;


/* **** **** **** **** **** ****
 * Functions
 * **** **** **** **** **** ****/

/* 
 * setup
 */
void setup() {
	byte i=0;
	pinMode(BTX, OUTPUT);
	currentState = START;
	ssid = passwd = host = uri = NULL;
	port = 80;
	Serial.begin(BPS_HOST);
	bytesbuf[0]=HELLO_SIZE;
	while(i<bytesbuf[0]) {
		bytesbuf[i+1]=hello[i];
		i++;
	}
	Serial.println("Sending hello sequence");
	sendBytesbuf();
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
 * automaton
 */
int stateTransition(int currentState) {
	int newState = CLEANUP;
	char * userInput = NULL;
	switch (currentState) {
		case START:
			if (!(userInput = userIO("SSID: ",true)))
				break;
			saveString(userInput, &ssid);
			newState = IN_PASSWD;
			break;

		case CLEANUP:
			while(Serial.readBytes(commsBuffer, COMMS_BUFFER_SIZE));
			commsBuffer[0] = 0;
			Serial.flush();
			if (ssid) free(ssid);
			if (passwd) free(passwd);
			if (host) free(host);
			if (uri) free(uri);
			ssid = passwd = host = uri = NULL;
			newState = START;
			break;

		case IN_PASSWD:
			if (!(userInput = userIO("Passwd: ",true)))
				break;
			saveString(userInput, &passwd);
			newState = IN_ACTION;
			break;

		case IN_ACTION:
			if (!(userInput = userIO("Action: ",true)))
				break;
			if (!strcmp(userInput,"GET")) {
				if (WiFi.status() != WL_CONNECTED) {
#ifdef DEBUG
					Serial.print("Connecting to SSID: ");
					Serial.print(ssid);
					Serial.print(" passwd: ");
					Serial.println(passwd);
#endif
					WiFi.begin(ssid, passwd);
					while (WiFi.status() != WL_CONNECTED) {
						delay(500);
						Serial.print(".");
					}
				}
				newState = IN_HOST;
			}
			else if (!strcmp(userInput,"AP"))
				newState = OUT_CLIENT;
			else
				newState = CLEANUP;
			break;

		case IN_HOST:
			if (!(userInput = userIO("Host: ",true)))
				break;
			saveString(userInput, &host);
			newState = IN_PORT;
			break;

		case IN_PORT:
			if (!(userInput = userIO("Port: ",true)))
				break;
			port = atoi(userInput);
#ifdef DEBUG
			Serial.print("Connecting to ");
			Serial.print(host);
			Serial.print(":");
			Serial.println(port);
#endif
			if (wifiClient.connect(host, port))
				newState = IN_URI;
			break;

		case IN_URI:
			if (!(userInput = userIO("URI: ",true)))
				break;
			saveString(userInput, &uri);
			memset(commsBuffer,0,COMMS_BUFFER_SIZE);
			strcpy(commsBuffer, "GET ");
			strcat(commsBuffer, uri);
			strcat(commsBuffer, " HTTP/1.0\r\nHost: ");
			strcat(commsBuffer, host);
			strcat(commsBuffer, "\r\nConnection: close\r\n\r\n");
#ifdef DEBUG
			Serial.println(commsBuffer);
#endif
			wifiClient.print(commsBuffer);
			delay(200);
			newState = OUT_PAGE;
			break;

		case OUT_PAGE:
			while(wifiClient.available())
				Serial.print(wifiClient.readStringUntil('\r'));
			newState = IN_ACTION;
			break;

		case OUT_CLIENT:
			newState = OUT_CLIENT;
			break;

		default:
			newState = CLEANUP;
			break;
	}
#ifdef DEBUG
	if (userInput) {
		Serial.print("====> ");
		Serial.println(userInput);
	}
#endif
	return newState;
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

void sendBytesbuf() {
	byte i;
	digitalWrite(BTX, LOW);
	for(i=0; i<16; i++) {
		sendCarrier(BTX, sleepms);
	}
	for(i=0; i<=bytesbuf[0]; i++) {
		bytebitsSet(bytesbuf[i]);
		sendBytebits();
	}
	sendCarrier(BTX, sleepms);
	sendCarrier(BTX, sleepms);
}


/* 
 * main loop
 */
void loop() {
#ifdef DEBUG
	Serial.print("currentState: ");
	Serial.println(currentState);
#else
	Serial.println();
#endif
	currentState = stateTransition(currentState);
}
