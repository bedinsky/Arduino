/*

NYU ITP - Sensor Workshop - Tom Igoe (fall 2012)
 Datalogging: Manuela Donoso & Luca Shapiro
http://itp.nyu.edu/physcomp/sensors/Reports/MQ135
 CODE TO MEASURE AIR POLLUTION WITH: MQ-135, HIH-4030, AND TMP-36

 Based on: 
 Code by Wiring.org:
 http://wiring.org.co/learning/basics/airqualitymq135.html
 Code by Adafruit:
 http://www.ladyada.net/learn/sensors/tmp36.html
 Secret Voltmeter:
 http://code.google.com/p/tinkerit/wiki/SecretVoltmeter
 */

int pollutionPin = 0; // MQ-135
int humidityPin = 1; // HIH-4030
int tempPin = 2; // TMP-36

int pollutionReading = 0;
int humidityReading = 0;
int tempReading = 0;

float temperatureC = 0;
float temperatureF = 0;
//const int chipSelect = 4; //SD card pin
const int interval = 10*100; // the interval between sensor reads, in ms
long lastReadTime = 0;        // the last time you read the sensor, in ms
long vccResult;


void setup() {
  Serial.begin(9600);
  Serial.println ("co2(v) \t rH(%) \t temp(*C)");
}


void loop() {

  // get the current time in ms:
  long currentTime = millis();
  if (currentTime > lastReadTime + interval) {

    readVcc();

    // READ POLLUTION
    pollutionReading = analogRead(pollutionPin);
    float pollutionVoltage = pollutionReading * vccResult;
    pollutionVoltage /= 1024.0;
    Serial.print(pollutionVoltage);

    // READ HUMIDITY
    humidityReading = analogRead(humidityPin);
    float humidityVoltage = humidityReading * vccResult;
    humidityVoltage /= 1024.0;

    // convert to percentage
    float humidityPercentage = humidityVoltage * 100;
    humidityPercentage /= vccResult;
    Serial.print("\t");
    Serial.print(humidityPercentage);

    // READ TEMPERATURE
    tempReading = analogRead(tempPin);

    // convert readings to voltage, using 5V battery
    float tempVoltage = tempReading * vccResult;
    tempVoltage /= 1024.0;

    // now print out the temperature
    temperatureC = (tempVoltage - 0.5) * 100 ;
    Serial.print("\t");  //print in other column
    Serial.println(temperatureC);

    // now convert to Fahrenheight
    //temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
    //Serial.println("Degrees F:"); 
    //Serial.println(temperatureF); 

    // update the time of the most current reading:
    lastReadTime = millis();
  }
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
  Serial.println("VCC is: "); 
  Serial.println(vccResult);
}