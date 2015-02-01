/*** 4-ticks transmission frames
* carrier : 1000
* zero: 1100
* one: 1110
*/

#define LED 13
#define BTX 9
#define BRX 8
#define CARRIER 2
#define ERROR -1

int sleepms = 5;
byte bytebits[8];

void setup() {
	pinMode(LED, OUTPUT);
	pinMode(BTX, OUTPUT);
	pinMode(BRX, INPUT);
	Serial.begin(9600);
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

int getFrame(int pin)
{
	int cyclecount1=0;
	int cyclecount0=0;
	while (digitalRead(pin) == HIGH) {
		cyclecount1 = cyclecount1 + 1;
	}
	while (digitalRead(pin) == LOW) {
		cyclecount0 = cyclecount0 + 1;
	}
	return readFrame(cyclecount0, cyclecount1);
}

void sendBytebits()
{
	int i=8;
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

void loop() {
  	byte iobyte;
	int frame,i;
	digitalWrite(LED, HIGH);
	Serial.println("enver byte value:");
	while(Serial.available() <= 0);
	iobyte=Serial.parseInt();
	digitalWrite(LED, LOW);
	digitalWrite(BTX, LOW);
	for(i=0; i<16; i++) {
		sendCarrier(BTX, sleepms);
	}
	bytebitsSet(iobyte);
	sendBytebits();
	sendCarrier(BTX, sleepms);
	sendCarrier(BTX, sleepms);
	Serial.print("byte sent: ");
	Serial.println(iobyte);
	i=8;
	while (i) {
		while((frame = getFrame(BRX)) == CARRIER) {
			i=8;
		}
		bytebits[--i] = frame;
	}
	Serial.print("byte read: ");
	Serial.println(bytebitsGet());
}
