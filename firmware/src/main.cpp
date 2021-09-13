#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <LittleFS.h>
#include <user_interface.h>

#include "display.hpp"
#include "foxie_esp12.hpp"
#include "settings.hpp"

#include "clock_menu.hpp"
#include "settings_menu.hpp"
#include "time_menu.hpp"

void setup() {
    Settings settings;
    Rtc rtc;
    Display disp(settings);

    MenuManager menuMgr;
    menuMgr.Add(std::make_shared<TimeMenu>(disp, rtc, settings));
    menuMgr.Add(std::make_shared<ClockMenu>(disp, rtc, settings));
    menuMgr.Add(std::make_shared<SettingsMenu>(disp, settings));

    disp.Update();
    menuMgr.ActivateMenu(1);

    // setupWiFi("FoxieClock");

    while (true) {
        // handleWiFi();

        rtc.Update();
        menuMgr.Update();
        disp.Update();

        delay(5);
    }
}

void loop() {}
