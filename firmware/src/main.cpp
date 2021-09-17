#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <user_interface.h>

#define FIRMWARE_VER 1

#include "clock.hpp"
#include "config_menu.hpp"
#include "display.hpp"
#include "foxie_ntp.hpp"
#include "foxie_wifi.hpp"
#include "option.hpp"
#include "settings.hpp"
#include "time_menu.hpp"
#include "updater.hpp"

void CheckButtonsOnBoot(Settings& settings, Display& display, FoxieWiFi& wifi);

void setup() {
    Settings settings;
    Display disp(settings);
    FoxieWiFi wifi(disp, settings);

    CheckButtonsOnBoot(settings, disp, wifi);

    MenuManager menuMgr(disp, settings);
    Rtc rtc;
    FoxieNTP ntp(settings, rtc);

    menuMgr.Add(std::make_shared<TimeMenu>(disp, rtc, settings));  // menu 0
    menuMgr.Add(std::make_shared<Clock>(disp, rtc, settings));  // clock menu 1

    auto configMenu = std::make_shared<ConfigMenu>(disp, settings);
    configMenu->AddTextSetting("HOUR", {"12", "24"});
    configMenu->AddRangeSetting("UTC", -12, 12, [&]() { ntp.UpdateRTCTime(); });
    configMenu->AddTextSetting("WIFI", {"OFF", "ON", "CFG"});
    configMenu->AddRunFuncSetting("INFO", [&]() {
        String info;
        info += "IP: ";
        info +=
            WiFi.isConnected() ? WiFi.localIP().toString() : "NOT CONNECTED";
        info += " MEM FREE: " + String(ESP.getFreeHeap());

        disp.DrawTextScrolling(info, GREEN);
    });
    configMenu->AddRunFuncSetting("VER", [&]() {
        disp.DrawTextScrolling("FC/OS v" + String(FIRMWARE_VER) +
                                   " and may the schwarz be with you!",
                               PURPLE);
    });

    configMenu->AddRangeSetting("MINB", MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    configMenu->AddRangeSetting("MAXB", MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    configMenu->AddTextSetting("WLED", {"OFF", "ON"});

    configMenu->AddTextSetting("DEVL", {"OFF", "ON"});
    configMenu->AddRunFuncSetting("UPDT", [&]() {
        Updater updater(disp);
        updater.Download();
    });

    menuMgr.Add(configMenu);  // menu 2

    menuMgr.ActivateMenu(1);  // primary clock screen, implemented as a menu

    // use a while loop instead of loop() ... I just hate globals, OK?
    while (true) {
        rtc.Update();
        ntp.Update();
        menuMgr.Update();
        wifi.Update();
        disp.Update();

        yield();  // necessary on ESP platform to allow WiFi-related code to
                  // run
    }
}

void CheckButtonsOnBoot(Settings& settings, Display& display, FoxieWiFi& wifi) {
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT);
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);

    // if left button is held on boot, go into safe mode. this still allows
    // ArduinoOTA to function and the firmware can be updated using espota
    // just in case you accidentally brick the runtime
    if (digitalRead(PIN_BTN_LEFT) == LOW) {
        display.DrawTextCentered("SAFE", ORANGE);
        settings["DEVL"] == "ON";
        while (true) {
            wifi.Update();
            display.Update();

            yield();  // necessary on ESP platform to allow WiFi-related code
                      // to run
        }
    }

    // if up button is held on boot, clear settings.
    if (digitalRead(PIN_BTN_UP) == LOW) {
        display.DrawTextScrolling("SETTINGS CLEARED", PURPLE);
        settings.clear();
        settings.Save();
        ESP.eraseConfig();
    }
}

void loop() {}
