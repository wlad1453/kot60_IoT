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
#define TINY_GSM_MODEM_A6

#define SerialMon Serial                // Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialAT Serial1                // Set serial for AT commands (to the module) // Use Hardware Serial on Mega, Leonardo, Micro

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

// Your GPRS credentials, if any
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

  // Set your reset, enable, power pins here

  SerialMon.println(F("Wait..."));

  // TinyGsmAutoBaud(SerialAT,GSM_AUTOBAUD_MIN,GSM_AUTOBAUD_MAX); 
  SerialAT.begin(9600);                             // was 9600, 115200
  delay(3000);

  SerialMon.println(F("Initializing modem...")); // Restart takes quite some time. To skip it, call init() instead of restart()
  modem.restart();   // 15.05
  // modem.init();
  
  String modemInfo = modem.getModemInfo();
  SerialMon.print(F("Modem Info: ")); SerialMon.println(modemInfo);
  
  sessionBegin = millis();
  randomSeed(analogRead(0));
}

void loop() {

String data  = ""; // "?Ut=50&Um=40&Bm=30&Bt=20";  
String TankT = "";
String RoomT = "";

uint8_t Ut(0), Um(0), Bm(0), Bt(0), MaccT(0), KitT(0), LivT(0), BathT(0), CorT(0), Bed1(0);   // Temperature values in heating tank and rooms
float TstT(15.2);
bool bar1(0), bar2(1), bar3(0), hWat(0), hfP(1), hrP(0), hwP(1), boilP(1), camP(1);           // State of bars and pumps. Values just for simulation
bool *eqwP[9];
bool success = false;
uint16_t eqw(0);

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
  KitT  = 15 + random(0,10); LivT = 15 + random(0,10); BathT = 15 + random(0,10); CorT = 15 + random(0,10); Bed1 = 15 + random(0,10);

  tstT = 15.0 + random(100) / 10.0;
  SerialMon.print("tstT = "); SerialMon.println(tstT); 
  
  /******  End of Simulation   *******/
  
  MaccT = (Ut + Um + Bm + Bt) / 4;                                                      // Middle temp. value in the heat accumulator 
  TankT = Tconv(Ut) + Tconv(Um) + Tconv(Bm) + Tconv(Bt);                                // String variable to be sent to a6script.php
  RoomT = Tconv(KitT) + Tconv(LivT) + Tconv(BathT) + Tconv(CorT) + Tconv(Bed1);         // String variable to be sent to a6script.php


  
  SerialMon.print(F("\nTankTemp: ")); SerialMon.print(TankT);  SerialMon.print(F("  RoomTemp: ")); SerialMon.print(RoomT);  
  SerialMon.println(F("\t\t\t\t**  HW status"));
  SerialMon.print(F("Minuts from the session begin:  ")); SerialMon.print( (millis() - sessionBegin) / 60000 ); 
  SerialMon.println("\t\t\t\t\t**");
    
  bool eqwArr[] = {camP, boilP, hwP, hrP, hfP, hWat, bar3, bar2, bar1};     // Equipment status array of bool.
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
  
  data = "?Ut=" + String(Ut) + "&Um=" + String(Um) + "&Bm=" + String(Bm) + "&Bt=" + String(Bt) + "&TstT=" + String(tstT) + "&eqw=" + String(eqw);
                                                                                        // Data to be sent to a6script within GET proc.
  data = resource + data;  // "/a6script.php"                                           // Complete HTML GET command data
  // data = resource;
  
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
      SerialMon.print(c);
      
      if ( c == '$') { j = i; k = 1; }        // SerialMon.print(F(" ---> U ")); SerialMon.println(i); 
      if ( i == j + k && c == 'e') { k++;  }  // , SerialMon.print(F(" ---> t ")); SerialMon.println(i)
      if ( i == j + k && c == 'q') { k++;  }   //  , SerialMon.print(F(" ---> _ ")); SerialMon.println(i)
      if ( i == j + k && c == 'w') { k++;  }  // , SerialMon.print(F(" ---> = ")); SerialMon.println(i)
      if ( i == j + k && c == ' ') { k++; d = ""; }   // SerialMon.print(F(" ---> _ ")); SerialMon.println(i);  
      if ( k == 5 && i > j + k + 1 && i < j + k + 5 ) {
        d += c ; eqw = d.toInt(); s++;
        if ( s == 3 ) { 
        SerialMon.print(" i: "); SerialMon.print(i);
        SerialMon.print(" $eqw: "); SerialMon.print(eqw); 
        s = 0;
        
        lastDataSent = millis();
        SerialMon.print(F(" Last Data Sent  ")); SerialMon.print(lastDataSent);  
        success = true;
        }
      }  
      i++;
      timeout = millis();
    }
  }  
  
  //SerialMon.println(F("\n ***** End reading ***** "));
  // SerialMon.println(d); 
  //SerialMon.println(F(" ***** End fragment ***** \n"));
  // d = "";

  // Shutdown

  client.stop();                SerialMon.println(F("Server disconnected"));

  modem.gprsDisconnect();       SerialMon.println(F("GPRS disconnected"));

  if ( success ) {
    delay (1800000);          // send data once a 30 min
    success = false;
  } else {
    delay (5000); // 10 000
  }
  
}

void ModemA6_ON(byte pwrON) {
  pinMode(pwrON, OUTPUT);
  digitalWrite(pwrON, HIGH);
  delay(2000);
  digitalWrite(pwrON,LOW);  
  }

String Tconv(int T) {                     // converts temperature value (int) into String format. 1. additional "0", 2. negative values
  if (T < 10) return "0" + String(T);
  else if (T < 0) return String(100 + T);
  else return String(T);  
  }

uint16_t eqSta(byte lngt, bool bo[], bool* boP[]) {               // This function encode the equipment status data into a binary number

     /* for (byte i = 0; i < lngt; i++) {
      Serial.print(i); Serial.print(" within eqSta  "); Serial.println(bo[i]);
     } */
  
  uint16_t eqD = 0, eqDd = 0;                       // Equipment data as a binary number
  uint16_t dig = 1;                        // digit weight. 1 means LSB
  for (byte i = 0; i < lngt; i++) {
    eqD += dig * bo[i];                   // Number each bit of which corresponds with the cirtaine peace of equipment
    // Serial.print(i); Serial.print("  "); Serial.print(bo[i]); Serial.print("  "); Serial.print(dig); Serial.print("  "); Serial.println(eqD);
    dig *= 2;
    }
    eqDd = eqD;
  for (int i = 0; i < 9; i++) {
    if ( eqDd % 2 ) {
     *boP[i] = 1;      // assign the value of bool to the correspond address.
     eqDd = (eqDd - 1) / 2;
    }
    else {
     *boP[i] = 0;      // assign the value.
     eqDd = eqDd / 2;
    }
  }
  
   return eqD;
  }
