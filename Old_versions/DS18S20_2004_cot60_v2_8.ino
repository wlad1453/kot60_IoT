/*  LCD I2C initialization and T measurements simulation, Version 1.3 03.02.16 
 *  Backpack Interface labelled "LCM1602 IIC  A0 A1 A2"
 *  RTC DS3231 connected over IIC interface
 *  Heating energy accumulation / consumption simulated
 *  3 displays screen, switching consequentedly
 *  
 *  Next steps:
 *  1. Energy balans calculation. Energy losses - middle tank temp. difference through time unit. minus heating energy
 *  2. T1-T8 -> T[1]-T[8]
 *  3. RelayX - > Realy[X]
 *  
 *  All done with register operations
 */
#include <Wire.h>              // Arduino IDE
#include <LiquidCrystal_I2C.h> // https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
#include <DS3231.h>
#include <OneWire.h>            // OneWire Library                         *new*
#include <DallasTemperature.h>  // DS18B20 Library                         *new*

/*-----( Declare objects )-----*/
DS3231  rtc(SDA, SCL);        // Init the DS3231 using the hardware interface

// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
                                                                // 0x3F for 20x4 LCD
                                                                
//   Relay num   0  1  2  3
const byte Relay[4] = {3, 4, 5, 6}; // Control pin number
#define relay1 3              // Wood boiler heater 1
#define relay2 4              // Wood boiler heater 2
#define relay3 5              // Wood boiler heater 3
#define relay4 6              // Water boiler heater,"Day" heater
#define BUZZER 7              // Buzzer connected to the pin D7

#define ONE_WIRE_BUS 9           // OneWire interface (data) is plugged into port 9
#define TEMPERATURE_PRECISION 10 // Set the resolution of DS18B20 sensors

// Keyboard
// lines(INPUT_PULLUP): A0,A1,A2,A3; columns(OUTPUT): B10,B11,B12

OneWire oneWire(ONE_WIRE_BUS);        // Setup a oneWire instance to communicate with any OneWire devices
DallasTemperature sensors(&oneWire);  // Pass our oneWire reference to Dallas Temperature.

//  ----  Sensor addresses --------------
DeviceAddress Sensor0 = { 0x28, 0xFF, 0xB1, 0x97, 0xB2, 0x15, 0x03, 0xAF }; // Sensor 1 upper
DeviceAddress Sensor1 = { 0x28, 0xFF, 0x19, 0x73, 0xB2, 0x15, 0x03, 0xE5 }; // Sensor 2 middle upper
DeviceAddress Sensor2 = { 0x28, 0xFF, 0x56, 0x8F, 0xB2, 0x15, 0x01, 0xBC }; // Sensor 3 middle low
DeviceAddress Sensor3 = { 0x28, 0xFF, 0x97, 0x05, 0xB2, 0x15, 0x01, 0x91 }; // Sensor 4 bottom

DeviceAddress Sensor4 = { 0x28, 0xFF, 0xD4, 0x50, 0xC0, 0x15, 0x01, 0xCF }; // Living room temp.
DeviceAddress Sensor5 = { 0x28, 0xFF, 0x25, 0x50, 0xC0, 0x15, 0x01, 0xA4 }; // Kitchen temp.
DeviceAddress Sensor6 = { 0x28, 0xFF, 0xCD, 0x6E, 0xC2, 0x15, 0x02, 0xB0 }; // Guest room temp.
DeviceAddress Sensor7 = { 0x28, 0xFF, 0x67, 0x53, 0xC0, 0x15, 0x01, 0x46 }; // Bathroom temp.

uint8_t * sens[] = { Sensor0, Sensor1, Sensor2, Sensor3, Sensor4, Sensor5, Sensor6, Sensor7 };
  
unsigned long previousTime = 0, nowSim = 10000; // Value of the fixed time point
byte screenN = 0;               // Screen number for the switching between screens
int period;                     // Time in millisec. for each screen is schowing      
int tsim = 0;                   // Variable changing the temperature values

// ------------  Data needed for the heating energy calculation  ----------------------
const int   heatBufferVolume = 980;   // Liters, kG
const float cWater = 4.184;           // thermal capacity of water by 60°C kJ/(kG*K)
const float tRef = 20;                // minimum water temperature °C
const float heatLosses = 4.0;         // heat energy losses (kWt/h)
float heatEnergy = 0;           // heat energy in kWt/h stored in the buffer tank
float heatingTime = 0.0;        // approximate time to the next heating in hrs.
uint8_t  heaterN(2);               // number of the night heater allowed
boolean relayOn = false, dayHeaterOn = false, day(false); // Show state of the PLC
boolean nightAlwd(true), dayAlwd(true);
uint8_t nightThours(23), nightTmin(0), dayThours(7), dayTmin(0);
float maxNightT = 90.0, minNightT = 60.0, minDayT = 20.0, maxDayT = 60.0;

// Buffer T   Upp.  MidU  MidL  Bott. Ro.1  Ro.2  Ro.3  Ro.4, T[8] - avarage
float T[9] = {25.0, 22.0, 20.0, 18.0, 22.0, 24.0, 23.0, 19.0};
float * ptrT[9] = {&T[0], &T[1], &T[2], &T[3], &T[4], &T[5], &T[6], &T[7], &T[8]} ;

// ptrT = &T;
                                                          
Time t;                         // Creating the time structure 't'
uint8_t hours = 0, minutes = 0; // t.hour, t.min

// ----------- Set up menu data: ---------------------
// Manual mode, heater element can be switched On/Off manually
// Heater switching allowed at night / at day separately
// How many night heaters are allowed (1, 2, (3))
// Night/day time settings nightTime = 23:00, dayTime = 07:00
// maxNightT = 90°C, minNightT = 60°C, minDayT = 20°C, maxDayT = 60°C
// All set date should be writen in EEPROM


void setup()   /*----( SETUP: RUNS ONCE )----*/
  {
  Serial.begin(9600);    // Used to communicate with PC // while (!Serial); - needed for Leonardo

  lcd.begin(20,4);          // (20,4) initialize the lcd for 20 chars 4 lines and turn on backlight
  rtc.begin();              // initialize RTC DS3231
       
  // The following line can be uncommented to set the date and time
  // setRTC();

  sensors.begin();          // Start up the DallasTemperature.h (DS18B20) library
  sensors.requestTemperatures();

 // relay1 3, relay2 4, relay3 5, relay4 6, buzzer 7th-bit
  DDRD  |= 0b11111000;  // DDRD is the direction register for Port D (Arduino digital pins 0-7), "1" means output
  PORTD |= 0b01111000;  // All relays are set to HIGH (switched off), the byzzer - to LOW (off)
  
  LCDintro();  // Introduction on LCD, plays once
 
  // relayTest(); // Optional
   
  
}  /*--( END SETUP )---*/

void loop()   /*----( LOOP: RUNS CONSTANTLY )----*/
{
  unsigned long now = millis();                           // internal time - for the switching between screens  
  t = rtc.getTime(); 
  hours = t.hour;  minutes = t.min;   // Current time stored in 'hours' and 'minutes'

  serDebug();
 
  sensors.requestTemperatures();
  for (uint8_t i = 0; i < 8; i++)   T[i] = sensors.getTempC(sens[i]);
 
  tempSimulation (ptrT); // exchange measured values through simulated ones
  
  // hours = (uint8_t)((millis() - now)/10000)
  /* if ( (millis()- nowSim) > 10000 ) {
    hours = hours + 1;
    if(hours > 24) hours = 0;
    nowSim =  millis();
    }*/

    
  // ----------------- Business logic ------------------
  
  if ((hours >= 23) || (hours < 7)) day = false; else day = true;  // Day time or night?
  
  // Night mode
  if (!day && !relayOn && !dayHeaterOn) {  // If at night both the relay and the day heater are switched OFF
    nightHeating(1);
    relayOn = true;
  }
  else if (day && relayOn) {               // If we are still heating with night heater but it became day time
    nightHeating(0);
    relayOn = false;
    delay(2000);
  }

  // Day mode
  if (day && (T[0] < 20.0) && !dayHeaterOn && !relayOn) { // If there is day time and the upper temp falls under 15°C
    dayHeating(1);
    dayHeaterOn = true;
  } 
  else if (dayHeaterOn && (!day || (T[0] > 30.0)) ) { // If at night OR the UTemp has risen over 30°C
    dayHeating(0);
    dayHeaterOn = false;
  }

  // Alarm conditions - HeatAlarm:    UpperT > 95°C, 
  //                    LowTempAlarm: BottomT < 10°C, any of roomT < 8°C
  //                    FireAlarm:    any of roomT > 40°C
    
  if(now < previousTime) previousTime = now;  // overflow condition
  if((now - previousTime) >= period)          // switching between screens if
                                              // 'period'-milliseconds ran out
  { screenN++; if (screenN == 3) screenN = 0; // Screen counter setting
    previousTime = now; 
    screenPause(200);                            // a pause between screens
    
    switch (screenN) {                        // one-time procedure on the time switching condition
      case 0:
        period = 5000; 
        outputForm();                         // prints numbers 1-8 and (°)sign on LCD
        break;
      case 1:
        period = 2000;    
        break;
      case 2:
        period = 1000;    
        lcd.setCursor(4,1); lcd.print("next heating");    
        lcd.setCursor(4,2); lcd.print("in      hrs");    
        break;  
      default: // if nothing else matches, do the default (optional)
      break;
    } // End of switch (screenN) one-time 
  }   // End of time switch (now - previousTime) >= period

  switch(screenN) { // cyclic procedure showing data on the lcd screen
    case 0:
      showTempLCD(ptrT); // prints temperature value (1-8) on LCD
      break;
    case 1:
      lcd.setCursor(4,0); lcd.print(rtc.getDOWStr());     // Send Day-of-Week as a string
      lcd.setCursor(2,1); lcd.print(rtc.getDateStr());    // Send date as dd.mm.yyyy
      lcd.setCursor(3,2); lcd.print(rtc.getTimeStr());    // Send time string as hh:mm:ss
      lcd.setCursor(4,3); lcd.print(rtc.getTemp(),1);     // Send inside temperature
      lcd.write(0xdf);    lcd.print("C");
      lcd.setCursor(15,0); lcd.print("R1 "); if(relayOn)lcd.write(0xff); else lcd.write(0xa5);    // lcd.print("."); 
      lcd.setCursor(15,1); lcd.print("R2 "); if(relayOn)lcd.write(0xff); else lcd.write(0xa5);    // lcd.print(".");  
      lcd.setCursor(15,2); lcd.print("R3 "); lcd.write(0xa5);                                     // lcd.print("."); 
      lcd.setCursor(15,3); lcd.print("dH "); if(dayHeaterOn)lcd.write(0xff); else lcd.write(0xa5); //lcd.print("."); ; 
    
      break;
    case 2:
      heatEnergy = heatBufferVolume * cWater * (T[8] - tRef); // stored heat energy (kJ/h)
      heatingTime = heatEnergy/3600/heatLosses;             // time in hrs. to the next heating,
                                                            // assumed that the heat losses are 4 kWt/h
      lcd.setCursor(5,0); lcd.print(heatEnergy/3600,1);     // calculate and show the stored heat energy
      lcd.print(" kWt/h");
      lcd.setCursor(7,2);
      if (heatEnergy >= 0.0)
        if (heatingTime < 10.0) {
          lcd.print(" ");
          lcd.print(heatingTime,1); 
        }
        else lcd.print(heatingTime,1); 
      else {
        lcd.setCursor(4,2); lcd.print("immediately !!!");
      }
      break;
    default:
      break;
  } // End of switch (screenN) cyclic
  
}  /* --(end main loop )-- */

void screenPause(int duration) {
    lcd.clear();         lcd.noDisplay();
    delay(duration);     lcd.display();    }

void outputForm() // for 20x4 lcd display
    {
      lcd.clear();
      lcd.setCursor(0,0); lcd.print("TA: U --,-  lM --,-"); lcd.write(0xdf);
      lcd.setCursor(0,1); lcd.print("   uM --,-   B --,-"); lcd.write(0xdf);
      lcd.setCursor(0,2); lcd.print("Gnd L --,-   G --,-"); lcd.write(0xdf);
      lcd.setCursor(0,3); lcd.print("Fl. K --,-   B --,-"); lcd.write(0xdf);
      
      lcd.setCursor(10,0); lcd.write(0xdf); // Put the °-sign at the right place   
      lcd.setCursor(10,1); lcd.write(0xdf);
      lcd.setCursor(10,2); lcd.write(0xdf);
      lcd.setCursor(10,3); lcd.write(0xdf);     
    }
    
void showTempLCD (float * Temp[9]) 
    { 
      lcd.setCursor(6,0);  printTemp(*Temp[0]);  // Buffer tank upper sensor
      lcd.setCursor(6,1);  printTemp(*Temp[1]);  // middle upper sensor
      lcd.setCursor(15,0); printTemp(*Temp[2]);  // middle lower sensor
      lcd.setCursor(15,1); printTemp(*Temp[3]);  // bottom sensor
      lcd.setCursor(6,2);  printTemp(*Temp[4]);  // Living room temp.
      lcd.setCursor(6,3);  printTemp(*Temp[5]);  // Kitchen temp.
      lcd.setCursor(15,2); printTemp(*Temp[6]);  // Guest room temp.
      lcd.setCursor(15,3); printTemp(*Temp[7]);  // Bathroom temp.
/*
      delay (3000); 
      
      int k = 0, i = 15, j = 1;
      for ( k = 0; k < 8; k++)
      {      
      if (j == 1) j = 0; else j = 1; if (k > 3) j += 2;
      if ((k % 2) == 0) {if (i == 15) i = 6; else i = 15;}
      lcd.setCursor(i,j); lcd.print(*Temp[k],1);  
      if (k > 3) j -= 2;
      }*/
    }
    
void printTemp(float Temp) {
  if (Temp != -127.0) {
    if (Temp < 10.0) lcd.print(" ");
    lcd.print(Temp,1);
  }
  else lcd.print(" n.a");
}
    
void buzzerPeep (byte buzzerPin, int duration)
    {digitalWrite(buzzerPin, HIGH);   delay(duration); digitalWrite(buzzerPin, LOW);}  
    
void tempSimulation (float *Ts[9]) // tsim - global variable
    {
    if (tsim < 350) *Ts[0]= 15.0 + 0.24*tsim; // + float(random(0,200))/200;        // Tank upper temp. rising
    else            *Ts[0]= 99.0 - 0.24*(tsim-349); // + float(random(0,200))/200;  // Tank upper temp. falling
    
    *Ts[8] = *Ts[0];                                                    // Initial sum
    
    for (byte i = 1; i < 4; i++) {                                      // Buffer tank temperature variation
      *Ts[i] = *Ts[i-1] * 0.75; // + float(random(0,100))/200; 
      *Ts[8] += *Ts[i]; }
    *Ts[8]= *Ts[8] / 4;                                            // Average temperature in the buffer tank     
    
    // for (byte i = 4; i < 8; i++) *Ts[i]= *Ts[3] * 0.75 + float(random(0,200))/100; // Air temperature in the rooms
   
 /* if ((Tm < 20.0) || (T1 > 95.0)) // alarm signal if Tm is lower as 25° or T1 exceeds 95°
    { lcd.noBacklight();
      digitalWrite(BUZZER, HIGH);   delay(300);
      lcd.backlight();
      digitalWrite(BUZZER, LOW);    delay(150);
      digitalWrite(BUZZER, HIGH);   delay(150);
      digitalWrite(BUZZER, LOW);    delay(50);
      } */
  
    tsim++; delay(500);
    if (tsim == 700) tsim = 0;
} // ------- End of T-measurement simulation --------

void relayTest () {     // Relay switch ON (LOW) / OFF (HIGH)
 
    buzzerPeep (BUZZER, 100);
    digitalWrite (relay1, LOW); delay(500); digitalWrite (relay1, HIGH);

    buzzerPeep (BUZZER, 100);
    digitalWrite (relay2, LOW); delay(500); digitalWrite (relay2, HIGH);

    buzzerPeep (BUZZER, 100);
    digitalWrite (relay3, LOW); delay(500); digitalWrite (relay3, HIGH);
 
    buzzerPeep (BUZZER, 100);
    digitalWrite (relay4, LOW); delay(500); digitalWrite (relay4, HIGH);
    
} // --------End relaytest -------------

void nightHeating (boolean on) {     // Relay switch ON (LOW) / OFF (HIGH)
  uint8_t wahl = (int) random(0, 300)/100;
  if (on) {
    //if (wahl < 1) {;}                       // relay1, relay2
    //else if ((wahl >= 1) && (wahl < 2)) {;} // relay2, relay3
    //else if ((wahl >= 2) && (wahl <= 3)) {;};   // relay1, relay3
    
    buzzerPeep (BUZZER, 100);
    PORTD &= ~(1 << relay1); delay(1000); // digitalWrite (relay1, LOW); 
    buzzerPeep (BUZZER, 100);
    PORTD &= ~(1 << relay2); delay(1000); // digitalWrite (relay2, LOW); 
    buzzerPeep (BUZZER, 100);
    // digitalWrite (relay3, LOW); delay(500);
  }
  else {
    buzzerPeep (BUZZER, 100);
    PORTD |= (1 << relay1); delay(500); // digitalWrite (relay1, HIGH);
    buzzerPeep (BUZZER, 100);
    PORTD |= (1 << relay2); delay(500); // digitalWrite (relay2, HIGH);
    buzzerPeep (BUZZER, 100);
    PORTD |= (1 << relay3);             // digitalWrite (relay3, HIGH);
  }
 } // --------End nightHeating -------------

 void dayHeating (boolean on) {     // Water boiler relay switch ON (LOW) / OFF (HIGH)
  if (on) {
    buzzerPeep (BUZZER, 100);
    PORTD &= ~(1 << relay4); delay(2000); // digitalWrite (relay4, LOW); 
    buzzerPeep (BUZZER, 100);    
  }
  else {
    buzzerPeep (BUZZER, 100);
    PORTD |= (1 << relay4); delay(2000);  // digitalWrite (relay4, HIGH); 
    buzzerPeep (BUZZER, 100);   
  }
 } // --------End dayHeating -------------

 void serDebug(){
  Serial.println(F("\t\t0\t1\t2\t3\t4\t5\t6\t7\t8\tPORTD\trel1\trel2\trel3\trel4\tbuzz"));
  Serial.print(rtc.getTimeStr()); Serial.print(" ");// Serial.print("\t");
  
  Serial.print( t.hour ); Serial.print("\t");
  // Serial.print( hours ); Serial.print("\t");
  // Serial.print( minutes ); Serial.print("\t");
  for (int i = 0; i < 9; i++) {Serial.print(T[i], 1); Serial.print("\t");} 
  Serial.print(PIND, BIN); 
  Serial.print("\t");  Serial.print(!!(PORTD & (1 << relay1)) );  // Serial.print(digitalRead(3));
  Serial.print("\t");  Serial.print(!!(PORTD & (1 << relay2)) );  // Serial.print(digitalRead(4));
  Serial.print("\t");  Serial.print(!!(PORTD & (1 << relay3)) );  // Serial.print(digitalRead(5));
  Serial.print("\t");  Serial.print(!!(PORTD & (1 << relay4)) );  // Serial.print(digitalRead(6));
  Serial.print("\t");  Serial.print(!!(PORTD & (1 << BUZZER)) );  // Serial.print(digitalRead(7));
  // Serial.print("    ");  Serial.print(__TIME__); // 10:45:38    Feb 19 2019
  // Serial.print("    ");  Serial.print(__DATE__); // 10:45:38    Feb 19 2019
  Serial.println(); 
 }
 
 void LCDintro() {
   lcd.setCursor(0,0); lcd.print(F("Heat accum. 980 l."));
   lcd.setCursor(0,1); lcd.print(F("Temperature Sensors:"));
   lcd.setCursor(0,2); lcd.print(F("8 DS18S20 I2C"));
   lcd.setCursor(0,3); lcd.print(F("4 relays")); delay(2000);

  }

 void setRTC() {
  String sysTime = __TIME__, sysDate = __DATE__;
  String uCTime = "", uCDate = "";
  int h = 12, m = 0, s = 0, day = 1, month = 1, y = 2019;
  const char * monthName[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}; 

  uCTime = sysTime.substring(0, 2); h = uCTime.toInt();
  uCTime = sysTime.substring(3, 5); m = uCTime.toInt();
  uCTime = sysTime.substring(6, 8); s = uCTime.toInt();
  uCTime = "PLC time: " + (String)h + ":" + (String)m + ":" + (String)s + "   ";
  
  day = (sysDate.substring(4, 6)).toInt();
  y = (sysDate.substring(7, 12)).toInt();
  uCDate = sysDate.substring(0, 3);
  for( byte i = 0; i < 12; i++) {if (uCDate == monthName[i]) month = i + 1;}
  
  uCDate = (String)day + "/" + (String)month + "/" + (String)y;
  
  Serial.print("Sys time: "); Serial.print(sysTime); Serial.print(" "); Serial.println(sysDate);
  Serial.print(uCTime); Serial.println(uCDate);
  
  rtc.setTime(h, m, s);     // Set the time to 12:00:00 (24hr format)
  rtc.setDate(day, month, y);   // Set the date to January 1st, 2014 
  }
 
/* ( THE END ) */
