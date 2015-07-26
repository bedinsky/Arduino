

#include <DHT.h>
#include <movingAvg.h> 
#define DHTTYPE DHT11   // DHT 11 
//timers
#include <Time.h>
#include <TimeAlarms.h>


#define DHTPIN 4
#define MQ4PIN 1     // MQ-4


#define RELAY_ON 0
#define RELAY_OFF 1
#define Relay_1  2  
#define Relay_2  3


DHT dht(DHTPIN, DHTTYPE);
movingAvg temperatureAvg;
movingAvg humidityAvg;
movingAvg mq4Avg;

const int interval = 10*1000; // the interval between sensor reads, in ms
long lastReadTime = 0;        // the last time you read the sensor, in ms
long vccResult;

int humidity_reading;
int temperature_reading;
int mq4_reading;

int humidity_avg;
int temperature_avg;
int mq4_avg;

int humidity_goal = 65;
int temperature_goal = 20;
int mq4_goal = 200;


bool fanOn = 0;
bool fanOpen = 0;
long fanTimeOn = 0;
long fanTimeOpen = 0;

long fanCycleOn = 10 * 60;
long fanCycleOpen = 30 * 60;

void setup()   /*----( SETUP: RUNS ONCE )----*/
{
  Serial.begin(9600); //(Remove all 'Serial' commands if not needed)
   
  dht.begin();
  readVcc();
  readDHT();
  mq4_avg = mq4Avg.reading(mq4_goal);
  temperature_avg = temperatureAvg.reading(temperature_goal);

//-------( Initialize Pins so relays are inactive at reset)----
  digitalWrite(Relay_1, RELAY_OFF);
  digitalWrite(Relay_2, RELAY_OFF);
  
//---( THEN set pins as outputs )----  
  pinMode(Relay_1, OUTPUT);   
  pinMode(Relay_2, OUTPUT);  
  delay(4000); //Check that all relays are inactive at Reset

  //define how many seconds to pass between sensor, relay checks
  int sensorCheckFrequency = 10;
  int relayCheckFrequency = sensorCheckFrequency * 2;
  Alarm.timerRepeat(sensorCheckFrequency, checkSensors);
  Alarm.timerRepeat(relayCheckFrequency, checkRelays);

}/*--(end setup )---*/

void loop()   /*----( LOOP: RUNS CONSTANTLY )----*/
{

  //checks state of scheduled events, as per http://answers.oreilly.com/topic/2704-how-to-create-an-arduino-alarm-that-calls-a-function/ 
  Alarm.delay(1);
  if (fanOn) {
    if ((now() - fanTimeOn) > fanCycleOn) {
      turnOffFan();
    }
  }
  else {
    if ((now() - fanTimeOpen) > fanCycleOpen) {
      closeFan();
    }
  }
  



}/* --(end main loop )-- */


void checkSensors()
{
    readDHT();
    readMQ4();

  /*
  Serial.print("T: "); 
  Serial.println(temperature_avg);
  Serial.println(temperature_goal);
  Serial.print("H: "); 
  Serial.println(humidity_avg);
  Serial.println(humidity_goal);
  Serial.print("MQ4: "); 
  Serial.println(mq4_reading);
  Serial.println(mq4_goal);
  */
}

void openFan() {
  digitalWrite(Relay_1, RELAY_ON);
  delay(1000);
  fanOpen = 1;
  fanTimeOpen = now();
}

void closeFan() {
  digitalWrite(Relay_1, RELAY_OFF);
  delay(1000);
  fanOpen = 0;
  fanTimeOpen = 0;

}


void turnOnFan() {
  digitalWrite(Relay_2, RELAY_ON);
  delay(1000);
  fanOn = 1;
  fanTimeOn = now();
}

void turnOffFan() {
  digitalWrite(Relay_2, RELAY_OFF);
  delay(1000);
  fanOn = 0;
  fanTimeOn = 0;
}

//check whether a relay needs turned on
void checkRelays()
{
  
  bool poorAirQuality = ((humidity_avg > humidity_goal)||(mq4_reading >= mq4_goal));
  
  if (temperature_avg > temperature_goal) {
    if (!fanOpen) openFan();
  }
 
  if (poorAirQuality) {
    if (!fanOpen) openFan();
    if (!fanOn) turnOnFan();
  }
 
}



void readDHT() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity_reading = (int) dht.readHumidity();
  temperature_reading = (int) dht.readTemperature();
  if (!isnan(humidity_reading)) {

    if (humidity_reading > 0) {
      humidity_avg = humidityAvg.reading(humidity_reading);
    }

  }
  if (!isnan(temperature_reading)) {
    if (temperature_reading > 0) {
      temperature_avg = temperatureAvg.reading(temperature_reading);
    }
  }
}

void readMQ4() {
  mq4_reading = readSensorVoltage(MQ4PIN) * 1000;
  if (mq4_reading > 0) {
    mq4_avg = mq4Avg.reading(mq4_reading);
  }
  mq4_goal = ((float)mq4_avg * 1.5);
}

float readSensorVoltage (int pin){
  float sensor_reading = analogRead(pin);
  float voltage = sensor_reading * vccResult;
  voltage /= 1024.0;
  return voltage;
}

// To read internal voltage from arduino
long readVcc() {
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  vccResult = ADCL;
  vccResult |= ADCH<<8;
  vccResult = 1126400L / vccResult; // Back-calculate AVcc in mV
  vccResult /= 1000; // voltage is returned in millivolts, so divide by 1000
  return vccResult;
}

/* ( THE END ) */



