/*
 * ESPClock v1.0
 * (c) 2019 Merlin04
 * This code is licensed under the MIT license (https://opensource.org/licenses/MIT)
 * 
 * NTP code based off of https://www.arduino.cc/en/Tutorial/UdpNtpClient
 */


// Settings
const bool SET_TIME_ON_STARTUP = true;
const char* ssid     = ""; // Hardcoded ssid/password combo for setting time on startup
const char* password = "";

// Needed for WifiManager
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

// Needed for UDP/NTP
#include <WiFiUdp.h>

// Needed for RTC/Matrix
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include "RTClib.h"

// Font
#include "font.h"

Adafruit_8x8matrix matrix = Adafruit_8x8matrix();
RTC_DS1307 RTC;

int lastMinute = 60;

WiFiManager wifiManager;

// UDP Init

WiFiUDP Udp;
unsigned int localPort = 8888;       // local port to listen for UDP packets
const char timeServer[] = "time.nist.gov"; // time.nist.gov NTP server
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

void setup () {
    // Init peripherals
    Serial.begin(115200);
    d("Serial Connected!");
    Wire.begin();
    d("Wire");
    matrix.begin(0x70);
    d("Matrix");
    RTC.begin();
    d("RTC Begin");
    if (! RTC.isrunning()) {
      Serial.println("RTC is NOT running!");
      // following line sets the RTC to the date & time this sketch was compiled
      // RTC.adjust(DateTime(__DATE__, __TIME__));
    }
    d("RTC running");
    pinMode(14, INPUT_PULLUP); // D5
    // Check if the WiFi setup button is pressed
    d("WiFiManager init...");
    if(digitalRead(14) == LOW) {
      d("WiFi setup button pressed");
      delay(50);
      while(digitalRead(14) == LOW) {delay(1);}
      wifiManager.autoConnect("clockSetupAP");
      d("WiFi setup complete");
      Udp.begin(localPort);
      d("UDP begin");
      setTime();
      d("setTime done");
    }
    // Check if the SET_TIME_ON_STARTUP setting is set
    else if(SET_TIME_ON_STARTUP) {
      // connect to hardcoded credentials
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid, password);
      d("WiFi setup complete");
      Udp.begin(localPort);
      d("UDP begin");
      setTime();
      d("setTime done");
    }
    d("Disconnecting");
    WiFi.disconnect();
    d("Entering loop!");
}



/*
Notes:

RTC functions:
  RTC.now() (DateTime type)
  DateTime.year/month/day/hour/minute/second


Matrix:
  matrix.clear();
  matrix.drawBitmap(0, 0, smile_bmp, 8, 8, LED_ON);
  matrix.writeDisplay();
  delay(500);

Font:
0-11: Hour 1-12
12, 13: AM, PM
14-19: 0-5 Minute 10s digit
20-30: 0-9 Minute 1s digit

IMAGES[24] is 3 minutes 1s digit
*/


void loop () {
    DateTime now = RTC.now();
    if(not(now.minute() == lastMinute)) {
      d("Drawing time");
      drawTime(now);
      lastMinute = now.minute();
    }
    delay(100);
}


void drawTime(DateTime dtTime) {
  // Function to draw the time given a DateTime object
  int ones = (dtTime.minute()%10);
  int tens = ((dtTime.minute()/10)%10);
  int hour12 = dtTime.hour();
  int ampm = 12;
  if(hour12 > 12) {
     hour12 = hour12 - 12;
     ampm = 13;
  }
  d(String(hour12));
  d(":");
  d(String(tens));
  d(String(ones));
  d(String(ampm));
  matrix.clear();
  matrix.drawBitmap(0, 0, IMAGES[hour12 - 1], 8, 8, LED_ON); // Hour
  matrix.drawBitmap(0, 0, IMAGES[tens + 14], 8, 8, LED_ON); // Tens place
  matrix.drawBitmap(0, 0, IMAGES[ones + 20], 8, 8, LED_ON); // Hour
  matrix.drawBitmap(0, 0, IMAGES[ampm], 8, 8, LED_ON); // AM/PM
  matrix.writeDisplay();
  d("Time drawn!");
}

void setTime() {
  // Connect to WiFi and set the time
  sendNTPpacket(timeServer);
  delay(1000);
  if (Udp.parsePacket()) {
    // We've received a packet, read the data from it
    Udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    // the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // hour, minute and second:
    int newHour = (epoch  % 86400L) / 3600; // the hour (86400 equals secs per day)
    int newMinute = (epoch  % 3600) / 60; // the minute (3600 equals secs per minute)
    int newSecond = epoch % 60; // the second
    d(String(newHour));
    d(":");
    d(String(newMinute));
    d(":");
    d(String(newSecond));
    if(newHour<9){
      DateTime newDate = DateTime(2019, 1, 4, (24 - (8 - newHour)), newMinute, newSecond);
      RTC.adjust(newDate);
    }
    else {
      DateTime newDate = DateTime(2019, 1, 4, newHour - 8, newMinute, newSecond);
      RTC.adjust(newDate);
    }
    
  }
  else {
    setTime();
  }
}


// send an NTP request to the time server at the given address
void sendNTPpacket(const char * address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); // NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void d(String myCharArray) {
  Serial.println(myCharArray);
}

