

#include <DHT.h>
#include <movingAvg.h> 
#define DHTTYPE DHT11   // DHT 11 
//timers
#include <Time.h>
#include <TimeAlarms.h>


#define DHTPIN 3


#define RELAY_ON 0
#define RELAY_OFF 1
#define Relay_1  5  


DHT dht(DHTPIN, DHTTYPE);
movingAvg temperatureAvg;
movingAvg humidityAvg;

const int interval = 10*1000; // the interval between sensor reads, in ms
long lastReadTime = 0;        // the last time you read the sensor, in ms
long vccResult;

int humidity_reading;
int temperature_reading;

int humidity_avg;
int temperature_avg;

int humidity_goal = 55;
int temperature_goal = 31;


bool fanOn = 0;
long fanTimeOn = 0;

long fanCycleOn = 10 * 60;

void setup()   /*----( SETUP: RUNS ONCE )----*/
{
  Serial.begin(9600); //(Remove all 'Serial' commands if not needed)
   
  dht.begin();
  readVcc();
  readDHT();
  temperature_avg = temperatureAvg.reading(temperature_goal);

//-------( Initialize Pins so relays are inactive at reset)----
  digitalWrite(Relay_1, RELAY_OFF);
  
//---( THEN set pins as outputs )----  
  pinMode(Relay_1, OUTPUT);   
  delay(4000); //Check that all relays are inactive at Reset

  //define how many seconds to pass between sensor, relay checks
  int sensorCheckFrequency = 10;
  int relayCheckFrequency = sensorCheckFrequency * 2;
  Alarm.timerRepeat(sensorCheckFrequency, checkSensors);
  Alarm.timerRepeat(relayCheckFrequency, checkRelays);
   turnOnFan();

}/*--(end setup )---*/

void loop()   /*----( LOOP: RUNS CONSTANTLY )----*/
{

  Alarm.delay(1);
  if (fanOn) {
    if ((now() - fanTimeOn) > fanCycleOn) {
      turnOffFan();
    }
  }
  



}/* --(end main loop )-- */


void checkSensors()
{
    readDHT();

  
  Serial.print("T: "); 
  Serial.print(temperature_avg);
  Serial.print(" "); 
  Serial.println(temperature_goal);
  Serial.print("H: "); 
  Serial.print(humidity_avg);
  Serial.print(" "); 
  Serial.println(humidity_goal);
  
}



void turnOnFan() {
  digitalWrite(Relay_1, RELAY_ON);
  delay(1000);
  fanOn = 1;
  fanTimeOn = now();
}

void turnOffFan() {
  digitalWrite(Relay_1, RELAY_OFF);
  delay(1000);
  fanOn = 0;
  fanTimeOn = 0;
}

//check whether a relay needs turned on
void checkRelays()
{
  
  if (temperature_avg > temperature_goal) {
   if (!fanOn) turnOnFan();
  }
  if (humidity_avg > humidity_goal) {
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



