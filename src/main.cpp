//Author: Ajay Pala
//Github: https://github.com/ajaypala/FakeGPS/
//Date: 10th July 2022
//Inspred by: xSnowHeadx March 2022 https://github.com/xSnowHeadx/FakeGPS

#include <Arduino.h>
#include <WiFiManager.h>  // https://github.com/esp8266/Arduino
#define WMAPNAME "FakeGPS-AP" //Wifi Manager AP name
#define WMTIMEOUT 180 //Wifi manager time out in seconds

#include <ArduinoJson.h>  //https://github.com/bblanchon/ArduinoJson
#include <TimeLib.h>      //https://github.com/PaulStoffregen/Time

#define WTAServerName  "http://worldtimeapi.org/api/"
char timezone[] = "ip"; // PREFERENCE: TimeZone. Go to http://worldtimeapi.org/api/timezone to find your timezone string or chose "ip" to use IP-localisation for timezone detection
#define WTAPERIOD 3600  // seconds; how often to update time from World Time APi
#define GPSPERIOD 60  // seconds; how often to update the time on the clock
#define GPSBAUD 9600  // GPS Serial bps

//== DOUBLE-RESET DETECTOR ==
#include <DoubleResetDetector.h> // https://github.com/datacute/DoubleResetDetector
#define DRD_TIMEOUT 10 // Second-reset must happen within 10 seconds of first reset to be considered a double-reset
#define DRD_ADDRESS 0 // RTC Memory Address for the DoubleResetDetector to use
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

WiFiClient wifiClient;
#include <ESP8266HTTPClient.h>
HTTPClient http;

time_t getWtaTime() {
	Serial.println("FakeGPS: Getting WTA Time");

	char url[64] = WTAServerName;
	strcat(url, timezone);

	if (!http.begin(wifiClient, url)) {
    Serial.println("FakeGPS: http begin failed");
    http.end();
    return 0;
  }
  int httpCode = http.GET();
	if (httpCode != HTTP_CODE_OK) {
    Serial.print("FakeGPS: http GET failed. Code: ");
    Serial.println(httpCode);
    http.end();
    return 0;
  }
  String payload = http.getString();
	http.end();
  if(!payload.length())  {
    Serial.println("FakeGPS: payload length failed");
    return 0;
  }

  const size_t capacity = JSON_OBJECT_SIZE(15) + 400;
	DynamicJsonDocument jsonDocument(capacity);
  if (deserializeJson(jsonDocument, payload.c_str()))  {
    Serial.println("FakeGPS: deserializeJson failed");
    return 0;
  }

  //      int week_number = jsonDocument["week_number"]; // 31
  //      const char* utc_offset = jsonDocument["utc_offset"]; // "-04:00"
  //      const char* utc_datetime = jsonDocument["utc_datetime"]; // "2019-08-01T16:58:40.68279+00:00"
  long unixtime = jsonDocument["unixtime"]; // 1564678720
  //      const char* timezone = jsonDocument["timezone"]; // "America/New_York"
  int raw_offset = jsonDocument["raw_offset"]; // -18000
  //      const char* dst_until = jsonDocument["dst_until"]; // "2019-11-03T06:00:00+00:00"
  int dst_offset = jsonDocument["dst_offset"]; // 3600
  //      const char* dst_from = jsonDocument["dst_from"]; // "2019-03-10T07:00:00+00:00"
  //      bool dst = jsonDocument["dst"]; // true
  //      int day_of_year = jsonDocument["day_of_year"]; // 213
  //      int day_of_week = jsonDocument["day_of_week"]; // 4
  //      const char* datetime = jsonDocument["datetime"]; // "2019-08-01T12:58:40.682790-04:00"
  //      const char* client_ip = jsonDocument["client_ip"]; // "23.235.227.109"
  //      const char* abbreviation = jsonDocument["abbreviation"]; // "EDT"

	return (unixtime + raw_offset + dst_offset);

} // getWtaTime


void setup() {
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);   // Initialize the LED_BUILTIN pin as an output
  digitalWrite(LED_BUILTIN, LOW); //Led on

  // Serial
  Serial.begin(115200);
  Serial.println("");
  Serial.println("FakeGPS: Booting up");

  // Wifi
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  wm.setConfigPortalTimeout(WMTIMEOUT);
  bool wmres;

  if (drd.detectDoubleReset()) {
    Serial.println("FakeGPS: Double Reset Detected; Starting Wifi Config Portal");
     wmres = wm.startConfigPortal(WMAPNAME);
  } else {
    Serial.println("FakeGPS: Single Reset Detected; Auto connect");
    wmres = wm.autoConnect(WMAPNAME);
  }


  if (!wmres)
  {
    //reset and try again
    Serial.println("FakeGPS: Failed to connect and WM hit timeout");
    drd.stop();
    Serial.println("FakeGPS: Clear DRD & Restarting");
    ESP.reset();
  }

  //if you get here you have connected to the WiFi
  Serial.println("FakeGPS: connected...yeey :)");

  setSyncInterval(GPSPERIOD);
  setSyncProvider(getWtaTime);

  Serial1.begin(GPSBAUD); //Tx only GPIO2 (D4); LED will turn off as it uses the same pin on WEMOS D1 Mini.
} // setup


void updateClock() {
    char tstr[128];
    sprintf(tstr, "$GPRMC,%02d%02d%02d,A,0000.0000,N,00000.0000,E,0.0,0.0,%02d%02d%02d,0.0,E,S",
      hour(), minute(), second(), day(), month(), year() % 100);
    
    unsigned char cs;
    unsigned int i;
    cs = 0;
    for (i = 1; i < strlen(tstr); i++)		// calculate checksum
      cs ^= tstr[i];
    sprintf(tstr + strlen(tstr), "*%02X", cs);

    Serial.println(tstr);					// send to console
    Serial1.println(tstr);					// send to clock
}


time_t prevDisplay = 0;
timeStatus_t prevTimeStatus;
timeStatus_t currentTimeStatus;

void loop() {
  drd.loop(); //service drd
  
  //get current time status
  currentTimeStatus = timeStatus();

  //is current time good?
  if (currentTimeStatus == timeSet) {
    //time is good
    if (prevTimeStatus != timeSet) {
      //increase Sync interval back to normal if it was decreased previously
      Serial.println("FakeGPS: Timeset. setSyncInterval(WTAPERIOD)");
      setSyncInterval(WTAPERIOD);
    }

    //update clock and flash LED
    //digitalWrite(LED_BUILTIN, LOW); //LED on; Doesnt work as Serial1 uses the same pin on WEMOS D1 Mini
    updateClock();  
    //digitalWrite(LED_BUILTIN, HIGH); //LED off; Doesnt work as Serial1 uses the same pin on WEMOS D1 Mini

  } else {  //time not set
    //digitalWrite(LED_BUILTIN, LOW); //LED on; Doesnt work as Serial1 uses the same pin on WEMOS D1 Mini

    if (prevTimeStatus == timeSet) {
      //decrease Sync interval to get the time set faster
      Serial.println("FakeGPS: Time not set. setSyncInterval(GPSPERIOD)");
      setSyncInterval(GPSPERIOD);
    }
  }

  prevTimeStatus = currentTimeStatus;

  delay(GPSPERIOD * 1000);  //delay until next update

} // loop
