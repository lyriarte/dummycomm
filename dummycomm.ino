/*** 4-ticks transmission frames
* carrier : 1000
* zero: 1100
* one: 1110
*/

#define BTX 13
#define ERROR -1

#define INECHO 2
#define TRIGGER 3
#define ECHO_TIMEOUT 100000
#define ECHO2CM(x) (x/60)

int sleepms;
byte iobyte;
byte bytebits[8];

void setup() {
	pinMode(BTX, OUTPUT);
	pinMode(INECHO, INPUT);
	pinMode(TRIGGER, OUTPUT);
	Serial.begin(9600);
	sleepms = 5;
	iobyte = 128;
	digitalWrite(TRIGGER, LOW);
}

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

void telemeterByte() {
	unsigned long echoDuration;
	unsigned long centimeter;
	digitalWrite(TRIGGER, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGGER, LOW);
	echoDuration = pulseIn(INECHO, HIGH, ECHO_TIMEOUT);
	centimeter = ECHO2CM(echoDuration);
	iobyte = centimeter < 255 ? centimeter : 255;
}


void loop() {
	sendByte(iobyte);
	Serial.print("byte sent: ");
	Serial.println(iobyte);
	telemeterByte();
}
