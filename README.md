# ESPClock

Firmware for ESP8266 clock with 8\*8 LED matrix

## Hardware

Connect an [8\*8 LED matrix](https://www.adafruit.com/product/959) and a [RTC](https://www.amazon.com/WINGONEER-DS1307-AT24C32-Module-Arduino/dp/B01H6EBRDQ/) to an ESP8266 (I used a Wemos D1 Mini Lite). 

## Configuration

If you want the clock to connect to a network and set the time every time it turns on, set `SET_TIME_ON_STARTUP` to `true` and fill in your WiFi credentials in `ssid` and `password`. Otherwise, set `SET_TIME_ON_STARTUP` to `false`. When you connect pin 14 to GND and reset the ESP8266, it will enter WiFi setup mode. Using another device, connect to the AP `clockSetupAP` and configure the WiFi for the device. It will connect to your chosen network and set the time. 
