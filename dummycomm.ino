/*** 4-ticks transmission frames
* carrier : 1000
* zero: 1100
* one: 1110
*/

#define BTX 13
#define BRX 8
#define STEP1 5
#define STEP2 4
#define STEP3 3
#define STEP4 2
#define CARRIER 2
#define NOFRAME 3
#define NOFRAME_TRESHOLD 0xFFFF
#define ERROR -1

int sleepms;
byte iobyte;
byte bytebits[8];

int stepperms = 5;
byte steps8[] = {
  HIGH,  LOW,  LOW,  LOW,
  HIGH, HIGH,  LOW,  LOW,
   LOW, HIGH,  LOW,  LOW,
   LOW, HIGH, HIGH,  LOW,
   LOW,  LOW, HIGH,  LOW, 
   LOW,  LOW, HIGH, HIGH,
  HIGH,  LOW,  LOW, HIGH,
};

void step8(byte pin1, byte pin2, byte pin3, byte pin4) {
	byte i=0;
	while (i<32) {
		digitalWrite(pin1, steps8[i++]);
		digitalWrite(pin2, steps8[i++]);
		digitalWrite(pin3, steps8[i++]);
		digitalWrite(pin4, steps8[i++]);
		delay(stepperms);
	}
}  

void stepClkw(byte pin1, byte pin2, byte pin3, byte pin4) {
	step8(pin4, pin3, pin2, pin1);
}

void stepCclw(byte pin1, byte pin2, byte pin3, byte pin4) {
	step8(pin1, pin2, pin3, pin4);
}

void setup() {
	pinMode(STEP1, OUTPUT);
	pinMode(STEP2, OUTPUT);
	pinMode(STEP3, OUTPUT);
	pinMode(STEP4, OUTPUT);
	pinMode(BTX, OUTPUT);
	pinMode(BTX, OUTPUT);
	pinMode(BRX, INPUT);
	sleepms = 5;
	iobyte = 128;
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

byte readFrame(unsigned int cyclecount0, unsigned int cyclecount1)
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

byte getFrame(int pin)
{
	unsigned int cyclecount1=0;
	unsigned int cyclecount0=0;
	while (digitalRead(pin) == HIGH) {
		cyclecount1 = cyclecount1 + 1;
	}
	while (digitalRead(pin) == LOW) {
		cyclecount0 = cyclecount0 + 1;
		if (cyclecount0 == NOFRAME_TRESHOLD)
			return NOFRAME;
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

byte getByte() {
	byte frame,i;
	i=8;
	while (i) {
		while((frame = getFrame(BRX)) >= CARRIER) {
			if (frame == NOFRAME) {
				if (Serial.available() > 0) {
					frame = Serial.parseInt();
					return frame;
				}
			}
			i=8;
		}
		bytebits[--i] = frame;
	}
	return bytebitsGet();
}

void loop() {
	byte i;
	sendByte(iobyte);
	Serial.print("byte sent: ");
	Serial.println(iobyte);
	iobyte=getByte();
	Serial.print("byte read: ");
	Serial.println(iobyte);
	for(i=0; i<iobyte; i++) {
		stepClkw(STEP1,STEP2,STEP3,STEP4);
	}
	for(i=0; i<iobyte; i++) {
		stepCclw(STEP1,STEP2,STEP3,STEP4);
	}
}
