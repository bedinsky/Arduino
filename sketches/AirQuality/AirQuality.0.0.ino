
#include <DHT.h>
#include <LiquidCrystal.h>
#include <LCDKeypad.h>

#define DHTPIN 2     // what pin we're connected to
#define MQ135PIN 1   // MQ-135
#define MQ131PIN 2   // MQ-131

#define DHTTYPE DHT11   // DHT 11 

int lcd_key     = 0;
int lcd_pressed = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
// https://github.com/JChristensen/movingAvg


const int interval = 10*1000; // the interval between sensor reads, in ms
long lastReadTime = 0;        // the last time you read the sensor, in ms
long vccResult;

int mq131Reading = 0;
int mq135Reading = 0;
int mq131Max = 0;
int mq135Max = 0;

float humidityReading;
float temperatureReading;
float humidityMax = 0;
float temperatureMax = 0;

DHT dht(DHTPIN, DHTTYPE);

LCDKeypad lcd;


void setup() {
  Serial.begin(9600); 
  Serial.println("AirQuality setup!");
  readVcc();
  
  dht.begin();
 
  
  lcd.begin(16, 2);
  lcd.clear();

  readMQ135();
  readMQ131();
  readDHT();
  
  lcd_pressed = btnUP;

}

void loop() {

  // get the current time in ms:
  long currentTime = millis();
  if (currentTime > lastReadTime + interval) {
    readVcc();
    readMQ135();
    readMQ131();
    readDHT();
    lcdPrint();
    lcd_key = read_LCD_buttons();
    if (lcd_key != lcd_pressed) {
      lcd_pressed = lcd_key;
    }
    lastReadTime = millis();
  }

}

void lcdPrint() {
   
   lcd.clear();
   if (lcd_pressed == btnUP) {
    lcd.print("P: "); 
    lcd.print(mq135Reading);
    lcd.print(" (");
    lcd.print(mq135Max);
    lcd.print(")");
    lcd.setCursor(0,1);
    lcd.print("O: "); 
    lcd.print(mq131Reading);
    lcd.print(" (");
    lcd.print(mq131Max);
    lcd.print(")");
   }
   else {
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(temperatureReading) || isnan(humidityReading)) {
    lcd.println("Failed to read from DHT");
  } else {
    lcd.print("H: "); 
    lcd.print(humidityReading);
    lcd.print(" (");
    lcd.print(humidityMax);
    lcd.print(")");
    lcd.print(" % ");
     lcd.setCursor(0,1);
   lcd.print("T: "); 
    lcd.print(temperatureReading);
    lcd.print(" (");
    lcd.print(temperatureMax);
    lcd.print(")");
    lcd.print(" *C");
  }
   }

}


void serialPrint() {

    Serial.print("Pollution: "); 
    Serial.print(mq135Reading);
    Serial.print(" (");
    Serial.print(mq135Max);
    Serial.print(")");
    Serial.print("\t");
    Serial.print("Ozone: "); 
    Serial.print(mq131Reading);
    Serial.print(" (");
    Serial.print(mq131Max);
    Serial.print(")");
    Serial.println("");

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(temperatureReading) || isnan(humidityReading)) {
    Serial.println("Failed to read from DHT");
  } else {
    Serial.print("Humidity: "); 
    Serial.print(humidityReading);
    Serial.print(" (");
    Serial.print(humidityMax);
    Serial.print(")");
    Serial.print(" %\t");
    Serial.print("Temperature: "); 
    Serial.print(temperatureReading);
    Serial.print(" (");
    Serial.print(temperatureMax);
    Serial.print(")");
    Serial.print(" *C");
    Serial.println("");
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
}


// READ MQ135
float readMQ135 (){
    mq135Reading = analogRead(MQ135PIN);
    if (mq135Reading > mq135Max) {
      mq135Max = mq135Reading;
    }
    float voltage = mq135Reading * vccResult;
    voltage /= 1024.0;
    return voltage;
}

// READ MQ131
float readMQ131 (){
    mq131Reading = analogRead(MQ131PIN);
    if (mq131Reading > mq131Max) {
      mq131Max = mq131Reading;
    }
    float voltage = mq131Reading * vccResult;
    voltage /= 1024.0;
    return voltage;
}

void readDHT() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidityReading = dht.readHumidity();
    if (humidityReading > humidityMax) {
      humidityMax = humidityReading;
    }
  temperatureReading = dht.readTemperature();
    if (temperatureReading > temperatureMax) {
      temperatureMax = temperatureReading;
    }

}

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 if (adc_key_in < 50)   return btnRIGHT; 
 if (adc_key_in < 195)  return btnUP;
 if (adc_key_in < 380)  return btnDOWN;
 if (adc_key_in < 555)  return btnLEFT;
 if (adc_key_in < 790)  return btnSELECT;  
 return btnNONE;  // when all others fail, return this...
}

/*

#include <SD.h>

#include “DHT.h”
#define DHTPIN 2 // what pin we’re connected to
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const int chipSelect = 10;
void setup()
{
Serial.begin(9600);

// Serial.println(“DHTxx test!”);
dht.begin();

// Serial.print(“Initializing SD card…”);
pinMode(10, OUTPUT);

if (!SD.begin(chipSelect)) {
//Serial.println(“Card failed, or not present”);
}
// Serial.println(“card initialized.”);
}

void loop()
{

// make a string for assembling the data to log:
String dataString = “”;

//Sensor code starts

int sensor1 = 0;
int sensor2 = 1;

// TEMP- HUMIDITY CODE
int h = (int)dht.readHumidity();
int t = (int)dht.readTemperature();

// (not a number) then something went wrong!
// if (isnan(t) || isnan(h)) {
// Serial.println(“Failed to read from DHT”);
// } else {
// Serial.print(“Humidity, “);
Serial.println(h);
//Serial.print(” ,t”);
// Serial.print(“,Temperature, “);
Serial.println(t);
// Serial.println(” ,”);
// }

// GAS CODE
int g = analogRead(0); // read analog input pin 0
Serial.println(g);

delay(1000);
//END OF SENSOR CODE

//START OF STRING CODE

dataString = String(t) + “, ” + String(h) + “, ” + String(g);

// open the file. note that only one file can be open at a time,
File dataFile = SD.open(“datalog.txt”, FILE_WRITE);

// if the file is available, write to it:
if (dataFile) {
dataFile.println(dataString);
dataFile.close();
// print to the serial port too:
Serial.println(dataString);
}
// if the file isn’t open, pop up an error:
else {
Serial.println(“error opening datalog.txt”);
}
}

The value we  are measuring is a function of Vrl- the voltage.

Vrl = sensorValue * 5.0 / 1024.0. Vc is 5.0 volts when connected to the regulated power source on the Arduino.
*/
