#include <Time.h>



//include the datetime library, so our garduino can keep track of how long the lights are on
// #include <DateTime.h>

//define analog inputs to which we have connected our sensors
int moistureSensor = 0;
int lightSensor = 1;
int tempSensor = 2;

//define digital outputs to which we have connecte our relays (water and light) and LED (temperature)
int waterPump = 7;
int lightSwitch = 8;
int tempLed = 2;

//define variables to store moisture, light, and temperature values
int moisture_val;
int light_val;
int temp_val;

//decide how many hours of light your plants should get daily
float hours_light_daily_desired = 14;

//calculate desired hours of light total and supplemental daily based on above values
float proportion_to_light = hours_light_daily_desired / 24;
float seconds_light = 0;
float proportion_lit;

//setup a variable to store seconds since arduino switched on
float start_time;
float seconds_elapsed;
float seconds_elapsed_total;
float seconds_for_this_cycle;

void setup() {
//open serial port
Serial.begin(9600);
//set the water, light, and temperature pins as outputs that are turned off
pinMode (waterPump, OUTPUT);
pinMode (lightSwitch, OUTPUT);
pinMode (tempLed, OUTPUT);
digitalWrite (waterPump, LOW);
digitalWrite (lightSwitch, LOW);
digitalWrite (tempLed, LOW);

//establish start time
start_time = now();
seconds_elapsed_total = 0;

}
void loop() {
// read the value from the moisture-sensing probes, print it to screen, and wait a second
moisture_val = analogRead(moistureSensor);
Serial.print("moisture sensor reads ");
Serial.println( moisture_val );
delay(1000);
// read the value from the photosensor, print it to screen, and wait a second
light_val = analogRead(lightSensor);
Serial.print("light sensor reads ");
Serial.println( light_val );
delay(1000);
// read the value from the temperature sensor, print it to screen, and wait a second
temp_val = analogRead(tempSensor);
Serial.print("temp sensor reads ");
Serial.println( temp_val );
delay(1000);
Serial.print("seconds total = ");
Serial.println( seconds_elapsed_total );
delay(1000);
Serial.print("seconds lit = ");
Serial.println( seconds_light);
delay(1000);
Serial.print("proportion desired = ");
Serial.println( proportion_to_light);
delay(1000);
Serial.print("proportion achieved = ");
Serial.println( proportion_lit);
delay(1000);

//turn water on when soil is dry, and delay until soil is wet
if (moisture_val < 850)
{
digitalWrite(waterPump, HIGH);
}

while (moisture_val < 850)
{
digitalWrite(tempLed, HIGH);
delay(10000);
//thanks to JoshTW for the following, important correction
moisture_val = analogRead(moistureSensor);
digitalWrite(tempLed, LOW);
}

digitalWrite(waterPump, LOW);

//update time, and increment seconds_light if the lights are on
seconds_for_this_cycle = now() - seconds_elapsed_total;
seconds_elapsed_total = now() - start_time;
if (light_val > 900)
{
seconds_light = seconds_light + seconds_for_this_cycle;
}

//cloudy days that get sunny again: turn lights back off if light_val exceeds 900. this works b/c the supplemental lights aren't as bright as the sun:)
if (light_val > 900)
{
digitalWrite (lightSwitch, LOW);
}

//turn off lights if proportion_lit>proportion_to_light, and then wait 5 minutes
if (proportion_lit > proportion_to_light)
{
digitalWrite (lightSwitch, LOW);
delay (300000);
}

//figure out what proportion of time lights have been on
proportion_lit = seconds_light/seconds_elapsed_total;

//turn lights on if light_val is less than 900 and plants have light for less than desired proportion of time, then wait 10 seconds
if (light_val < 900 and proportion_lit < proportion_to_light)
{
digitalWrite(lightSwitch, HIGH);
delay(10000);
}

//turn on temp alarm light if temp_val is less than 850 (approximately 50 degrees Fahrenheit)
if (temp_val < 850)
{
//digitalWrite(tempLed, HIGH);
}

}
