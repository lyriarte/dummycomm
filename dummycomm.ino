/*** 4-ticks transmission frames
* carrier : 1000
* zero: 1100
* one: 1110
*/

#define BTX 13
#define BRX 8
#define CARRIER 2
#define ERROR -1

int sleepms = 20;
byte bytebits[8];
byte bytesbuf[256];
byte hexled[] = {
  0b01111110,
  0b00010010,
  0b10111100,
  0b10110110,
  0b11010010,
  0b11100110,
  0b11101110,
  0b00110010,
  0b11111110,
  0b11110110,
  0b11111010,
  0b11001110,
  0b01101100,
  0b10011110,
  0b11101100,
  0b11101000
};

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

void setup() {
	int i=0;
	pinMode(BTX, OUTPUT);
	pinMode(BRX, INPUT);
	Serial.begin(9600);
	bytesbuf[0]=HELLO_SIZE;
	while(i<bytesbuf[0]) {
		bytesbuf[i+1]=hello[i];
		i++;
	}
	Serial.println("Sending hello sequence");
	sendBytesbuf();
}

int bytebitsGet() {
	int value=0;
	int power=1;
	int i=0;
	while (i<8) {
		if (bytebits[i++])
			value = value + power;
		power = power * 2;
	}
	return value;
}

void bytebitsSet(int value) {
	int i=0;
	while (i<8) {
		if (value & 1)
			bytebits[i++] = 1;
		else
			bytebits[i++] = 0;
		value = value >> 1;
	}
}

void bytesbufHexled() {
	byte high= bytesbuf[1] / 16;
	byte low = bytesbuf[1] % 16;
	bytesbuf[0] = 2;
	bytesbuf[1] = hexled[high];
	bytesbuf[2] = ((hexled[low] & 0x0f) << 4 ) | ((hexled[low] & 0xf0) >> 4 );
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

void sendBytesbuf() {
	int i;
	digitalWrite(BTX, LOW);
	for(i=0; i<32; i++) {
		sendCarrier(BTX, sleepms);
	}
	for(i=0; i<=bytesbuf[0]; i++) {
		bytebitsSet(bytesbuf[i]);
		sendBytebits();
	}
	sendCarrier(BTX, sleepms);
	sendCarrier(BTX, sleepms);
}

void loop() {
	int frame,i;
	Serial.println("enter size:");
	while(Serial.available() <= 0);
	bytesbuf[0]=Serial.parseInt();
	i=0;
	while(i<bytesbuf[0]) {
		while(Serial.available() <= 0);
		bytesbuf[++i]=Serial.parseInt();
	}
	if (bytesbuf[0] == 1) {
		bytesbufHexled();
	}
	sendBytesbuf();
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
