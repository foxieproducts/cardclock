#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <user_interface.h>

#include "clock_menu.hpp"
#include "config_menu.hpp"
#include "display.hpp"
#include "foxie_wifi.hpp"
#include "settings.hpp"
#include "time_menu.hpp"

void setup() {
    Settings settings;
    // settings.clear();
    // ESP.eraseConfig();

    Rtc rtc;
    Display disp(settings);
    FoxieWiFi foxieWiFi(disp, settings);

    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    if (digitalRead(PIN_BTN_LEFT) == LOW) {
        // safe mode OTA on boot. Hopefully WiFi is configured!
        disp.DrawText(1, "SAFE", ORANGE);
        while (true) {
            rtc.Update();
            foxieWiFi.Update();
            disp.Update();

            delay(5);
        }
    }

    MenuManager menuMgr(disp, settings);
    menuMgr.Add(std::make_shared<TimeMenu>(disp, rtc, settings));   // menu 0
    menuMgr.Add(std::make_shared<ClockMenu>(disp, rtc, settings));  // menu 1

    auto configMenu = std::make_shared<ConfigMenu>(disp, settings);
    configMenu->Add({disp, settings, "HOUR_FMT", {"12", "24"}});
    configMenu->Add({disp, settings, "WIFI", {"OFF", "ON", "CFG"}});
    configMenu->Add({disp, settings, "ADDR", [&]() {
                         String ip = foxieWiFi.IsConnected()
                                         ? WiFi.localIP().toString()
                                         : "NOT CONNECTED";
                         disp.DrawTextScrolling(ip, GREEN);
                     }});
    menuMgr.Add(configMenu);  // menu 2

    menuMgr.ActivateMenu(1);  // clock menu

    while (true) {
        rtc.Update();
        menuMgr.Update();
        foxieWiFi.Update();
        disp.Update();

        delay(5);
    }
}

void loop() {}
