#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <user_interface.h>

#include "button.hpp"
#include "display.hpp"
#include "rtc.hpp"

Display g_display;
Rtc g_rtc;

void setup() {
    g_display.SetBrightness(5);
}

void loop() {
    g_display.DrawTextScrolling("hello world!", PURPLE);
}
