#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <user_interface.h>  // for ESP-specific API calls
#include <memory>            // for std::shared_ptr

// you might be asking yourself -- why is all the code in header files?
// answer: in a relatively small project like this, the separation of .hpp/.cpp
// files adds an unnecessarily cumbersome layer to rapid iteration

#include "clock.hpp"
#include "config_menu.hpp"
#include "display.hpp"
#include "foxie_ntp.hpp"
#include "foxie_wifi.hpp"
#include "option.hpp"
#include "settings.hpp"
#include "time_menu.hpp"
#include "web_update.hpp"

void CheckButtonsOnBoot(Settings& settings, Display& display, FoxieWiFi& wifi);

void setup() {
    using namespace std;
    auto settings = make_shared<Settings>();
    auto display = make_shared<Display>(*settings);
    auto wifi = make_shared<FoxieWiFi>(*settings, *display);
    auto updater = make_shared<WebUpdate>(*settings, *display);

    CheckButtonsOnBoot(*settings, *display, *wifi);

    auto rtc = make_shared<Rtc>(*settings);
    auto ntp = make_shared<FoxieNTP>(*settings, *rtc);
    auto menuMgr = make_shared<MenuManager>(*display, *settings);

    menuMgr->Add(make_shared<TimeMenu>(*display, *rtc, *settings));  // menu 0
    menuMgr->Add(make_shared<Clock>(*display, *rtc, *settings));     // menu 1

    // all config menu options are below
    auto configMenu = make_shared<ConfigMenu>(*display, *settings);  // menu 2
    configMenu->AddRangeSetting(F("MINB"), MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    configMenu->AddRangeSetting(F("MAXB"), MIN_BRIGHTNESS, MAX_BRIGHTNESS);
    configMenu->AddTextSetting(F("CLKB"), {F("OFF"), F("ON")});
    configMenu->AddTextSetting(F("WLED"), {F("OFF"), F("ON")});
    configMenu->AddTextSetting(F("24HR"), {"OFF", "ON"});
    configMenu->AddRangeSetting(F("UTC"), -12, 12,
                                [&]() { ntp->UpdateRTCTime(); });
    configMenu->AddTextSetting(F("WIFI"), {"OFF", "ON", "CFG"});
    configMenu->AddRunFuncSetting(F("INFO"), [&]() {
        String info;
        info += F("IP:");
        info +=
            WiFi.isConnected() ? WiFi.localIP().toString() : F("NOT CONNECTED");
        info += F(" FH:") + String(ESP.getFreeHeap());
        info += F(" FCS:") + String(ESP.getFreeContStack());
        info += F(" UPT:") + String(millis() / 1000 / 60);

        display->DrawTextScrolling(info, GREEN);
    });
    configMenu->AddRunFuncSetting(F("VER"), [&]() {
        display->DrawTextScrolling(F("FC/OS v") + String(FW_VERSION) +
                                       F(" and may the schwarz be with you!"),
                                   PURPLE);
    });
    configMenu->AddTextSetting(F("DEVL"), {F("OFF"), F("ON")});
    configMenu->AddRunFuncSetting(F("UPDT"), [&]() { updater->Download(); });
    menuMgr->Add(configMenu);

    menuMgr->SetDefaultAndActivateMenu(1);  // clock menu

    // use a while loop instead of loop() ... I just hate globals, OK?
    while (true) {
        rtc->Update();
        ntp->Update();
        menuMgr->Update();
        wifi->Update();
        display->Update();

        yield();  // allow the ESP's system/WiFi code a chance to run
    }
}

void CheckButtonsOnBoot(Settings& settings, Display& display, FoxieWiFi& wifi) {
    pinMode(PIN_BTN_UP, INPUT_PULLUP);
    pinMode(PIN_BTN_DOWN, INPUT);
    pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
    pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);

    // if left button is held on boot, go into safe mode. this still allows
    // ArduinoOTA to function and the firmware can be updated using espota
    // just in case you accidentally put the board into a reboot loop that
    // doesn't involve any of the code this "safe" mode depends on...
    if (Button::AreAnyButtonsPressed() == PIN_BTN_LEFT) {
        display.DrawTextCentered(F("SAFE"), ORANGE);
        settings[F("DEVL")] == F("ON");
        while (true) {
            wifi.Update();
            display.Update();

            yield();  // allow the ESP's system/WiFi code a chance to run
        }
    }

    // if UP button is held on boot, clear settings.
    if (Button::AreAnyButtonsPressed() == PIN_BTN_UP) {
        display.SetBrightness(20);
        display.DrawText(0, F("CLR?"), ORANGE);
        display.DrawChar(14, CHAR_RIGHT_ARROW, GREEN);
        display.Show();

        if (Button::WaitForButtonPress() == PIN_BTN_RIGHT) {
            display.DrawTextScrolling(F("SETTINGS CLEARED"), PURPLE);
            settings.clear();
            settings.Save();
            ESP.eraseConfig();
        }
        Button::WaitForNoButtons();
    }
}

void loop() {}
