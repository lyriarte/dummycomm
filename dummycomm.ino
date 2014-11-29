/*** 4-ticks transmission frames
* carrier : 1000
* zero: 1100
* one: 1110
*/

#define LED 13
#define BTX 0
#define BRX 1
#define CARRIER 2
#define ERROR -1

int sleepms = 20;
int bytebits[8];

void setup() {
	pinMode(LED, OUTPUT);
	pinMode(BTX, OUTPUT);
	pinMode(BRX, INPUT);
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
	int frame,i;
	digitalWrite(LED, HIGH);
	digitalWrite(BTX, LOW);
	i=8;
	while (i) {
		while((frame = getFrame(BRX)) == CARRIER) {
			i=8;
		}
		bytebits[--i] = frame;
	}
	digitalWrite(LED, LOW);
	for(i=0; i<32; i++) {
		sendCarrier(BTX, sleepms);
	}
	i=8;
	while (i) {
		if (bytebits[--i])
			send1(BTX, sleepms);
		else
			send0(BTX, sleepms);
	}
	sendCarrier(BTX, sleepms);
	sendCarrier(BTX, sleepms);
}

