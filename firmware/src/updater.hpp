#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <user_interface.h>
#include <memory>

#include "display.hpp"

#define FIRMWARE_LOCATION "https://foxieproducts.com/firmware/cardclock"

class Updater {
  private:
    Display& m_display;
    std::shared_ptr<WiFiClientSecure> m_client;

  public:
    Updater(Display& display) : m_display(display) {
        // allocate this on initialization since it is fairly large
        m_client = std::make_shared<WiFiClientSecure>();
        m_client->setInsecure();
    }

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

        m_display.DrawTextCentered(F("WAIT"), GRAY);
        m_display.Show();

        // if successful, this will reboot before returning
        ESPhttpUpdate.update(*m_client, F(FIRMWARE_LOCATION));
    }
};
