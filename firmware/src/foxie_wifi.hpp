#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESPAsyncWiFiManager.h>
#include <memory>

#include "button.hpp"
#include "display.hpp"
#include "elapsed_time.hpp"
#include "settings.hpp"

class FoxieWiFi {
  private:
    Settings& m_settings;
    Display& m_display;

    std::shared_ptr<AsyncWebServer> m_server;
    std::shared_ptr<DNSServer> m_dns;
    std::shared_ptr<AsyncWiFiManager> m_wifiManager;

    bool m_isInitialized{false};
    bool m_isOTAInitialized{false};

  public:
    FoxieWiFi(Settings& settings, Display& display)
        : m_settings(settings), m_display(display) {
        m_server = std::make_shared<AsyncWebServer>(80);
        m_dns = std::make_shared<DNSServer>();
        m_wifiManager =
            std::make_shared<AsyncWiFiManager>(m_server.get(), m_dns.get());
        String value = m_settings[F("WIFI")] | "OFF";
        if (value == "OFF") {
            WiFi.forceSleepBegin();
        }
    }

    void Update() {
        if (!m_isInitialized) {
            Initialize();
        } else if (m_settings[F("WIFI")] == "CFG" ||
                   (m_settings[F("WIFI")] == "ON" &&
                    m_settings[F("wifi_configured")] != F("1"))) {
            Configure();
        } else if (m_settings[F("WIFI")] == F("OFF") && m_isInitialized) {
            WiFi.forceSleepBegin();
            m_isInitialized = false;
            m_isOTAInitialized = false;
        } else if (WiFi.isConnected() && m_settings[F("WIFI")] == F("OFF")) {
            WiFi.disconnect();
            m_settings.remove("wifi_configured");
        }

        if (m_isInitialized && WiFi.isConnected() && !m_isOTAInitialized &&
            m_settings[F("DEVL")] == F("ON")) {
            MDNS.begin(GetUniqueMDNSName().c_str());
            InitializeOTA();
        }

        if (m_isOTAInitialized && m_settings[F("DEVL")] == F("ON")) {
            ArduinoOTA.handle();
            MDNS.update();
            // server.handleClient();
        }
    }

    static bool IsConfigured() { return !WiFi.SSID().isEmpty(); }

  private:
    void Configure() {
        // TODO: Make sure config portal isn't open when calling this
        Initialize();

        m_settings[F("WIFI")] = F("OFF");
        m_settings.remove("wifi_configured");

        m_settings.Save(true);

        m_wifiManager->resetSettings();
        WiFi.persistent(true);

        m_wifiManager->setConfigPortalTimeout(120);
        m_display.DrawTextScrolling(F("Connect to Foxie_WiFiSetup"), GRAY);
        m_display.Clear();
        m_display.DrawText(1, F("<(I)>"), BLUE);
        m_display.Show();

        if (m_wifiManager->autoConnect(String(F("Foxie_WiFiSetup")).c_str())) {
            m_display.DrawTextScrolling(F("SUCCESS"), GREEN);
            m_settings[F("WIFI")] = F("ON");
            m_settings[F("wifi_configured")] = F("1");
        } else {
            m_display.DrawTextScrolling(F("FAILED"), RED);
            m_settings[F("WIFI")] = F("OFF");
            m_settings.remove("wifi_configured");
            WiFi.disconnect();
            m_isInitialized = false;
        }
        m_settings.Save(true);
    }

    void Initialize() {
        if (!m_isInitialized && m_settings[F("WIFI")] != F("OFF")) {
            WiFi.begin();
            WiFi.persistent(true);
            m_isInitialized = true;
        }
    }

    void InitializeOTA() {
        ArduinoOTA.onStart([&]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FS) {
                LittleFS.end();
            }
            m_display.Clear();
            m_display.DrawTextCentered(F("RECV"), DARK_GREEN);
            m_display.ClearRoundLEDs();
            m_display.Show();
        });
        ArduinoOTA.onEnd([&]() {
            m_display.Clear();
            m_display.DrawTextCentered(F("FLSH"), ORANGE);
            m_display.Show();
        });
        ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total) {
            m_display.DrawPixel(FIRST_HOUR_LED + map(progress, 0, total, 0, 11),
                                PURPLE);
            m_display.Clear();
            m_display.DrawTextCentered(
                String(map(progress, 0, total, 0, 100)) + F("%"), BLUE);
            m_display.Show();
            if (Button::AreAnyButtonsPressed() == PIN_BTN_LEFT) {
                while (Button::AreAnyButtonsPressed()) {
                    yield();
                }
                ESP.restart();
            }
        });
        ArduinoOTA.onError([&](ota_error_t error) {
            m_display.DrawTextScrolling(F("OTA ERR:") + String(error), RED);
        });

        ArduinoOTA.setHostname(GetUniqueMDNSName().c_str());
        ArduinoOTA.begin();
        m_isOTAInitialized = true;
    }

    String GetUniqueMDNSName() {
        return F("FoxieClock_") + String(WiFi.localIP()[3], DEC);
    }

    void SetupWebServer() {
#if 0
        server.on(
        "/update", HTTP_POST,
        []() {
            server.sendHeader("Connection", "close");
            server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            delay(100);
            ESP.restart();
        },
        []() {
            HTTPUpload& upload = server.upload();
            if (upload.status == UPLOAD_FILE_START) {
                Serial.setDebugOutput(true);
                WiFiUDP::stopAll();
                Serial.printf("Update: %s\n", upload.filename.c_str());
                uint32_t maxSketchSpace =
                    (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
                if (!Update.begin(
                        maxSketchSpace)) {  // start with max available size
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_WRITE) {
                if (Update.write(upload.buf, upload.currentSize) !=
                    upload.currentSize) {
                    Update.printError(Serial);
                }
            } else if (upload.status == UPLOAD_FILE_END) {
                if (Update.end(true)) {  // true to set the size to the current
                    // progress
                    Serial.printf("Update Success: %u\nRebooting...\n",
                                  upload.totalSize);
                } else {
                    Update.printError(Serial);
                }
                Serial.setDebugOutput(false);
            }
            yield();
        });
    server.begin();
    MDNS.addService("http", "tcp", 80);
#endif
    }
};