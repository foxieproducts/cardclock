#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <user_interface.h>

#include "clock_menu.hpp"
#include "display.hpp"
#include "foxie_esp12.hpp"

Display g_display;

void setup() {
    g_display.SetBrightness(5);
    MenuManager menuMgr(g_display);

    menuMgr.Add(std::make_shared<ClockMenu>(g_display));

    setupWiFi("FoxieClock");

    while (true) {
        handleWiFi();

        menuMgr.Update();

        yield();
    }
}

void loop() {}
