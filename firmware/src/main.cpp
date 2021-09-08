#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <user_interface.h>

#include "display.hpp"
#include "foxie_esp12.hpp"

#include "clock_menu.hpp"
#include "test_menu.hpp"
#include "time_menu.hpp"

void setup() {
    Rtc rtc;
    Display disp;
    disp.SetBrightness(5);

    MenuManager menuMgr;

    menuMgr.Add(std::make_shared<TestMenu>(disp));
    menuMgr.Add(std::make_shared<TimeMenu>(disp, rtc));
    menuMgr.Add(std::make_shared<ClockMenu>(disp, rtc));

    setupWiFi("FoxieClock");

    while (true) {
        handleWiFi();

        rtc.Update();
        menuMgr.Update();

        disp.SetBrightness(
            map(disp.GetBrightness(), 0, 100, MIN_BRIGHTNESS, MAX_BRIGHTNESS));
        disp.Show();

        delay(5);
    }
}

void loop() {}
