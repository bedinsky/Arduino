
#include <EEPROM.h>

#define TIME_HEADER  'T'   // Header tag for serial time sync message

//#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#endif

#define USE_SERIAL_LCD
#ifdef USE_SERIAL_LCD
#endif

//growerbot version 1.0
//thanks to sparkfun (pcb layout libraries), adafruit (tsl2561, dht libraries), and arduino (community support, libraries). please support them all!

//garden name and growth profile
String growName = "BBB's garden";
String sourceName = "Bob Bedinsky";
String sourceVariety = "Spathiphyllum";
String apName = "";
String urlName = "growerbot.com";

//lcd
#include <LiquidCrystal.h>
// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(5, 6, 7, 8, 9, 10);

//rotary encoder
int encoderPin1 = 2;
int encoderPin2 = 3;
volatile int lastEncoded = 0;
volatile long encoderValue = 0;
long lastEncoderValue = 0;
int lastMSB = 0;
int lastLSB = 0;
int buttonPin = 4;
boolean buttonState = 0;
int menu = 0;
int menuSub = 0;
int pRotationIncrement = 25;
int nRotationIncrement = -25;
boolean menuChange = false;
boolean valueChange = false;
boolean subMenu = 0;
float time = 0;
int debounce = 50;
boolean previous = 0;

//light sensor
#include <Wire.h>
#include <TSL2561.h>
TSL2561 tsl(TSL2561_ADDR_FLOAT);
uint16_t ir, full, visible, luxOld, luxNew, lum;
//set target light level here
uint16_t luxGoal = 250;
//set target light proportion here
float lightProportionGoal = .7;
float lightProportion = 0;


//seconds during which light level is at or above minimum lux
float timeBrightEnough = 0;
//number of seconds between turning lights off (makes sure we don't keep our assisted lighting on when it's bright enough naturally for the plants)
int turnOffLightFrequency = 1200;


//temp, humidity sensor
#define DHTPIN 11
#include <DHT.h>
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);



float humidOld, humidNew, tempOld, tempNew;

//moisture probes
int moisturePin = 3;
int moistOld = 0, moistNew = 0;
//set target moisture level here
int moistGoal = 200;

//total time elapsed
float timeTotal = 0;
float timeNew = 0;
float timeOld = 0;
int daysLeft = 30, daysElapsed = 0;


boolean synchronised = 0;

//indicates current status of light. ? todo switch to just reading if relay pin is high or low?
boolean lightOn = 0;
//seconds light is on
long lightTimeOn = 0;
// amounts of time for light to turn on in seconds.
long lightPeriodOn = 3600 * 3;
int lightHour = 3;

// seconds pump is on
long pumpTimeOn = 0;
//indicates current status of pump. ? todo switch to just reading if relay pin is high or low?
boolean pumpOn = 0;
// amounts of time for pump to turn on in seconds.
long pumpPeriodOn = 60 * 10;

long pumpFrequency = 86400 * 3;
long synchFrequency = 3600;



//relays
int relayWater = 12;
int relayLight = 13;

//imp 
#include <SoftwareSerial.h>
SoftwareSerial softSerial(14,15); //rx, tx

//timers
#include <Time.h>
#include <TimeAlarms.h>

//variables for input from server
boolean stringComplete = false;
String inputString = "";
String toSend;

inline const char * const BoolToString(bool b)
{
  return b ? "true" : "false";
}

void setup()
{
  inputString.reserve(200);
  
  //lcd
  lcd.begin(16, 2); 
  lcd.clear();
  lcd.print("growerbot v1.0");
  
  //rotary
  pinMode(encoderPin1, INPUT);
  pinMode(encoderPin2, INPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(encoderPin1, HIGH);
  digitalWrite(encoderPin2, HIGH);
  attachInterrupt(0, updateEncoder, CHANGE);
  attachInterrupt(1, updateEncoder, CHANGE);
  
  //light
  tsl.setGain(TSL2561_GAIN_16X);
  tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);
  
  //temp
  pinMode(DHTPIN, INPUT);
  //dht.begin();
  
  //relays
  pinMode(relayWater, OUTPUT);
  pinMode(relayLight, OUTPUT);
  
  turnOffPump();
  turnOffLight();
  
  //define how many seconds to pass between sensor, relay checks, send data, and receive settings
  int sensorCheckFrequency = 1;
  int relayCheckFrequency = 5;
  int dataSendFrequency = 60;
  int settingReceiveFrequency = 3600;
 
  //serial
  Serial.begin(9600);
  softSerial.begin(2400);
  waitForSynch();
  
  checkSensors();
  //check water; turn on & increment time if below optimal level 
  if (moistNew < moistGoal)
  {
    turnOnPump();
  }
  
  //timers dtNBR_ALARMS 8 !!!
  //read sensors every 1 second
  Alarm.timerRepeat(sensorCheckFrequency, checkSensors);
  //check if relays should be on every 5 seconds, watering or lighting as necessary
  Alarm.timerRepeat(relayCheckFrequency, relayCheck);
  //send data to server every minute
  Alarm.timerRepeat(dataSendFrequency, sendData);
  
  Alarm.alarmRepeat(lightHour,0,0, turnOnLight);
  Alarm.timerRepeat(pumpFrequency, turnOnPump);
  
  Alarm.timerRepeat(synchFrequency, synchRequest);
  //todo: reset daily stuff, proportions every day
  Alarm.timerRepeat(86400, dayUpdate);

}

void waitForSynch() {
  boolean isReady = false;
  int i = 0;
  while (!isReady) {
    delay(1000);
    softSerial.print("DD");
    softSerialCheck();
    i++;
    isReady = synchronised || (i > 10);
  }
}

String timeStamp() {
  String buff = ""; 
  if (month() < 10 ) buff = buff + "0";
  buff = buff + String(month()) + "-"; 
  if (day() < 10 ) buff = buff + "0";
  buff = buff + String(day()) + " "; 
  if (hour() < 10 ) buff = buff + "0";
  buff = buff + String(hour()) + ":"; 
  if (minute() < 10 ) buff = buff + "0";
  buff = buff + String(minute());
  //DEBUG_PRINT(buff);
  return buff;
}


void loop()
{
  //checks state of scheduled events, as per http://answers.oreilly.com/topic/2704-how-to-create-an-arduino-alarm-that-calls-a-function/ 
  Alarm.delay(1);
  encodeDisplay();
  //check for input on softSerial
  softSerialCheck();
}

void encodeDisplay()
{
  //update display based on menu rotation
  if (encoderValue > pRotationIncrement)
  {
    if ( subMenu == 0) menu++;
    else menuSub++;
    encoderValue = 0;
    menuChange = true;
  }
  if (encoderValue < nRotationIncrement)
  {
    if ( subMenu == 0) menu--;
    else menuSub--;
    encoderValue = 0;
    menuChange = true;
  }
  
  //update lcd whenever sensor value or menu change
  if (subMenu == 0 && (menuChange == true || valueChange == true))
  {
    lcd.clear();
    lcd.setCursor(0,0);
    switch(menu) 
    {
    case -1:
      menu = 5;
      break;
    case 0:
    //grow menu
      lcd.print(growName);
      lcd.setCursor(0,1);
      lcd.print(timeStamp());
      //lcd.print(daysLeft);
      //lcd.print(" days left");
      lcd.print(" (" + String(daysElapsed) + ")");
      break;
    case 1:
    //light menu
      lcd.print("light ");
      lcd.print(luxNew);
      lcd.print(" lux");
      lcd.setCursor(0,1);
      lcd.print("proportion ");
      lcd.print(lightProportion);
      break;
    case 2:
    //moisture menu
      lcd.print("moisture level");
      lcd.setCursor(0,1);
      lcd.print(moistNew);
      break;
    case 3:
    //temp/humidity
      lcd.print(tempNew);
      lcd.print(" Celsius");
      lcd.setCursor(0,1);
      lcd.print(humidNew);
      lcd.print("% humidity");
      break;
    case 4:
    //connection
      lcd.print("AP ");
      lcd.print(apName);
      lcd.setCursor(0,1);
      lcd.print(urlName);
      break;
    case 5:
      menu = 0;
      break;
    }
  }
  
  //update lcd whenever sensor value or submenu change
  if (subMenu == 1 && (menuChange == true || valueChange == true))
  {  
    lcd.clear();
    lcd.setCursor(0,0);
    //decide which submenu to show based on main menu
    switch(menu) 
    {
    case -1:
      menu = 5;
      break;
    //growMenu's submenus
    case 0:
      switch(menuSub)
      {
        case -1:
          menuSub = 1;
          break;
        case 0:
          lcd.print("planted ");
          lcd.print(daysElapsed);
          lcd.setCursor(0,1);
          lcd.print("days ago");        
          break;
        case 1: 
          lcd.print(sourceName);
          lcd.setCursor(0,1);
          lcd.print(sourceVariety);
          break;
        case 2:
          menuSub = 0;
          break;
      }
      break;
    //lightMenu's submenus
    case 1:
      switch(menuSub)
      {
        case -1:
          menuSub = 3;
          break;
        case 0:
          lcd.print("target ");
          lcd.print(luxGoal);
          lcd.print(" lux");
          lcd.setCursor(0,1);
          lcd.print("proportion ");
          lcd.print(lightProportionGoal);
          break;
        case 1: 
          lcd.print("infrared");
          lcd.setCursor(0,1);
          lcd.print(ir);
          break;
        case 2: 
          lcd.print("visible");
          lcd.setCursor(0,1);
          lcd.print(visible);          
          break;
        case 3: 
          lcd.print("lumens");
          lcd.print(lum);
          break;
        case 4: 
          menuSub = 0;
          break;
      }
      break;
    //moistMenu's submenus
    case 2:
      lcd.print("target level");
      lcd.setCursor(0,1);
      lcd.print(moistGoal);
      break;
    //tempMenu's submenus
    case 3:
      break;
    //connMenu's submenus
    case 4:  
      break;
    case 5: 
      menu = 0;
      break;
    }
  }
  
  //read button press. use debounce to adjust. thx to arduino.cc for some of code
  buttonState=digitalRead(buttonPin);
  if (buttonState == 1 && previous == 0 && millis() - time > debounce)
  {
    if (subMenu == 1) 
    {
      subMenu = 0;
      menuSub = 0;
    }
    else subMenu = 1;
    time = millis();
    menuChange == true;
  }
  previous = buttonState;
  menuChange = false;
  valueChange = false;
}

void softSerialCheck()
{
  while (softSerial.available())
  {
    char inChar = (char)softSerial.read();
    inputString += inChar; 
    if (inChar == '\n')
    {
      stringComplete = true;
    }
  }

  if (stringComplete) 
  {
    DEBUG_PRINT(inputString);
    String subStr = "";
    char chars[] = "";
    
    subStr = parseValue(inputString,"D");
    if (sizeof(subStr) == 14) {
      char cmd[] = "";
      subStr.toCharArray(cmd, 14);
      time_t tSet = 0;
      tmElements_t tmSet;
 
      tmSet.Year = 1000 * (cmd[0] - '0') + 100 * (cmd[1] - '0') + 10 * (cmd[2] - '0') + cmd[3] - '0' - 1970;
      tmSet.Month = 10 * (cmd[4] - '0') + cmd[5] - '0';
      tmSet.Day = 10 * (cmd[6] - '0') + cmd[7] - '0';
      tmSet.Hour = 10 * (cmd[8] - '0') + cmd[9] - '0';
      tmSet.Minute = 10 * (cmd[10] - '0') + cmd[11] - '0';
      tmSet.Second = 10 * (cmd[12] - '0') + cmd[13] - '0';
      tSet = makeTime(tmSet); //convert to time_t
      setTime(tSet); //set the local time
       
      if (year() != 1970) {
        synchronised = 1;
      }
    }
    


    
    //use light proportion setting if sent from imp cloud
    subStr = parseValue(inputString,"L");
    if (sizeof(subStr) > 0) {
        subStr.toCharArray(chars, sizeof(subStr));
        lightProportionGoal = atof(chars);
    }
    
    
    //use light brightness setting if sent from imp cloud
    subStr = parseValue(inputString,"B");
    if (sizeof(subStr) > 0) {
        subStr.toCharArray(chars, sizeof(subStr));
        luxGoal = atof(chars);
    }

    
    //use moisture setting if sent from imp cloud
    subStr = parseValue(inputString,"M");
    if (sizeof(subStr) > 0) {
        subStr.toCharArray(chars, sizeof(subStr));
        moistGoal = atoi(chars);
    }
    
    
    //turn on light if Ll1 is received from imp cloud. note that normal cycling still applies: may go off moments later
    if (inputString.indexOf("l1") > -1) turnOnLight();
    //turn on pump if Pp1 is received from imp cloud. note that normal cycling still applies: may go off moments later
    if (inputString.indexOf("p1") > -1) turnOnPump();
    
    if (inputString.indexOf("l0") > -1) turnOffLight();
    if (inputString.indexOf("p0") > -1) turnOffPump();
 
 
    inputString = "";
    stringComplete = false; 
  }
}

String _parseValue(String inputString, String sep) {
  String result = String("");
  int start = inputString.indexOf(sep) + 1; 
  int end = inputString.lastIndexOf(sep);
  if (end > start) {
    result = inputString.substring(start, end);
  }
  return result;
  
}

String parseValue(String inputString, String name) {
  String result = String("");
  String from = String("<"+name+">");
  String to = String("</"+name+">");
  int start = inputString.indexOf(from) + sizeof(from); 
  int end = inputString.lastIndexOf(to);
  if (end > start) {
    result = inputString.substring(start, end);
  }
  return result;
  
}

//read light, temperature, humidity, and moisture from sensors
void checkSensors()
{
  light();
  temphum();
  moist();  
  //check if any values have changed. if so, update display.
  if (luxOld != luxNew || moistOld != moistNew || humidOld != humidNew || tempOld != tempNew) valueChange = true;
}

void light()
{
  luxOld = luxNew;
  lum = tsl.getFullLuminosity();
  ir = lum >> 16;
  full = lum & 0xFFFF;
  visible = full - ir;
  luxNew = tsl.calculateLux(full, ir);
}

void temphum()
{
  humidOld = humidNew;
  tempOld = tempNew;
  
  delay(2000);
  
  //Serial.print("Requesting data...");
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) ) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  else {
    humidNew = h;
    tempNew = t;
    float hic = dht.computeHeatIndex(t,h,false);
  }
  
 
  
}

void moist()
{
  moistOld = moistNew;
  moistNew = analogRead(moisturePin);
  DEBUG_PRINT("moisture level " + String(moistNew));
}

//read encoder turning
void updateEncoder()
{
  int MSB = digitalRead(encoderPin1); //MSB = most significant bit
  int LSB = digitalRead(encoderPin2); //LSB = least significant bit
  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) encoderValue ++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) encoderValue --;
  lastEncoded = encoded; //store this value for next time
}

//check whether a relay needs turned on
void relayCheck()
{
  //update time elapsed in total
  timeTotal = now();
  timeNew = timeTotal - timeOld;

  if (lightOn) {
    if ((timeTotal - lightTimeOn) > lightPeriodOn) {
      turnOffLight();
    }
  }
  
  if (pumpOn) {
    if ((timeTotal - pumpTimeOn) > pumpPeriodOn) {
      turnOffPump();
    }
  }
  


  //light
  //if luxNew > luxGoal, add timeNew to timeBrightEnough
  if (luxNew >= luxGoal)
  {
    timeBrightEnough = timeBrightEnough + timeNew;
    //Serial.print("time counts");
    //DEBUG_PRINT(timeBrightEnough);
    //add to timeLightOn if light is on
  }
  //calculate lightProportion
  lightProportion = timeBrightEnough / timeTotal;
  
  //turn on light if proportion < goal //and (luxNew < luxGoal or light is on)
  if (lightProportion < lightProportionGoal && luxNew < luxGoal)
  {
    // turnOnLight();
  }
  //turn off light if lightroportion > lightproportiongoal
  if (lightProportion >= lightProportionGoal) {
    // turnOffLight();
  }
  
  
  timeOld = now();
}

void sendData()
{
  toSend  = String("");
  if (!synchronised) {
    toSend = String("DD");
  }

  char temp[10];
  dtostrf(tempNew,1,2,temp);
  char hum[10];
  dtostrf(humidNew,1,2,hum);
  toSend = String(toSend + "L" + luxNew + "L" + "M" + moistNew + "M" + "T" + temp + "T" + "H" + hum + "H");

  if (lightOn) {
     toSend = toSend + "l1";
  }
  else {
     toSend = toSend + "l0";
  }
  if (pumpOn) {
     toSend = toSend + "p1";
  }
  else {
     toSend = toSend + "p0";
  }
  
  DEBUG_PRINT(toSend);
  softSerial.print(toSend);
}


void turnOnLight() {
  if (!lightOn) {
    DEBUG_PRINT("turning light on");
    digitalWrite(relayLight, HIGH);
    delay(1000);
    lightOn = 1;
    lightTimeOn = now();
  }
}

void turnOffLight() {
  DEBUG_PRINT("turning light off");
  digitalWrite(relayLight, LOW);
  delay(1000);
  lightOn = 0;
  lightTimeOn = 0;
}

void turnOnPump() {
  if (!pumpOn) {
    DEBUG_PRINT("turning pump on");
    digitalWrite(relayWater, HIGH);
    delay(1000);
    pumpOn = 1;
    pumpTimeOn = now();
  }
}

void turnOffPump() {
  DEBUG_PRINT("turning pump off");
  digitalWrite(relayWater, LOW);
  delay(1000);
  pumpOn = 0;
  pumpTimeOn = 0;
}

void dayUpdate()
{
  //daysLeft = daysLeft--;
  daysElapsed = daysElapsed++;
  timeBrightEnough = 0;
}

void synchRequest() {
  synchronised = 0;
}
