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

    MenuManager menuMgr(disp, settings);
    menuMgr.Add(std::make_shared<TimeMenu>(disp, rtc, settings));
    menuMgr.Add(std::make_shared<ClockMenu>(disp, rtc, settings));

    auto configMenu = std::make_shared<ConfigMenu>(disp, settings);
    configMenu->Add({disp, settings, "HOUR_FMT", {"12", "24"}});
    configMenu->Add({disp, settings, "WIFI", {"OFF", "ON", "CFG"}});
    menuMgr.Add(configMenu);
    menuMgr.ActivateMenu(1);

    ElapsedTime waitToInit;

    FoxieWiFi fwifi(disp, settings);

    while (true) {
        rtc.Update();
        menuMgr.Update();
        fwifi.Update();
        disp.Update();

        delay(5);
    }
}

void loop() {}
