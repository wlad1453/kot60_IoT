/**************************************************************
 * TinyGSM Getting Started guide:
 *   https://tiny.cc/tinygsm-readme
 *   
 *   ancestor of 0_0 version
 *   Migration to Mega2560 platform
 *   version 0.01 15/05/20
 *   
 *   OK
 **************************************************************/
#include <Wire.h>              // Arduino IDE
#include <LiquidCrystal_I2C.h> // https://bitbucket.org/fmalpartida/new-liquidcrystal/downloads
#include <DS3231.h>
#include <OneWire.h>            // OneWire Library                         *new*
#include <DallasTemperature.h>  // DS18B20 Library                         *new*
// #include <string.h>

// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
                                                                // 0x3F for 20x4 LCD
#define TINY_GSM_MODEM_A6
#define SerialMon Serial                // Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialAT Serial1                // Set serial for AT commands (to the module) // Use Hardware Serial on Mega, Leonardo, Micro

#define ONE_WIRE_BUS_0 10               // OneWire interface (data) is plugged into port 10
#define ONE_WIRE_BUS_1 11               // OneWire interface (data) is plugged into port 11
#define TEMPERATURE_PRECISION 9         // Set the resolution of DS18B20 sensors (was 10)


DS3231  rtc(SDA, SCL);                  // Init the DS3231 using the hardware interface
Time rtc_t;                                 // Creating the time structure 't'

uint8_t   hours = 0, minutes = 0, seconds = 0, stop_sec = 0; // t.hour, t.min
uint8_t   month = 1, day_of_month =1;
uint8_t   screen_count = 0;
uint16_t  y = 2000;
float     rtc_temp (0);
boolean   show_time = true;

OneWire oneWire_0( ONE_WIRE_BUS_0 );            // Setup a oneWire instance. Heat accumulator sensors. Further heat radiators and floor sens.
DallasTemperature sensorsHA( &oneWire_0 );      // Pass our oneWire reference to Dallas Temperature.
OneWire oneWire_1( ONE_WIRE_BUS_1 );            // Setup a oneWire instance. Rooms 1st floor sensors, further 2nd floor and outer temp.
DallasTemperature sensorsR( &oneWire_1 );       // Pass our oneWire reference to Dallas Temperature.


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

// Buffer T   Upp.  MidU  MidL  Bott. Ro.1  Ro.2  Ro.3  Ro.4, T[8] - avarage
float T[9] = {25.0, 22.0, 20.0, 18.0, 22.0, 24.0, 23.0, 19.0, 21.0};        // Some default values
float * ptrT[9] = {&T[0], &T[1], &T[2], &T[3], &T[4], &T[5], &T[6], &T[7], &T[8]} ;

// Increase RX buffer to capture the entire response
// Chips without internal buffering (A6/A7, ESP8266, M590)
// need enough space in the buffer for the entire response
// else data will be lost (and the http library will fail).

#define TINY_GSM_RX_BUFFER 650        // 650 //128 // last 670 // was 650

// #define DUMP_AT_COMMANDS           // See all AT commands, if wanted // uncomment if debugging is needed   
#define TINY_GSM_DEBUG SerialMon      // Define the serial console for debug prints, if needed

// Range to attempt to autobaud
// #define GSM_AUTOBAUD_MIN 9600      // 9600                                                                   
// #define GSM_AUTOBAUD_MAX  19200    // 38400 // 115200 // 57600 // 115200 // 19200 // 38400 

// Add a reception delay - may be needed for a fast processor at a slow baud rate
//#define TINY_GSM_YIELD() { delay(2); }

// Define how you're planning to connect to the internet
#define TINY_GSM_USE_GPRS true
#define TINY_GSM_USE_WIFI false

#define GSM_PIN ""    // set GSM PIN, if any

// GPRS credentials:
const char apn[]  =  "internet";    // "internet"; Megafone
const char gprsUser[] = "gdata";    // gdata
const char gprsPass[] = "gdata";    // gdata

// Server details
const char server[] =  "kot60.online"; // "vsh.pp.ua"; // 
const char resource[] = "/a6script.php"; // "/pics/Kot60_IoT.txt"; // "/TinyGSM/logo.txt"; //  "/test.txt"; // "?Ut=50&Um=40&Bm=30&Bt=20"; // 

#include <TinyGsmClient.h>

#ifdef DUMP_AT_COMMANDS
  #include <StreamDebugger.h>
  StreamDebugger debugger(SerialAT, SerialMon);
  TinyGsm modem(debugger);
#else
  TinyGsm modem(SerialAT);
#endif

  TinyGsmClient client(modem);  
  const int  port = 80;
  bool       success = false;

  unsigned long lastDataSent(0); 
  
  // *** Header declaration of function set ***
  void ModemA6_ON(byte pwrON);
  void showTempMask();
  void showTimeLCD(Time * tts);
  void showTempLCD(float Temp[9]);
  void showTimeSerial(Time * tts);
  String Tconv(int T);
  uint16_t eqSta(byte lngt, bool bo[], bool* boP[]);
  void syncronizeRTC(String T, String D);

void setup() {  
  ModemA6_ON(9);
  SerialMon.begin(9600);                            // last one 57600 // 115200 // Set console baud rate
  delay(10);
  
  lcd.begin(20,4);                                  // (20,4) initialize the lcd for 20 chars 4 lines and turn on backlight
  rtc.begin();                                      // initialize RTC DS3231
  // rtc_t = rtc.getTime();  
  // showTimeSerial(&rtc_t);      
  
  // setRTC(); // Set RTC to the system time values (__TIME__, __DATE__). If the FW has been downloaded into uC!!!

  SerialMon.println(F("Wait..."));

  // TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,GSM_AUTOBAUD_MAX); 
  SerialAT.begin(9600);                             // was 9600, 115200
  delay(3000);

  SerialMon.println(F("Initializing modem..."));    // Restart takes quite some time. To skip it, call init() instead of restart()
  modem.restart();   //  modem.init();
  
  String modemInfo = modem.getModemInfo();
  SerialMon.print(F("Modem Info: ")); SerialMon.println(modemInfo);

  sensorsHA.begin();          // Start up the DallasTemperature.h (DS18B20) library. Heat accumulator sensors. Further HR and HF sensors.
  sensorsR.begin();           // Start up the DallasTemperature.h (DS18B20) library. Rooms and outer temp. sensors

  screen_count = 0;
  // randomSeed(analogRead(0));
}

void loop() {

  String data  = "";    // "?Ut=50&Um=40&Bm=30&Bt=20";  
  String TankT = "";    // A string of 8 numbers, each two are HA level temperaturs
  String RoomT = "";    // Same as the previous but with rooms temp.
  String buf = "";      // Buffer for data read from the modem
  String Tstr, Dstr;    // Time and Date as a String
  
  float Ut(0), Um(0), Bm(0), Bt(0), MaccT(0);                                                   // Temperature readings in HA tank. MaccT - middle T°
  float KitT(41.1), LivT(42.2), Bed1(43.3), CorT(44.4), BathT(45.5);                            // Temperature values in the rooms
  bool bar1(0), bar2(1), bar3(0), hWat(0), hfP(1), hrP(0), hwP(1), boilP(1), camP(1);           // Bars and pumps state. Simulated values
  bool eqwArr[] = {camP, boilP, hwP, hrP, hfP, hWat, bar3, bar2, bar1};                         // Equipment status array of bool. Camin, boiler, hot water, heating radiators, heating floor
  bool *eqwP[9];                                                                                // Pointers array  
  bool showTemp = true;
  uint16_t eqw(0);
  uint16_t Tpos(0), Dpos(0); 
  
  rtc_t = rtc.getTime();                                   // Get time and date from rtc
  rtc_temp = rtc.getTemp();
  
  hours = rtc_t.hour;  minutes = rtc_t.min; seconds = rtc_t.sec;   // Current time stored in 'hours' and 'minutes'
  y = rtc_t.year; month = rtc_t.mon; day_of_month = rtc_t.date;
  
  // if ( (millis() - screenStopWatch) > 1000 ) {    // a variant of getting the next second
    // screenStopWatch = millis();    

  if(stop_sec != seconds) {                         // Next second condition
    stop_sec = seconds;  

    // *** Sensors reading section ***
    sensorsHA.requestTemperatures();
    sensorsR.requestTemperatures();
    
    for (uint8_t i = 0; i < 8; i++)   {

      if ( i < 4 )  T[i] = sensorsHA.getTempC(sens[i]);       // Sensors on the 0th line, pin 10. Currently 0 -> 3, Heat Accumulator
      else          T[i] = sensorsR.getTempC(sens[i]);        // Sensors on the 1st line, pin 11. Currently 4 -> 7, Room temperature
      
      if ( T[i] < -100 ) T[i] = 10 * ( 8 - i ) + 0.5;         // Just to simulate some meaningfull number
      SerialMon.print("T"); SerialMon.print(i); SerialMon.print(" "); SerialMon.print(T[i]); SerialMon.print("   "); 
    }
    SerialMon.println();  
    // End sensor reading

    // *** Presentaion section ***
    if ( screen_count < 4 ) showTimeLCD(&rtc_t); 
    else if ( screen_count < 8 ) { 
      if ( screen_count == 4 ) showTempMask();
      showTempLCD( T );
    }
    else {}  // Show outer T, stored kWts, etc.
    
    showTimeSerial( &rtc_t ); 
    screen_count++; 
    if ( screen_count > 11 ) screen_count = 0;
    // *** End presentation
  }

  if( (millis() - lastDataSent) > 3600000 ) {                                           // Modem restart once in 60 min.
    SerialMon.print(F(" ***** Last Data Sent  ")); SerialMon.print((millis() - lastDataSent) / 60000);    
    SerialMon.print(F("min ago ****    ***** Modem restart ****  ")); SerialMon.println(millis());
    modem.restart();
    lastDataSent = millis();
  }  
  
  // SerialMon.print(F("\n***** Last Cycle done ****  "));   SerialMon.print(millis());   
  // SerialMon.print(F(" ***** Last Data Sent ****  "));     SerialMon.println(lastDataSent);   

  if ( T[0] < 0 && T[1] < 0 && T[2] < 0 && T[3] < 0 ) { //***  Simulation (if no real measured data present)  ***
  
    Ut = 40 + random(0,40);  Um = 30 + random(0,20);  Bm = 25 + random(0,15);  Bt = 10 + random(0,15);
    // KitT  = 15 + random(0,10); LivT = 15 + random(0,10); BathT = 15 + random(0,10);  Bed1 = 15 + random(0,10);
    
  } else { // *** Real sensor reading ***
    Ut = T[0];  Um = T[1];  Bm = T[2];  Bt = T[3];    
  }
  
  KitT = T[4]; LivT = T[5]; Bed1 = T[6]; BathT = T[7];              // Real sensors reading
  CorT = 15 + random(0,10);                                         // Simulated
  
  SerialMon.println(); 
  SerialMon.print("KitT = "); SerialMon.print(KitT); SerialMon.print("  LivT = "); SerialMon.print(LivT); 
  SerialMon.print("  Bed1 = "); SerialMon.print(Bed1); SerialMon.print("  BathT = "); SerialMon.println(BathT); 
  //******  End of Simulation   *******
  
  MaccT = (Ut + Um + Bm + Bt) / 4;                                                      // Middle temp. value in the heat accumulator 
  TankT = Tconv(Ut) + Tconv(Um) + Tconv(Bm) + Tconv(Bt);                                // String variable to be sent to a6script.php, 8 numbers
  RoomT = Tconv(KitT) + Tconv(LivT) + Tconv(BathT) + Tconv(CorT) + Tconv(Bed1);         // String variable to be sent to a6script.php, 10 numbers
  
  SerialMon.print(F("\nTankTemp: ")); SerialMon.print(TankT);  SerialMon.print(F("  RoomTemp: ")); SerialMon.println(RoomT);  
  // SerialMon.println(F("\t\t\t\t**  HW status"));
  // SerialMon.println("\t\t\t\t\t**");
      
  for (int i = 0; i < 9; i++)  eqwP[i] = &eqwArr[i];                        // assign the address of bool.
  
  byte len = sizeof(eqwArr)/sizeof(eqwArr[0]);                              // Number of elements in equArr
  eqw = eqSta(len, eqwArr, eqwP);                                           // Encoded equipment status (binary)
  
  for (byte i = 0; i < len ; i++) {
    SerialMon.print(i); SerialMon.print("\t");
  }  
  SerialMon.println("**");
  for (byte i = 0; i < len ; i++) {
    SerialMon.print(eqwArr[i]); SerialMon.print("\t"); 
  }
  SerialMon.println(F("**\n"));
  
  data = "?Ut=" + String(Ut) + "&Um=" + String(Um) + "&Bm=" + String(Bm) + "&Bt=" + String(Bt);                     // Heat accumulator values
  data += "&kitT=" + String(KitT) + "&livT=" + String(LivT) + "&bedT=" + String(Bed1) + "&bathT=" + String(BathT);  // Room temp. values
  data += "&eqw=" + String(eqw);                                                                                    // Equipment status
                                                                                        // Data to be sent to a6script within GET proc.
  data = resource + data;  // "/a6script.php"                                           // Complete HTML GET-command data

                                      // ***** Here beginns the sending section *****

  if( (millis() - lastDataSent) > 1800000 || !success ) {         // Each 30 minutes, or wenn no success achieved

  lcd.clear();
  showTimeLCD( &rtc_t );
  lcd.setCursor(2,3); lcd.print("Connecting server");
   
  // SerialMon.print(F("Resource + data: ")); SerialMon.println(data);
  SerialMon.println(F("Waiting for GSM network..."));
  lcd.setCursor(2,3); lcd.print("Waiting for GSM  ");
 
  if (!modem.waitForNetwork()) {
    SerialMon.println(F(" fail GSM"));
    delay(1000); // 5000 // 10 000
    return;
  }
  SerialMon.println(F(" success GSM"));
  lcd.setCursor(2,3); lcd.print("** Success GSM **");
  

  if (modem.isNetworkConnected()) {
    SerialMon.println(F("Network connected GSM"));
    lcd.setCursor(2,3); lcd.print("Network connected");
  }

#if TINY_GSM_USE_GPRS
  // GPRS connection parameters are usually set after network registration
    SerialMon.print(F("Connecting to Megafon "));
    SerialMon.print(apn); SerialMon.print("  ");
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      SerialMon.println(F(" fail GPRS i-net"));
      delay(1000); // 10000
      return;
    }
    SerialMon.println(F(" success GPRS i-net "));
    lcd.setCursor(2,3); lcd.print("Success GPRS i-nt");
   
  if (modem.isGprsConnected()) {
    SerialMon.println(F("GPRS i-net connected"));
    lcd.setCursor(2,3); lcd.print("GPRS int connect ");
  }
#endif

  for(byte i = 0; i < 10; i++)
  {
    SerialMon.print(F("Connecting to ")); SerialMon.println(server);
    lcd.setCursor(2,3); lcd.print("Connecting to srv");
    if (!client.connect(server, port)) {
      SerialMon.println(F(" fail i-net site "));
      delay(1000); // 5000 // 10 000
      return;
    }
    SerialMon.println(F(" success i-net site "));

    // Make a HTTP GET request:
    SerialMon.println(F("Performing HTTP GET request..."));
    lcd.setCursor(2,3); lcd.print("HTTP GET request ");
    client.print(String("GET ") + data  + String(" HTTP/1.1\r\n") );  // resource + data 
    SerialMon.println(F(" send GET + data + HTTP/1.1  "));
      
    client.print(String("Host: ") + server + "\r\n");  SerialMon.println(F(" send Host: server  "));
    client.print("Connection: close\r\n\r\n");         SerialMon.println(F("Connection: closed"));
    lcd.setCursor(2,3); lcd.print("Connection closed");
          
    // client.print("Keep-Alive: 300\r\n");         // added by me
    // client.print("Connection: Keep-Alive\r\n\r\n");  // added by me
    client.println();
     
    if ( client.available() ) break;
  }

                                // ****** Tosser ******  
  uint8_t i(0), j(0);
  byte k(0), s(0); byte Utd(0);
  String d = "";
  unsigned long timeout = millis();
  while (client.connected() && millis() - timeout < 10000L) {
    // Print available data
    while (client.available()) {
      char c = client.read();
      buf += c;
      SerialMon.print(c);
      
      if ( c == '$') { j = i; k = 1; }        // SerialMon.print(F(" ---> U ")); SerialMon.println(i); 
      if ( i == j + k && c == 'e') { k++;  }  // , SerialMon.print(F(" ---> t ")); SerialMon.println(i)
      if ( i == j + k && c == 'q') { k++;  }   //  , SerialMon.print(F(" ---> _ ")); SerialMon.println(i)
      if ( i == j + k && c == 'w') { k++;  }  // , SerialMon.print(F(" ---> = ")); SerialMon.println(i)
      if ( i == j + k && c == ' ') { k++; d = ""; }   // SerialMon.print(F(" ---> _ ")); SerialMon.println(i);  
      if ( k == 5 && i > j + k + 1 && i < j + k + 5 ) {
        d += c ; eqw = d.toInt(); s++;
        if ( s == 3 ) { 
          SerialMon.println("");    
          SerialMon.print(" i: "); SerialMon.print(i);
          SerialMon.print(" $eqw: "); SerialMon.print(eqw); 
          s = 0;
          
          lastDataSent = millis();  SerialMon.print(F(" Last Data Sent  "));  SerialMon.println(lastDataSent);  
          success = true;           SerialMon.print(F(" Success:  "));        SerialMon.println(success);  
        }
      }  
      i++;
      timeout = millis();
    } // while (client.available())
  
    SerialMon.println(""); 
    SerialMon.print(F(" Data buffer  ")); SerialMon.println(buf.length());  
    SerialMon.println(buf);  
   
    Tpos = buf.indexOf("$measT");                                     // Getting Time out of recieved http text.
    Tstr = buf.substring(Tpos + 10, Tpos + 18);
    SerialMon.print(F(" Time recieved ")); SerialMon.println(Tstr);  
    
    Dpos = buf.indexOf("$measD");                                     // Getting Date out of recieved http text.
    Dstr = buf.substring(Dpos + 10, Dpos + 18);    
    SerialMon.print(F(" Date recieved ")); SerialMon.println(Dstr); 
    /*
    SerialMon.print(F(" H: ")); SerialMon.println( Tstr.substring(0, 2).toInt() );
    SerialMon.print(F(" M: ")); SerialMon.println( Tstr.substring(3, 5).toInt() );
    SerialMon.print(F(" S: ")); SerialMon.println( Tstr.substring(6, 8).toInt() );
  */
    // Here is the RTC syncronization
    /*
    //if ( Tstr.substring(3, 5).toInt() !=  rtc_t.min ) {    // - possible condition
      SerialMon.print(F(" *** RTC syncronization *** "));
      rtc.setTime( Tstr.substring(0, 2).toInt(), Tstr.substring(3, 5).toInt(), Tstr.substring(6, 8).toInt() );           // Set the time to hh:mm:ss (24hr format)
      rtc.setDate( Dstr.substring(0, 2).toInt(), Dstr.substring(3, 5).toInt(), Dstr.substring(6, 8).toInt() );           // Set the date to dd/mm/yy 
      //} */
    syncronizeRTC( Tstr, Dstr, 7 );

    // -------- Get time and date from rtc ---------
     rtc_t = rtc.getTime();                                 
  rtc_temp = rtc.getTemp();
  
  SerialMon.print("Time read from RTC: "); SerialMon.print(rtc_t.hour); SerialMon.print(":"); SerialMon.print(rtc_t.min); SerialMon.print(":"); SerialMon.println(rtc_t.sec);
  SerialMon.print("Date read from RTC: "); SerialMon.print(day_of_month = rtc_t.date); SerialMon.print("-"); SerialMon.print(rtc_t.mon); SerialMon.print("-"); SerialMon.println(rtc_t.year);
    
  }  
 
  // *****  Shutdown  *****
  client.stop();              SerialMon.println(F("Server disconnected"));
  modem.gprsDisconnect();     SerialMon.println(F("GPRS disconnected"));

  }  // Here ends the sending section


/*
  if ( success ) {
    delay (1800000);          // send data once a 30 min
    success = false;
  } else {
    delay (5000); // 10 000
  }  */
} // END LOOP() 

void ModemA6_ON(byte pwrON) {
  pinMode(pwrON, OUTPUT);
  digitalWrite(pwrON, HIGH);
  delay(2000);
  digitalWrite(pwrON,LOW);  
  }

String Tconv(int T) {                     // converts temperature value (int) into String format. 1. additional "0", 2. negative values
  if ( T < 10 && T >= 0 ) return "0" + String(T);
  else if (T < 0) return String(100 + T);
  else return String(T);  
  }

uint16_t eqSta(byte lngt, bool bo[], bool* boP[]) {               // This function encode the equipment status data into a binary number

  uint16_t eqD = 0, eqDd = 0;                                     // Equipment data as a binary number
  uint16_t dig = 1;                                               // digit weight. 1 means LSB
  for (byte i = 0; i < lngt; i++) {
    eqD += dig * bo[i];                                           // Number each bit of which corresponds with the cirtaine peace of equipment
    // Serial.print(i); Serial.print("  "); Serial.print(bo[i]); Serial.print("  "); Serial.print(dig); Serial.print("  "); Serial.println(eqD);
    dig *= 2;
  }
  eqDd = eqD;
  for (int i = 0; i < 9; i++) {
    if ( eqDd % 2 ) {
     *boP[i] = 1;                                                 // assign the value of bool to the corresponding address.
     eqDd = (eqDd - 1) / 2;
    }
    else {
     *boP[i] = 0;                                                 // assign the value.
     eqDd = eqDd / 2;
    }
  }  
   return eqD;
} // End eqSta()

void setRTC() {
  String sysTime = __TIME__, sysDate = __DATE__;      // THese data are actual only during the compilation
  SerialMon.print("_TIME_: "); SerialMon.print(__TIME__); SerialMon.print(" _DATE_"); SerialMon.println(__DATE__);
  String uCTime = "", uCDate = "";
  int h = 12, m = 0, s = 0, day = 1, month = 1, y = 2019;
  const char * monthName[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}; 

  uCTime = sysTime.substring(0, 2); h = uCTime.toInt();
  uCTime = sysTime.substring(3, 5); m = uCTime.toInt();
  uCTime = sysTime.substring(6, 8); s = uCTime.toInt();
  
  s += 7;                                               // To compensate compilation and download time
  if ( s > 59 ) { s -= 60;  m++;
    if ( m > 59 ) { m -= 60;  h++;
      if ( h > 23 )  h -= 24; 
    }
  }
  uCTime = "PLC time: " + (String)h + ":" + (String)m + ":" + (String)s + "   ";
  
  day = (sysDate.substring(4, 6)).toInt();
  y = (sysDate.substring(7, 12)).toInt();
  uCDate = sysDate.substring(0, 3);
  for( byte i = 0; i < 12; i++) {if (uCDate == monthName[i]) month = i + 1;}
  
  uCDate = (String)day + "/" + (String)month + "/" + (String)y;
  
  SerialMon.print("Sys time: "); SerialMon.print(sysTime); SerialMon.print(" "); SerialMon.println(sysDate);
  SerialMon.print(uCTime); SerialMon.println(uCDate);
  
  rtc.setTime(h, m, s);           // Set the time to 12:00:00 (24hr format)
  rtc.setDate(day, month, y);     // Set the date to January 1st, 2014 
}

void showTimeSerial(Time * tts){  // Time to show via Serial
  SerialMon.print("Time:\t"); 
  if( tts->hour < 10 ) SerialMon.print("0");   SerialMon.print(tts->hour);    SerialMon.print(":"); 
  if( tts->min < 10 )  SerialMon.print("0");   SerialMon.print(tts->min);     SerialMon.print(":"); 
  if( tts->sec < 10 )  SerialMon.print("0");   SerialMon.print(tts->sec);     SerialMon.print("\t");
  SerialMon.print(" Date:\t"); SerialMon.print(tts->year); SerialMon.print("-"); 
  SerialMon.print(tts->mon); SerialMon.print("-"); SerialMon.print(tts->date); 
  SerialMon.print("\t"); SerialMon.print(rtc.getTemp()); SerialMon.println("°C");
}

void showTimeLCD(Time * tts) {

  lcd.clear();  
  lcd.setCursor(6,1); 
  if( tts->hour < 10 ) lcd.print("0");  lcd.print(tts->hour); lcd.print(":");
  if( tts->min < 10 )  lcd.print("0");  lcd.print(tts->min);  lcd.print(":");
  if( tts->sec < 10 )  lcd.print("0");  lcd.print(tts->sec); 
  lcd.setCursor(15,1); lcd.print(rtc.getTemp()); lcd.setCursor(19,1); lcd.write(0xdf);
  lcd.setCursor(5,2); 
  if ( tts->date < 10 ) lcd.print("0"); lcd.print(tts->date); lcd.print("-"); 
  if ( tts->mon  < 10 ) lcd.print("0"); lcd.print(tts->mon); lcd.print("-"); 
    lcd.print(tts->year); 
}

void showTempMask() {
  lcd.clear();  
   //_______________________________01234567890123456789_______________________
   lcd.setCursor(0,0); lcd.print(F("A U --,-  R K  --,-")); lcd.write(0xdf);
   lcd.setCursor(0,1); lcd.print(F("c m'--,-  o L  --,-")); lcd.write(0xdf);
   lcd.setCursor(0,2); lcd.print(F("c m,--,-  o Be --,-")); lcd.write(0xdf);
   lcd.setCursor(0,3); lcd.print(F("  B --,-  m Ba --,-")); lcd.write(0xdf);
        
   lcd.setCursor(8,0); lcd.write(0xdf); // Put the °-sign at the right place   
   lcd.setCursor(8,1); lcd.write(0xdf);
   lcd.setCursor(8,2); lcd.write(0xdf);
   lcd.setCursor(8,3); lcd.write(0xdf);          
}
void showTempLCD( float Temp[9] ) {
   uint8_t cln1 = 4, cln2 = 15;

   lcd.setCursor(cln1,0); lcd.print( String(Temp[0],1) );
   lcd.setCursor(cln1,1); lcd.print( String(Temp[1],1) );
   lcd.setCursor(cln1,2); lcd.print( String(Temp[2],1) ); 
   lcd.setCursor(cln1,3); lcd.print( String(Temp[3],1) ); 

   lcd.setCursor(cln2,0); lcd.print( String(Temp[4],1) );
   lcd.setCursor(cln2,1); lcd.print( String(Temp[5],1) );
   lcd.setCursor(cln2,2); lcd.print( String(Temp[6],1) ); 
   lcd.setCursor(cln2,3); lcd.print( String(Temp[7],1) ); 
}

void syncronizeRTC( String T, String D, uint8_t corrS ) {
  uint8_t h, m, s;
  uint8_t d, maxD, month, y;
  
  SerialMon.println(F(" *** RTC syncronization procedure *** ")); SerialMon.println();
  SerialMon.print("Time str: "); SerialMon.println(T); 
  SerialMon.print("Date str: "); SerialMon.println(D); 


  
  h = T.substring(0, 2).toInt(); m = T.substring(3, 5).toInt(); s = T.substring(6, 8).toInt(); 
  d = D.substring(0, 2).toInt(); m = D.substring(3, 5).toInt(); y = D.substring(6, 8).toInt();

  SerialMon.print("Time out of str: "); SerialMon.print(h); SerialMon.print(":"); SerialMon.print(m); SerialMon.print(":"); SerialMon.println(s);
  SerialMon.print("Date out of str: "); SerialMon.print(d); SerialMon.print("-"); SerialMon.print(month); SerialMon.print("-"); SerialMon.println(y);

  if ( month == 2 ) { if ( y % 4 ) maxD = 28; else maxD = 29; }                     // February leap year condition
  else if ( month == 4 || month == 6 || month == 9 || month == 11) maxD = 30;       // Apr., Jun., Sep., Nov.,
  else maxD = 31;                                                                   // Jan., Mar., May., Jul., Aug., Oct., Dec.

  s += corrS;                           // Setting time correction
  if ( s > 59 ) { 
    s -= 60; m++; 
    if ( m > 59 ) { 
      m -= 60; h++; 
      if ( h > 23 ) { 
        h -= 24; d++; 
        if ( d > maxD ) { 
          d -= maxD; 
          month++; 
          if ( month > 12 ) { month -= 12; y++; }
        } // d
      }   // h
    }     // m
  }       // s
  
  rtc.setTime( h, m, s );           // Set the time to hh:mm:ss (24hr format)
  rtc.setDate( d, month, y );       // Set the date to dd/mm/yy   
  SerialMon.print("Time synchronized: "); SerialMon.print(h); SerialMon.print(":"); SerialMon.print(m); SerialMon.print(":"); SerialMon.println(s);
  SerialMon.print("Date synchronized: "); SerialMon.print(d); SerialMon.print("-"); SerialMon.print(month); SerialMon.print("-"); SerialMon.println(y);
}
