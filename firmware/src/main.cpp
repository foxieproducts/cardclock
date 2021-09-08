#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <user_interface.h>

#include "display.hpp"
#include "foxie_esp12.hpp"

#include "clock_menu.hpp"
#include "time_menu.hpp"

void setup() {
    Rtc rtc;
    Display disp;
    disp.SetBrightness(5);

    MenuManager menuMgr(disp);

    menuMgr.Add(std::make_shared<TimeMenu>(disp, rtc));
    menuMgr.Add(std::make_shared<ClockMenu>(disp, rtc));

    setupWiFi("FoxieClock");

    while (true) {
        handleWiFi();

        rtc.Update();
        menuMgr.Update();

        yield();
    }
}

void loop() {}
