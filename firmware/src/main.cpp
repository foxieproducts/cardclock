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
#include "test_menu.hpp"
#include "time_menu.hpp"

void setup() {
    Settings settings;
    Rtc rtc;
    Display disp(settings);
    disp.SetBrightness(5);

    MenuManager menuMgr;

    menuMgr.Add(std::make_shared<TestMenu>(disp, settings));
    menuMgr.Add(std::make_shared<TimeMenu>(disp, rtc, settings));
    menuMgr.Add(std::make_shared<ClockMenu>(disp, rtc, settings));

    // setupWiFi("FoxieClock");
    disp.Update();
    disp.DrawTextScrolling(settings["test"], PURPLE);

    // settings.Save();

    while (true) {
        // handleWiFi();

        rtc.Update();
        menuMgr.Update();
        disp.Update();

        delay(5);
    }
}

void loop() {}
