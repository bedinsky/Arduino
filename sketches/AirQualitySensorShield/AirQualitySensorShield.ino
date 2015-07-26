#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <movingAvg.h> //https://github.com/JChristensen/movingAvg


/*-----( Declare objects )-----*/

#define lcdAddr 0x20 // set the address of the I2C device the LCD is connected to

// create an lcd instance with correct constructor for how the lcd is wired to the I2C chip
//LiquidCrystal_I2C lcd(0x27, 4, 5, 6, 0, 1, 2, 3, 7, POSITIVE); // addr, EN, RW, RS, D4, D5, D6, D7, Backlight, POLARITY
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
//LiquidCrystal_I2C lcd(0x27,16,2); // set the LCD address to 0x27
#define DHTTYPE DHT11   // DHT 11 


/*-----( Declare Constants, Pin Numbers )-----*/

#define DHTPIN 2
#define MQ135PIN 0   // MQ-135
#define MQ131PIN 1   // MQ-131
#define MQ4PIN 2     // MQ-4


DHT dht(DHTPIN, DHTTYPE);
movingAvg temperatureAvg;
movingAvg humidityAvg;
movingAvg mq135Avg;
movingAvg mq131Avg;
movingAvg mq4Avg;

const int interval = 10*1000; // the interval between sensor reads, in ms
long lastReadTime = 0;        // the last time you read the sensor, in ms
long vccResult;

int humidity_reading;
int temperature_reading;
int mq4_reading = 0;
int mq131_reading = 0;
int mq135_reading = 0;



int humidity_avg;
int temperature_avg;
int mq4_avg = 0;
int mq131_avg = 0;
int mq135_avg = 0;


void setup()   /*----( SETUP: RUNS ONCE )----*/
{
  Serial.begin(9600); //(Remove all 'Serial' commands if not needed)

  
  dht.begin();
  readVcc();
    readDHT();
    readMQ4();
    readMQ131();
    readMQ135();
 lcd.begin(16,2); // initialize the lcd
 lcd.backlight();
  // Print a message to the LCD.
  lcd.print("DHT11 Temp/Humid");
  lcd.setCursor(0,1);
  lcd.print("MQ131 MQ135 MQ4");
  //lcd.noBacklight();
 
}/*--(end setup )---*/

void loop()   /*----( LOOP: RUNS CONSTANTLY )----*/
{

  long currentTime = millis();
  if (currentTime > lastReadTime + interval) {
    lcd.backlight();
    readDHT();
    readMQ4();
    readMQ131();
    readMQ135();
  lcdPrintReading("T", temperature_reading, temperature_avg);
  lcdPrintReading("H", humidity_reading, humidity_avg);
  lcdPrintReading("O", mq131_reading, mq131_avg);
  lcdPrintReading("Q", mq135_reading, mq135_avg);
  lcdPrintReading("M", mq4_reading, mq4_avg);
    lastReadTime = millis();
      lcd.clear();
      lcd.noBacklight();
  }

  
  //readPIR();

}/* --(end main loop )-- */

/*-----( Declare User-written Functions )-----*/
//
//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
        return 1.8 * celsius + 32;
}

//Celsius to Kelvin conversion
double Kelvin(double celsius)
{
        return celsius + 273.15;
}

// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm 
double dewPoint(double celsius, double humidity)
{
        double A0= 373.15/(273.15 + celsius);
        double SUM = -7.90298 * (A0-1);
        SUM += 5.02808 * log10(A0);
        SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
        SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
        SUM += log10(1013.246);
        double VP = pow(10, SUM-3) * humidity;
        double T = log(VP/0.61078);   // temp var
        return (241.88 * T) / (17.558-T);
}

// delta max = 0.6544 wrt dewPoint()
// 5x faster than dewPoint()
// reference: http://en.wikipedia.org/wiki/Dew_point
double dewPointFast(double celsius, double humidity)
{
        double a = 17.271;
        double b = 237.7;
        double temp = (a * celsius) / (b + celsius) + log(humidity/100);
        double Td = (b * temp) / (a - temp);
        return Td;
}

void lcdPrintReading(String name, int value, int avg) {
    //if (value > avg) {
      //lcd.backlight();
      lcd.clear();
      lcd.print(name); 
      lcd.print(": "); 
      lcd.print(value);
      lcd.print(" (");
      lcd.print(avg);
      lcd.print(")");
      delay(2000);
      //lcd.clear();
      //lcd.noBacklight();
    //}
}

void readDHT() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  humidity_reading = (int) dht.readHumidity();
  temperature_reading = (int) dht.readTemperature();
  if (!isnan(humidity_reading)) {
    humidity_avg = humidityAvg.reading(humidity_reading);
  }
  if (!isnan(temperature_reading)) {
    temperature_avg = temperatureAvg.reading(temperature_reading);
  }
}

void readMQ4() {
  mq4_reading = readSensorVoltage(MQ4PIN) * 1000;
  mq4_avg = mq4Avg.reading(mq4_reading);
}
void readMQ131() {
  mq131_reading = readSensorVoltage(MQ131PIN) * 1000;
  mq131_avg = mq131Avg.reading(mq131_reading);
}
void readMQ135() {
  mq135_reading = readSensorVoltage(MQ135PIN) * 1000;
  mq135_avg = mq135Avg.reading(mq135_reading);
}

float readSensorVoltage (int pin){
  float sensor_reading = analogRead(pin);
  float voltage = sensor_reading * vccResult;
  voltage /= 1024.0;
/*  
  Serial.print("Pin: "); 
  Serial.print(pin);
  Serial.print(" "); 
  Serial.print("Value: "); 
  Serial.println(voltage);
*/  
  
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

