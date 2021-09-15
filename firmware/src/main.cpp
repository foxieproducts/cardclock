#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <user_interface.h>

#define FIRMWARE_VER 1

#include "clock.hpp"
#include "config_menu.hpp"
#include "display.hpp"
#include "foxie_wifi.hpp"
#include "settings.hpp"
#include "time_menu.hpp"

void CheckForSafeMode();

void setup() {
    CheckForSafeMode();

    Settings settings;
    // settings.clear();
    // ESP.eraseConfig();

    Rtc rtc;
    Display disp(settings);
    FoxieWiFi foxieWiFi(disp, settings);
    MenuManager menuMgr(disp, settings);

    menuMgr.Add(std::make_shared<TimeMenu>(disp, rtc, settings));  // menu 0
    menuMgr.Add(std::make_shared<Clock>(disp, rtc, settings));  // clock menu 1

    auto configMenu = std::make_shared<ConfigMenu>(disp, settings);
    configMenu->Add({disp, settings, "HOUR_FMT", {"12", "24"}});
    configMenu->Add({disp, settings, "WIFI", {"OFF", "ON", "CFG"}});
    configMenu->Add({disp, settings, "ADDR", [&]() {
                         String ip = foxieWiFi.IsConnected()
                                         ? WiFi.localIP().toString()
                                         : "NOT CONNECTED";
                         disp.DrawTextScrolling(ip, GREEN);
                     }});
    configMenu->Add(
        {disp, settings, "VER", [&]() {
             String ip = foxieWiFi.IsConnected() ? WiFi.localIP().toString()
                                                 : "NOT CONNECTED";
             disp.DrawTextScrolling("FC/OS v" + String(FIRMWARE_VER) +
                                        " and may the schwarz be with you!",
                                    PURPLE);
         }});
    menuMgr.Add(configMenu);  // menu 2

    menuMgr.ActivateMenu(1);  // primary clock screen, implemented as a menu

    // use a while loop instead of loop() ... I just hate globals, OK?
    while (true) {
        rtc.Update();
        menuMgr.Update();
        foxieWiFi.Update();
        disp.Update();

        yield();  // necessary on ESP platform to allow WiFi-related code to run
    }
}

void CheckForSafeMode() {
    // if left button is held on boot, go into safe mode. this still allows
    // ArduinoOTA to function and the firmware can be updated using espota
    // just in case you accidentally brick the runtime
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    if (digitalRead(PIN_BTN_LEFT) == LOW) {
        Settings settings;
        Display disp(settings);
        FoxieWiFi foxieWiFi(disp, settings);
        disp.DrawText(1, "SAFE", ORANGE);

        while (true) {
            foxieWiFi.Update();
            disp.Update();

            yield();
        }
    }
}

void loop() {}
