
/******************************************************************************
 *  i2c_gpio
 *  Keith Neufeld
 *  May 26, 2008
 *
 *  Prototype I2C interface to TI 9535 and 9555 GPIO expanders.
 *
 *  Arduino analog input 5 - I2C SCL
 *  Arduino analog input 4 - I2C SDA
 *
 ******************************************************************************/

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define LCDADDR 0x20 // set the address of the I2C device the LCD is connected to

// create an lcd instance with correct constructor for how the lcd is wired to the I2C chip
LiquidCrystal_I2C lcd(LCDADDR, 4, 5, 6, 0, 1, 2, 3, 7, NEGATIVE); // addr, EN, RW, RS, D4, D5, D6, D7, Backlight, POLARITY

// Creat a set of new characters
const uint8_t charBitmap[][8] = {
   { 0xc, 0x12, 0x12, 0xc, 0, 0, 0, 0 },
   { 0x6, 0x9, 0x9, 0x6, 0, 0, 0, 0 },
   { 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0, 0x0 },
   { 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0, 0x0 },
   { 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0, 0x0 },
   { 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0, 0x0 },
   { 0x0, 0x0, 0x0, 0x6, 0x9, 0x9, 0x6, 0x0 },
   { 0x0, 0x0, 0x0, 0xc, 0x12, 0x12, 0xc, 0x0 }
   
};

void setup()
{
   
  pinMode(4, INPUT);
  digitalWrite(4, HIGH);
  pinMode(5, INPUT);
  digitalWrite(5, HIGH);
  
  int charBitmapSize = (sizeof(charBitmap ) / sizeof (charBitmap[0]));

  lcd.begin(16,2);  // initialize the lcd as 20x4 (16,2 for 16x2)
  
  lcd.setBacklight(1); // switch on the backlight

  for ( int i = 0; i < charBitmapSize; i++ )
   {
      lcd.createChar ( i, (uint8_t *)charBitmap[i] );
   }

  lcd.home ();  // go home to character 0 on line 0 (1st line)
  lcd.print("Hello, ARDUINO ");  
  lcd.setCursor(0,1);  // character 0 on line 1 (2nd line)
  lcd.print (" FORUM - fm   ");
  lcd.setCursor(0,2); // character 0 on line 2 (3rd line)
  lcd.print("This is line 3");
  lcd.setCursor(0,3); // character 0 on line 3 (4th line)
  lcd.print("This is line 3");
  delay(1000);
}

void loop()
{
   lcd.clear();
   lcd.home ();
   // Do a little animation by writing to the same location
   for ( int i = 0; i < 2; i++ )
   {
      for ( int j = 0; j < 16; j++ )
      {
         lcd.print (char(random(7)));
      }
      lcd.setCursor ( 0, 1 );
   }
   delay (200);
}
