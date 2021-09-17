#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <user_interface.h>

#include "display.hpp"

#define FIRMWARE_LOCATION "https://foxieproducts.com/firmware/cardclock"

class Updater {
  private:
    Display& m_display;

  public:
    Updater(Display& display) : m_display(display) {}

    void Download() {
        if (!WiFi.isConnected()) {
            m_display.DrawTextScrolling(F("Not connected to WiFi"), ORANGE);
            return;
        }

        ESPhttpUpdate.onStart([&]() {
            m_display.ClearRoundLEDs(DARK_GRAY);
            m_display.Show();
        });
        ESPhttpUpdate.onEnd([&]() {
            m_display.Clear();
            m_display.DrawTextCentered(F("FLSH"), ORANGE);
            m_display.Show();
        });
        ESPhttpUpdate.onProgress(
            [&](unsigned int progress, unsigned int total) {
                m_display.DrawPixel(
                    FIRST_HOUR_LED + map(progress, 0, total, 0, 11), PURPLE);
                m_display.Clear();
                m_display.DrawTextCentered(
                    String(map(progress, 0, total, 0, 100)) + F("%"), PURPLE);
                m_display.Show();
            });
        ESPhttpUpdate.onError([&](int error) {
            m_display.DrawTextScrolling(ESPhttpUpdate.getLastErrorString(),
                                        RED);
        });
        ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        ESPhttpUpdate.rebootOnUpdate(true);

        // if successful, this will reboot before returning
        WiFiClientSecure* client = new WiFiClientSecure();
        client->setInsecure();

        m_display.DrawTextCentered(F("WAIT"), GRAY);
        m_display.Show();

        ESPhttpUpdate.update(*client, F(FIRMWARE_LOCATION));
    }
};
