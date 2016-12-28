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
 * serial comm to host
 */
#define HOST_SERIAL_STEP 200
#define HOST_SERIAL_TIMEOUT 10000

#define SSID "SSID"
#define PASSWD "password"
#define HOST "192.168.0.1"
#define PORT 80
#define URI "/index.html"


/* 
 * software serial comm to esp-01
 */
#define COMMS_BUFFER_SIZE 768
#define TX2 11
#define RX2 10

/* 
 * automaton states
 */
enum {
	START,
	CLEANUP,
	IN_SSID,
	SLEEP,
	SERIAL_ECHO,
	SERIAL_READ
};

/* 
 * control loop frequency
 */
#define POLL 1000



/* **** **** **** **** **** ****
 * Global variables
 * **** **** **** **** **** ****/

/* 
 * automaton status
 */
int currentState;

int sleepms;

char commsBuffer[COMMS_BUFFER_SIZE];

char *ssid, *passwd, *host, *uri;
int port;
boolean connected;

SoftwareSerial serial2(RX2, TX2);

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
	ssid = passwd = host = uri = NULL;
	connected = false;
	Serial.begin(BPS_HOST);
	serial2.begin(BPS_WIFI);
	sleepms = 5;
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

void wifiSettingsCleanup() {
	commsBuffer[0] = 0;
	if (ssid) free(ssid);
	if (passwd) free(passwd);
	if (host) free(host);
	if (uri) free(uri);
	ssid = passwd = host = uri = NULL;
}

void wifiSettingsDefault() {
	saveString(SSID, &ssid);
	saveString(PASSWD, &passwd);
	saveString(HOST, &host);
	port = PORT;
	saveString(URI, &uri);
}

boolean wifiSettingsInput() {
	Serial.print("SSID: ");
	if (!hostSerialAvailable(HOST_SERIAL_TIMEOUT)) {
		wifiSettingsDefault();
		return true;
	}
	char * input = NULL;
	if (!(input = userInput("")))
		return false;
	saveString(input, &ssid);
	if (!(input = userInput("PASSWD: ")))
		return false;
	saveString(input, &passwd);
	if (!(input = userInput("HOST: ")))
		return false;
	saveString(input, &host);
	if (!(input = userInput("PORT: ")))
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
		input = espInput("SSID");
		if (suffix(input,"SSID: "))
			chatReady = true;
	}
	input = espInput(ssid);
	if (!suffix(input,"PASSWD: "))
		return false;
	input = espInput(passwd);
	if (!suffix(input,"ACTION: "))
		return false;
	input = espInput("GET");
	if (!suffix(input,"HOST: "))
		return false;
	input = espInput(host);
	if (!suffix(commsBuffer,"PORT: "))
		return false;
	Serial.print("Sending PORT: ");
	Serial.println(port);
	serial2.print(port);
	input = espInput(NULL);
	if (!suffix(commsBuffer,"URI: "))
		return false;
	return true;
}

boolean wifiSendUri() {
	strcpy(commsBuffer, uri);
	if (! suffix(espInput(commsBuffer),"ACTION: "))
		return false;
	if (! suffix(espInput("GET"),"HOST: "))
		return false;
	if (! suffix(espInput(host),"PORT: "))
		return false;
	serial2.print(port);
	if (!suffix(espInput(NULL),"URI: "))
		return false;
	return true;
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
			Serial.print("CMD: ");
			if (!hostSerialAvailable(HOST_SERIAL_TIMEOUT)) {
				newState = IN_SSID;
				break;
			}
			if (!(input = userInput("")))
				break;
			saveString(input, &cmd);
			if (!strcmp(cmd,"SSID"))
				newState = IN_SSID;
			else if (!strcmp(cmd,"ECHO"))
				newState = SERIAL_ECHO;
			else if (!strcmp(cmd,"READ"))
				newState = SERIAL_READ;
			else if (!strcmp(cmd,"SLEEP"))
				newState = SLEEP;
			break;

		case SLEEP:
			if (!(input = userInput("SECONDS: ")))
				break;
			delay(1000 * atoi(input));
			newState = START;
			break;

		case SERIAL_ECHO:
			if (!(input = userInput("VALUE: ")))
				break;
			serial2.print(input);
			newState = START;
			break;

		case SERIAL_READ:
			if (!(input = espInput(NULL)))
				break;
			newState = START;
			break;

		case CLEANUP:
			while(Serial.readBytes(commsBuffer, COMMS_BUFFER_SIZE));
			Serial.flush();
			while(serial2.readBytes(commsBuffer, COMMS_BUFFER_SIZE));
			serial2.flush();
			commsBuffer[0] = 0;
			wifiSettingsCleanup();
			newState = START;
			break;

		case IN_SSID:
			while (!wifiSettingsInput())
				wifiSettingsCleanup();
			connected = wifiConnect();
			if (connected)
				connected = wifiSendUri();
			newState = START;
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
 * main loop
 */
void loop() {
	/* control loop frequency */
	unsigned long timeLoopStart;
	unsigned long timeLoop;
	timeLoopStart = millis();
#ifdef DEBUG
	Serial.print("currentState: ");
	Serial.println(currentState);
#else
	Serial.println();
#endif
	currentState = stateTransition(currentState);
	/* control loop frequency */
	timeLoop = millis() - timeLoopStart;
	if (timeLoop < POLL)
		delay(POLL - timeLoop);
}
