#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <user_interface.h>
#include <memory>

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
    auto settings = std::make_shared<Settings>();
    auto display = std::make_shared<Display>(*settings);
    auto wifi = std::make_shared<FoxieWiFi>(*display, *settings);
    auto updater = std::make_shared<Updater>(*display);

    CheckButtonsOnBoot(*settings, *display, *wifi);

    auto menuMgr = std::make_shared<MenuManager>(*display, *settings);
    auto rtc = std::make_shared<Rtc>();
    auto ntp = std::make_shared<FoxieNTP>(*settings, *rtc);

    menuMgr->Add(
        std::make_shared<TimeMenu>(*display, *rtc, *settings));  // menu 0
    menuMgr->Add(
        std::make_shared<Clock>(*display, *rtc, *settings));  // clock menu 1

    auto configMenu = std::make_shared<ConfigMenu>(*display, *settings);
    configMenu->AddTextSetting(F("HOUR"), {"12", "24"});
    configMenu->AddRangeSetting(F("UTC"), -12, 12,
                                [&]() { ntp->UpdateRTCTime(); });
    configMenu->AddTextSetting(F("WIFI"), {"OFF", "ON", "CFG"});
    configMenu->AddRunFuncSetting(F("INFO"), [&]() {
        String info;
        info += F("IP: ");
        info +=
            WiFi.isConnected() ? WiFi.localIP().toString() : F("NOT CONNECTED");
        info += F(" FHEAP: ") + String(ESP.getFreeHeap());
        info += F(" FCONTSTACK: ") + String(ESP.getFreeContStack());

        display->DrawTextScrolling(info, GREEN);
    });
    configMenu->AddRunFuncSetting(F("VER"), [&]() {
        display->DrawTextScrolling(F("FC/OS v") + String(FIRMWARE_VER) +
                                       F(" and may the schwarz be with you!"),
                                   PURPLE);
    });

    configMenu->AddRangeSetting(F("MINB"), MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    configMenu->AddRangeSetting(F("MAXB"), MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    configMenu->AddTextSetting(F("WLED"), {F("OFF"), F("ON")});

    configMenu->AddTextSetting(F("DEVL"), {F("OFF"), F("ON")});
    configMenu->AddRunFuncSetting(F("UPDT"), [&]() { updater->Download(); });

    menuMgr->Add(configMenu);  // menu 2

    menuMgr->ActivateMenu(1);  // primary clock screen, implemented as a menu

    // use a while loop instead of loop() ... I just hate globals, OK?
    while (true) {
        rtc->Update();
        ntp->Update();
        menuMgr->Update();
        wifi->Update();
        display->Update();

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
        display.DrawTextCentered(F("SAFE"), ORANGE);
        settings[F("DEVL")] == F("ON");
        while (true) {
            wifi.Update();
            display.Update();

            yield();  // necessary on ESP platform to allow WiFi-related code
                      // to run
        }
    }

    // if up button is held on boot, clear settings.
    if (digitalRead(PIN_BTN_UP) == LOW) {
        display.DrawTextScrolling(F("SETTINGS CLEARED"), PURPLE);
        settings.clear();
        settings.Save();
        ESP.eraseConfig();
    }
}

void loop() {}
