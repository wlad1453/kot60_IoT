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

// Set the pins on the I2C chip used for LCD connections:
//                    addr, en,rw,rs,d4,d5,d6,d7,bl,blpol
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address
                                                                // 0x3F for 20x4 LCD
#define TINY_GSM_MODEM_A6
#define SerialMon Serial                // Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialAT Serial1                // Set serial for AT commands (to the module) // Use Hardware Serial on Mega, Leonardo, Micro


#define ONE_WIRE_BUS_0 10               // OneWire interface (data) is plugged into port 10
#define ONE_WIRE_BUS_0 11               // OneWire interface (data) is plugged into port 11
#define TEMPERATURE_PRECISION 10        // Set the resolution of DS18B20 sensors


DS3231  rtc(SDA, SCL);                  // Init the DS3231 using the hardware interface
Time t;                                 // Creating the time structure 't'

uint8_t   hours = 0, minutes = 0, seconds = 0, stop_sec = 0; // t.hour, t.min
uint8_t   month = 1, day_of_month =1;
uint16_t  y = 2000, struct_size;
float     rtc_temp (0);
boolean   show_time = true;
bool      success = false;


OneWire oneWire(ONE_WIRE_BUS_0);        // Setup a oneWire instance to communicate with any OneWire devices
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

// Buffer T   Upp.  MidU  MidL  Bott. Ro.1  Ro.2  Ro.3  Ro.4, T[8] - avarage
float T[9] = {25.0, 22.0, 20.0, 18.0, 22.0, 24.0, 23.0, 19.0, 21.0};
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

  unsigned long lastDataSent(0), sessionBegin(0);
  float tstT(21.45);

void setup() {  
  ModemA6_ON(9);
  SerialMon.begin(9600);                            // last one 57600 // 115200 // Set console baud rate
  delay(10);
  
  lcd.begin(20,4);          // (20,4) initialize the lcd for 20 chars 4 lines and turn on backlight
  rtc.begin();                                      // initialize RTC DS3231
  t = rtc.getTime();  
  showTimeSerial(&t);      
  // setRTC(); // Set RTC to the system time values (__TIME__, __DATE__). If the FW has been downloaded into uC!!!
 
  // Set your reset, enable, power pins here

  SerialMon.println(F("Wait..."));

  // TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,GSM_AUTOBAUD_MAX); 
  SerialAT.begin(9600);                             // was 9600, 115200
  delay(3000);

  SerialMon.println(F("Initializing modem...")); // Restart takes quite some time. To skip it, call init() instead of restart()
  modem.restart();   //  modem.init();
  
  String modemInfo = modem.getModemInfo();
  SerialMon.print(F("Modem Info: ")); SerialMon.println(modemInfo);

  sensors.begin();          // Start up the DallasTemperature.h (DS18B20) library
  sensors.requestTemperatures();

  
  sessionBegin = millis();
  randomSeed(analogRead(0));
}

void loop() {

  String data  = ""; // "?Ut=50&Um=40&Bm=30&Bt=20";  
  String TankT = "";
  String RoomT = "";
  String buf = "";  // Buffer for data read from the modem
  String Tstr, Dstr;
  
  uint8_t Ut(0), Um(0), Bm(0), Bt(0), MaccT(0);                                                 // Temperature readings in tank
  float KitT(41.1), LivT(42.2), Bed1(43.3), CorT(44.4), BathT(45.5);                            // Temperature values in the rooms
  float TstT(15.2);                                                                             // Just for testing of float
  bool bar1(0), bar2(1), bar3(0), hWat(0), hfP(1), hrP(0), hwP(1), boilP(1), camP(1);           // State of bars and pumps. Values just for simulation
  bool eqwArr[] = {camP, boilP, hwP, hrP, hfP, hWat, bar3, bar2, bar1};                         // Equipment status array of bool. Camin, boiler, hot water, heating radiators, heating floor
  bool *eqwP[9];                                                                                // Pointers array  
  uint16_t eqw(0);
  uint16_t Tpos(0), Dpos(0);


  SerialMon.println("*******  Sensor readings *******");  
  sensors.requestTemperatures();
  for (uint8_t i = 0; i < 8; i++)   {
    T[i] = sensors.getTempC(sens[i]);
    // dailySMS += (String)T[i] + "  ";
    SerialMon.print("T"); SerialMon.print(i); SerialMon.print(" "); SerialMon.print(T[i]); SerialMon.print("   "); 
  }
  SerialMon.println();  
  
  t = rtc.getTime();                                   // Get time and date from rtc
  rtc_temp = rtc.getTemp();
  
  hours = t.hour;  minutes = t.min; seconds = t.sec;   // Current time stored in 'hours' and 'minutes'
  y = t.year; month = t.mon; day_of_month = t.date;
  
  if(stop_sec != seconds) {
  /*
    lcd.clear();     lcd.noDisplay();
    delay(1000);     lcd.display();  */
    lcd.clear();

    lcd.setCursor(6,1); 
    if( t.hour < 10 ) lcd.print("0");  lcd.print(t.hour); lcd.print(":");
    if( t.min < 10 )  lcd.print("0");  lcd.print(t.min);  lcd.print(":");
    if( t.sec < 10 )  lcd.print("0");  lcd.print(t.sec); 
    lcd.setCursor(5,2); 
    lcd.print(t.date); lcd.print("-"); lcd.print(t.mon); lcd.print("-"); lcd.print(t.year); 
   
     stop_sec = seconds;
     showTimeSerial(&t);
  }

  if( (millis() - lastDataSent) > 3600000 ) {                                           // Modem restart once in 60 min.
    SerialMon.print(F(" ***** Last Data Sent  ")); SerialMon.print((millis() - lastDataSent) / 60000);    
    SerialMon.print(F("min ago ****    ***** Modem restart ****  ")); SerialMon.println(millis());
    modem.restart();
    lastDataSent = millis();
    sessionBegin = millis(); 
  }  
  
  SerialMon.print(F("\n***** Last Cycle done ****  ")); SerialMon.print(millis());   
  SerialMon.print(F(" ***** Last Data Sent ****  ")); SerialMon.println(lastDataSent);   

  /******  Simulation (here should be the real measurement process including equipment status)   *******/
  Ut = 40 + random(0,40);  Um = 30 + random(0,20);  Bm = 25 + random(0,15);  Bt = 10 + random(0,15);
  
  CorT = 15 + random(0,10);  
  // KitT  = 15 + random(0,10); LivT = 15 + random(0,10); BathT = 15 + random(0,10);  Bed1 = 15 + random(0,10);
  KitT = T[4]; LivT = T[5]; Bed1 = T[6]; BathT = T[7];
  tstT = 15.0 + random(100) / 10.0;
  // SerialMon.print("tstT = "); SerialMon.println(tstT); 
  SerialMon.print("KitT = "); SerialMon.print(KitT); SerialMon.print("  LivT = "); SerialMon.print(LivT); 
  SerialMon.print("  Bed1 = "); SerialMon.print(Bed1); SerialMon.print("  BathT = "); SerialMon.println(BathT); 
  /******  End of Simulation   *******/
  
  MaccT = (Ut + Um + Bm + Bt) / 4;                                                      // Middle temp. value in the heat accumulator 
  TankT = Tconv(Ut) + Tconv(Um) + Tconv(Bm) + Tconv(Bt);                                // String variable to be sent to a6script.php
  RoomT = Tconv(KitT) + Tconv(LivT) + Tconv(BathT) + Tconv(CorT) + Tconv(Bed1);         // String variable to be sent to a6script.php
  
  SerialMon.print(F("\nTankTemp: ")); SerialMon.print(TankT);  SerialMon.print(F("  RoomTemp: ")); SerialMon.print(RoomT);  
  SerialMon.println(F("\t\t\t\t**  HW status"));
  SerialMon.print(F("Minuts from the session begin:  ")); SerialMon.print( (millis() - sessionBegin) / 60000 ); 
  SerialMon.println("\t\t\t\t\t**");
    
  
  for (int i = 0; i < 9; i++)  eqwP[i] = &eqwArr[i];                        // assign the address of bool.
  
  byte len = sizeof(eqwArr)/sizeof(eqwArr[0]);                              // Number of elements in equArr
  eqw = eqSta(len, eqwArr, eqwP);                                           // Encoded equipment status (binary)

  /*
  for (byte i = 0; i < len ; i++) {
    SerialMon.print(i); SerialMon.print("\t");
  }  
  SerialMon.println("**");
  for (byte i = 0; i < len ; i++) {
    SerialMon.print(eqwArr[i]); SerialMon.print("\t"); 
  }
  SerialMon.println(F("**\n"));*/ 
  
  data = "?Ut=" + String(Ut) + "&Um=" + String(Um) + "&Bm=" + String(Bm) + "&Bt=" + String(Bt);
  data += "&kitT=" + String(KitT) + "&livT=" + String(LivT) + "&bedT=" + String(Bed1) + "&bathT=" + String(BathT);
  data += "&eqw=" + String(eqw);
                                                                                        // Data to be sent to a6script within GET proc.
  data = resource + data;  // "/a6script.php"                                           // Complete HTML GET command data

                                      // ***** Here beginns the sending section *****

  if( (millis() - lastDataSent) > 1800000 || !success ) {

  lcd.setCursor(2,3); lcd.print("Connecting server");
   
  SerialMon.print(F("Resource + data: ")); SerialMon.println(data);
  SerialMon.println(F("Waiting for GSM network..."));
  if (!modem.waitForNetwork()) {
    SerialMon.println(F(" fail GSM"));
    delay(1000); // 5000 // 10 000
    return;
  }
  SerialMon.println(F(" success GSM"));

  if (modem.isNetworkConnected()) {
    SerialMon.println(F("Network connected GSM"));
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

  if (modem.isGprsConnected()) {
    SerialMon.println(F("GPRS i-net connected"));
  }
#endif

   for(byte i = 0; i < 10; i++)
   {
    SerialMon.print(F("Connecting to "));
    SerialMon.println(server);
    if (!client.connect(server, port)) {
      SerialMon.println(F(" fail i-net site "));
      delay(1000); // 5000 // 10 000
      return;
    }
    SerialMon.println(F(" success i-net site "));

      // Make a HTTP GET request:
      SerialMon.println(F("Performing HTTP GET request..."));
      client.print(String("GET ") + data  + String(" HTTP/1.1\r\n") );  // resource   
      SerialMon.println(F(" send GET + data + HTTP/1.1  "));
      
      client.print(String("Host: ") + server + "\r\n");
      SerialMon.println(F(" send Host: server  "));
      client.print("Connection: close\r\n\r\n");                   // 15.05.20
      SerialMon.println(F("Connection: close"));
      // client.print("Keep-Alive: 300\r\n");         // added by me
      // client.print("Connection: Keep-Alive\r\n\r\n");  // added by me
      client.println();
     
     if (client.available()) break;
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
    }
    /*
    SerialMon.println(""); 
    SerialMon.print(F(" Data buffer  ")); SerialMon.println(buf.length());  
    SerialMon.println(buf);  */
   
    Tpos = buf.indexOf("$measT");
    Tstr = buf.substring(Tpos + 10, Tpos + 18);
    SerialMon.print(F(" Time ")); SerialMon.println(Tstr);  
    
    Dpos = buf.indexOf("$measD");
    Dstr = buf.substring(Dpos + 10, Dpos + 18);    
    SerialMon.print(F(" Date ")); SerialMon.println(Dstr); 
    
  }  
 
  // *****  Shutdown  *****
  client.stop();            SerialMon.println(F("Server disconnected"));
  modem.gprsDisconnect();   SerialMon.println(F("GPRS disconnected"));

  } // Here ends the sending section

  
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
  s += 7;                                               // To compensate compilation time and download into the uC
  if ( s > 59 ) { s -= 60;  m++;
    if ( m > 59 ) { m -= 60;  h++;
      if ( h > 23 )  h -= 24; }
  }
  uCTime = "PLC time: " + (String)h + ":" + (String)m + ":" + (String)s + "   ";
  
  day = (sysDate.substring(4, 6)).toInt();
  y = (sysDate.substring(7, 12)).toInt();
  uCDate = sysDate.substring(0, 3);
  for( byte i = 0; i < 12; i++) {if (uCDate == monthName[i]) month = i + 1;}
  
  uCDate = (String)day + "/" + (String)month + "/" + (String)y;
  
  SerialMon.print("Sys time: "); SerialMon.print(sysTime); SerialMon.print(" "); SerialMon.println(sysDate);
  SerialMon.print(uCTime); SerialMon.println(uCDate);
  
  rtc.setTime(h, m, s);     // Set the time to 12:00:00 (24hr format)
  rtc.setDate(day, month, y);   // Set the date to January 1st, 2014 
}

void showTimeSerial(Time * tts){ // Time to show via Serial
  SerialMon.print("Time:\t"); 
  if( tts->hour < 10 ) SerialMon.print("0");   SerialMon.print(tts->hour);    SerialMon.print(":"); 
  if( tts->min < 10 )  SerialMon.print("0");   SerialMon.print(tts->min);     SerialMon.print(":"); 
  if( tts->sec < 10 )  SerialMon.print("0");   SerialMon.print(tts->sec);     SerialMon.print("\t");
  SerialMon.print(tts->year); SerialMon.print("-"); SerialMon.print(tts->mon); SerialMon.print("-"); SerialMon.print(tts->date); 
  SerialMon.print("\t"); SerialMon.print(rtc.getTemp()); SerialMon.println("°C");
}

void showTimeLCD(Time * tts) {

  lcd.clear();  
  lcd.setCursor(6,1); 
  if( tts->hour < 10 ) lcd.print("0");  lcd.print(tts->hour); lcd.print(":");
  if( tts->min < 10 )  lcd.print("0");  lcd.print(tts->min);  lcd.print(":");
  if( tts->sec < 10 )  lcd.print("0");  lcd.print(tts->sec); 
  lcd.setCursor(5,2); 
  lcd.print(tts->date); lcd.print("-"); lcd.print(tts->mon); lcd.print("-"); lcd.print(tts->year); 
/*
   lcd.setCursor(0,0); lcd.print(F("TA: U --,-  lM --,-")); lcd.write(0xdf);
   lcd.setCursor(0,1); lcd.print(F("   uM --,-   B --,-")); lcd.write(0xdf);
   lcd.setCursor(0,2); lcd.print(F("Gnd L --,-   G --,-")); lcd.write(0xdf);
   lcd.setCursor(0,3); lcd.print(F("Fl. K --,-   B --,-")); lcd.write(0xdf);
        
   lcd.setCursor(10,0); lcd.write(0xdf); // Put the °-sign at the right place   
   lcd.setCursor(10,1); lcd.write(0xdf);
   lcd.setCursor(10,2); lcd.write(0xdf);
   lcd.setCursor(10,3); lcd.write(0xdf);         
*/  
}
