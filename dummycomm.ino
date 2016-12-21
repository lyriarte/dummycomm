/*
 * Copyright (c) 2016, Luc Yriarte
 * License: BSD <http://www.opensource.org/licenses/bsd-license.php>
 */


#include <ESP8266WiFi.h>



/* **** **** **** **** **** ****
 * Constants
 * **** **** **** **** **** ****/

#define BPS_HOST 9600
#define COMMS_BUFFER_SIZE 4096

/* 
 * serial comm to host
 */
#define HOST_SERIAL_STEP 200
#define HOST_SERIAL_TIMEOUT 15000


/*
 * http buffer content
 */
#define HEADER_END "Connection: close"
#define HEADER_END_LEN 17

#define HTTP_REQ_STEP 200
#define HTTP_REQ_TIMEOUT 2000

/* 
 * single wire dummycomm
 */
#define DUMMYCOMM_TIMEOUT 5000
#define DUMMYCOMM_STEP 2
#define BTX 2
#define BRX 0
#define CARRIER 2
#define ERROR -1

/* 
 * automaton states
 */
enum {
	START,
	CLEANUP,
	IN_SSID,
 	IN_PASSWD,
	IN_ACTION,
	IN_HOST,
	IN_PORT,
	IN_URI,
	OUT_PAGE,
	COMM_TX,
	COMM_RX
};

#define CLIENT_MODE_GET 0
#define CLIENT_MODE_QUERY 1

/* **** **** **** **** **** ****
 * Global variables
 * **** **** **** **** **** ****/

/* 
 * automaton status
 */
int currentState;
int clientMode;

/* 
 * dummycomm IO
 */
int sleepms;
byte iobyte;
byte bytebits[8];

/* 
 * serial comms buffer
 */
char commsBuffer[COMMS_BUFFER_SIZE];

/* 
 * WiFi comms
 */
char *ssid, *passwd, *host, *uri, *content, *size;
int port;
WiFiClient wifiClient;

/* 
 * Commands
 */
char *cmd;


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
	clientMode = CLIENT_MODE_GET;
	ssid = passwd = host = uri = content = size = NULL;
	port = 80;
	Serial.begin(BPS_HOST);
	sleepms = 5;
	iobyte = 128;
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

boolean hostSerialAvailable(int timeout) {
	unsigned long startTs;
	unsigned long stopTs;
	startTs = millis();
	while(!Serial.available()) {
		delay(HOST_SERIAL_STEP);
		stopTs = millis();
		if (stopTs > startTs + timeout)
			return false;
	}
	return true;
}

void wifiConnect() {
	if (WiFi.status() == WL_CONNECTED)
		return;
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

void waitWifiClientAvailable(int timeout) {
	unsigned long httpReqStartTs;
	unsigned long httpReqStopTs;
	httpReqStartTs = millis();
	while(!wifiClient.available()) {
		delay(HTTP_REQ_STEP);
		httpReqStopTs = millis();
		if (httpReqStopTs > httpReqStartTs + timeout)
			break;
	}
}


/* 
 * automaton
 */
int stateTransition(int currentState) {
	int index = 0;
	int newState = CLEANUP;
	char * input = NULL;
	switch (currentState) {
		case START:
			if (!(input = userInput("CMD: ")))
				break;
			saveString(input, &cmd);
			if (!strcmp(cmd,"TX"))
				newState = COMM_TX;
			else if (!strcmp(cmd,"RX"))
				newState = COMM_RX;
			else if (!strcmp(cmd,"SSID"))
				newState = IN_SSID;
			break;

		case COMM_TX:
			if (!(input = userInput("BYTE: ")))
				break;
			iobyte = (byte) atoi(input);
			sendByte(iobyte);
			break;

		case COMM_RX:
			iobyte = 0xff;
			if (receiveByte(DUMMYCOMM_TIMEOUT)) {
				iobyte = bytebitsGet();
				Serial.println(iobyte);
			}
			else
				Serial.println("TIMEOUT");
			break;

		case CLEANUP:
			while(Serial.readBytes(commsBuffer, COMMS_BUFFER_SIZE));
			commsBuffer[0] = 0;
			Serial.flush();
			if (ssid) free(ssid);
			if (passwd) free(passwd);
			if (host) free(host);
			if (uri) free(uri);
			if (content) free(content);
			if (size) free(size);
			if (cmd) free(cmd);
			ssid = passwd = host = uri = content = size = cmd = NULL;
			newState = START;
			break;

		case IN_SSID:
			if (!(input = userInput("SSID: ")))
				break;
			saveString(input, &ssid);
			newState = IN_PASSWD;
			break;

		case IN_PASSWD:
			if (!(input = userInput("PASSWD: ")))
				break;
			saveString(input, &passwd);
			newState = IN_ACTION;
			break;

		case IN_ACTION:
			if (!(input = userInput("ACTION: ")))
				break;
			if (!strcmp(input,"GET") || !strcmp(input,"QUERY")) {
				wifiConnect();
				newState = IN_HOST;
				if (!strcmp(input,"GET"))
					clientMode = CLIENT_MODE_GET;
				else if (!strcmp(input,"QUERY"))
					clientMode = CLIENT_MODE_QUERY;
			}
			break;

		case IN_HOST:
			if (!(input = userInput("HOST: ")))
				break;
			saveString(input, &host);
			newState = IN_PORT;
			break;

		case IN_PORT:
			if (!(input = userInput("PORT: ")))
				break;
			port = atoi(input);
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
			if (!(input = userInput("URI: ")))
				break;
			saveString(input, &uri);
			if (WiFi.status() != WL_CONNECTED
			|| !wifiClient.connected())
				break;
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
			newState = OUT_PAGE;
			break;

		case OUT_PAGE:
			waitWifiClientAvailable(HTTP_REQ_TIMEOUT);
			while(wifiClient.available())
				Serial.print(wifiClient.readStringUntil('\r'));
			if (clientMode == CLIENT_MODE_GET)
				newState = IN_ACTION;
			else if (clientMode == CLIENT_MODE_QUERY)
				newState = IN_URI;
			break;

		default:
			newState = CLEANUP;
			break;
	}
#ifdef DEBUG
	if (input) {
		Serial.print("====> ");
		Serial.println(input);
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

int readFrame(int cyclecount0, int cyclecount1)
{
	if (cyclecount0 < cyclecount1) {
		if (cyclecount1 < 2 * cyclecount0)
			return 0; // c0 < c1 < 2.c0 : 1100
		return 1; // 2.c0 < c1 : 1110
	} else {
		if (cyclecount0 < 2 * cyclecount1)
			return 0; // c1 < c0 < 2.c1 : 1100
		return CARRIER; // 2.c1 < c0 : 1000
	}
	return ERROR; // never reach
}

int getFrame(int pin, int timeout)
{
	unsigned long cyclecount1=0;
	unsigned long cyclecount0=0;
	unsigned long startTs;
	unsigned long stopTs;
	startTs = millis();
	while (digitalRead(pin) == HIGH) {
		delay(DUMMYCOMM_STEP);
		stopTs = millis();
		if (stopTs > startTs + timeout)
			return ERROR;
		cyclecount1 = cyclecount1 + 1;
	}
	while (digitalRead(pin) == LOW) {
		delay(DUMMYCOMM_STEP);
		stopTs = millis();
		if (stopTs > startTs + timeout)
			return ERROR;
		cyclecount0 = cyclecount0 + 1;
	}
	return readFrame(cyclecount0, cyclecount1);
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

boolean receiveByte(int timeout) {
	int frame,i;
	i=8;
	while (i) {
		while((frame = getFrame(BRX, timeout)) == CARRIER) {
			i=8;
		}
		if (frame == ERROR)
			return false;
		bytebits[--i] = frame;
	}
	return true;
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
