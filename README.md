# FakeGPS v2
# ESP8266 time server for clocks using GPS, Wifi and [WorldTime-API](http://worldtimeapi.org) 
https://github.com/ajaypala/FakeGPS Inspired by: https://github.com/xSnowHeadx/FakeGPS

README ajaypala July 2022

## Introduction
I have an [Adafruit Ice Tube Clock](https://www.adafruit.com/product/194).

![Ice-Tube-Clock](https://github.com/adafruit/Ice-Tube-Clock/blob/master/assets/image.jpg)

Unfortunately these clocks only have a crystal for time accuracy and no automatic DST (Daylight Savings Time) adjustment. To get the exact time you can buy a GPS Receiver and hope there is sufficent GPS reception indoors. Alternatively, lets make a cheap alternative using the internet to get the time instead of GPS.

## Function
The FakeGPS requests the Local Time (including any Daylight Saving offset) from the [WorldTime-API](http://worldtimeapi.org) over WiFi and Internet and generates a GPRMC-Message in the GPS-Format NMEA-0183 as required for the Clock.
The first version of [firmware for the Ice Tube Clock](https://github.com/adafruit/Ice-Tube-Clock) doesnt support GPS time, however it was added shortly afterwards. The default baud rate in this version of the ice tube clock is 4800.  So there normally are no firmware-modifications necessary to adapt the timezone and DST because they will be investigated out of the public IP of the request. For special cases (f.e. the use of a foreign proxy server) the timezone can be selected manually by replacing "ip" with the wished [timezone](http://worldtimeapi.org/timezone). 

This converter generates a timestamp in local time and takes into account the DST-state. So the ice tube clock can be set to a UTC offset of 0 and doesn't need any changes for DST on the clock.

On first usage or if the module can't connect to the local WiFi-network it starts as accesspoint named "FakeGPS-AP" with no password. Connect to this AP and configure the SSID and Key of your local network as described [here](https://github.com/tzapu/WiFiManager). Then the module will connect to your network, act as GPS-Time-Server and keep the Wifi access data for the next power cycle. There is a 3 minute timeout for the WiFi Manager. If a successful WiFi connection is not achieved after timeout, the ESP8266 will restart. 

If you wish to change the Wifi SSID and key, invoke a double reset (within 10 seconds to force the WiFi Manager to start. 3 minute time out still applies. Old settings are retained until new settings are successful. 

## Hardware
* [ESP8266 LOLIN/Wemos D1 Mini](https://www.wemos.cc/en/latest/d1/d1_mini.html). 
* For the connection to the clock I used some wires with connectors for easy disassembly.

The serial GPS signal comes from Pin TXD1 (GPIO2, D4) of the Wemos D1 Mini with 9600 baud. The baud rate can be changed by . Connect the Wemos D1 Mini to 5V, GND and pin 2 of Atmega in ice tube clock as described in the adafruit docs instructions of the clock and configure the timereceiver of the clock for GPS-Format and 9600 Baud.
 
## Software
Because the Wemos D1 Mini is Arduino-compatible it can be programmed with any IDE for Arduino. Beside the Code-file published here there are some additional libraries needed, detailed below.


## Known Issues
The time will be incorrect around the time of a daylights saving time transition until the next scheduled request to obtain the time from WorldTime-API. This duration can be reduced by reducing the WorldTime-API polling interval (WTAPERIOD). In reality, this shouldn't be an issue as most people are fast asleep and wouldn't even notice.


##### Standard libraries
* [ESP8266HTTPClient](https://github.com/esp8266/Arduino)
##### Special libraries
* [WiFiManager](https://github.com/tzapu/WiFiManager)
* [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
* [TimeLib](https://github.com/PaulStoffregen/Time)
* [DoubleResetDetector](https://github.com/datacute/DoubleResetDetector)


