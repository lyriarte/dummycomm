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


/* 
 * global variables
 */
unsigned long mesureCm;
int sleepms;



/* 
 * functions
 */
void setup() {
	pinMode(INECHO, INPUT);
	pinMode(TRIGGER, OUTPUT);
	Serial.begin(9600);
	sleepms = 5;
	digitalWrite(TRIGGER, LOW);
}

void telemeterMesure() {
	unsigned long echoDuration;
	digitalWrite(TRIGGER, HIGH);
	delayMicroseconds(10);
	digitalWrite(TRIGGER, LOW);
	echoDuration = pulseIn(INECHO, HIGH, ECHO_TIMEOUT);
	mesureCm = echoDuration ? ECHO2CM(echoDuration) : MAX_CM;
}


void loop() {
	/* control loop frequency */
	unsigned long timeLoopStart;
	unsigned long timeLoop;
	timeLoopStart = millis();
	/* telemeter mesure */
	telemeterMesure();
	/* serial comm */
	Serial.print("mesure: ");
	Serial.println(mesureCm);
	/* control loop frequency */
	timeLoop = millis() - timeLoopStart;
	if (timeLoop < POLL)
		delay(POLL - timeLoop);
}
