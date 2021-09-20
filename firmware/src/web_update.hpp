#pragma once
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266httpUpdate.h>
#include <user_interface.h>  // for ESP-specific API calls
#include <memory>            // for std::shared_ptr

#include "button.hpp"
#include "display.hpp"

class WebUpdate {
  private:
    Settings& m_settings;
    Display& m_display;
    std::shared_ptr<WiFiClientSecure> m_client;

  public:
    WebUpdate(Settings& settings, Display& display)
        : m_settings(settings), m_display(display) {
        // allocate this on initialization since it is fairly large
        m_client = std::make_shared<WiFiClientSecure>();
        m_client->setInsecure();
    }

    void Download() {
        ElapsedTime connectTime;
        m_display.Clear();
        m_display.DrawText(1, F("<(I)>"), BLUE);
        m_display.Show();
        while (!WiFi.isConnected()) {
            if (Button::AreAnyButtonsPressed()) {
                m_display.DrawTextScrolling(F("Canceled"), GRAY);
                return;
            }
            yield();
        }
        ConfigureESPHttpUpdate();
        if (CheckForNewVersion()) {
            BeginDownload();
        }
    }

  private:
    void ConfigureESPHttpUpdate() {
        ESPhttpUpdate.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        ESPhttpUpdate.rebootOnUpdate(true);

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
                m_display.DrawInsideRingPixel(map(progress, 0, total, 0, 11),
                                              PURPLE);
                m_display.Clear();
                m_display.DrawTextCentered(
                    String(map(progress, 0, total, 0, 100)) + F("%"), BLUE);
                m_display.Update();
                if (Button::AreAnyButtonsPressed() == PIN_BTN_LEFT &&
                    m_settings[F("DEVL")] == F("ON")) {
                    m_display.Clear();
                    m_display.DrawTextCentered("CNCL", RED);
                    m_display.Show();
                    Button::WaitForNoButtons();
                    ESP.restart();
                }
            });
        ESPhttpUpdate.onError([&](int error) {
            m_display.DrawTextScrolling(ESPhttpUpdate.getLastErrorString(),
                                        RED);
            ESP.restart();
        });
    }

    bool CheckForNewVersion() {
        size_t ver = GetVersionFromServer();
        if (ver == 0) {
            return false;
        } else if (ver == FW_VERSION) {
            m_display.DrawTextScrolling(F("Up to date, install again?"), GRAY);
        } else {
            m_display.DrawTextScrolling(
                F("Press UP to install V") + String(ver), GREEN);
        }

        m_display.Clear();
        m_display.DrawText(1, F("UP?"), ORANGE);
        m_display.DrawChar(13, CHAR_UP_ARROW, GREEN);
        m_display.Show();

        if (Button::WaitForButtonPress(15000) != PIN_BTN_UP) {
            m_display.DrawTextScrolling(F("Canceled"), GRAY);
            return false;
        }

        return true;
    }

    void BeginDownload() {
        m_display.Clear();
        m_display.DrawTextCentered(F("<<"), BLUE);
        m_display.Show();

        // if successful, this will reboot before returning
        ESPhttpUpdate.update(*m_client,
                             F("https://") + String(F(FW_DOWNLOAD_ADDRESS)));
    }

  private:
    size_t GetVersionFromServer() {
        HTTPClient https;
        https.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

        m_display.Clear();
        m_display.DrawTextCentered(F(">>"), BLUE);
        m_display.Show();

        if (https.begin(*m_client,
                        F("https://") + String(F(FW_VERSION_ADDR)))) {
            const int httpCode = https.GET();

            // file found at server
            if (httpCode == HTTP_CODE_OK ||
                httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                // success!
                return https.getString().toInt();
            } else {
                m_display.DrawTextScrolling(https.errorToString(httpCode),
                                            DARK_RED);
            }

            https.end();
        }

        return 0;  // failed to get version
    }
};
