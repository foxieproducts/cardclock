#pragma once
#include <Arduino.h>
#include <ArduinoOTA.h>

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

#include <ESPAsyncWiFiManager.h>

//#include <WiFiClient.h>
//#include <WiFiUdp.h>

#include "display.hpp"
#include "elapsed_time.hpp"
#include "settings.hpp"

class FoxieWiFi {
  private:
    Display& m_display;
    Settings& m_settings;
    AsyncWebServer m_server{80};
    DNSServer m_dns;
    AsyncWiFiManager m_wifiManager{&m_server, &m_dns};
    bool m_isInitialized{false};
    bool m_isOTAInitialized{false};

  public:
    FoxieWiFi(Display& display, Settings& settings)
        : m_display(display), m_settings(settings) {
        String value = m_settings["WIFI"] | "OFF";
        if (value = "OFF") {
            WiFi.forceSleepBegin();
        }
    }

    void Update() {
        if (!m_isInitialized) {
            Initialize();
        } else if (m_settings["WIFI"] == "CFG" ||
                   (m_settings["WIFI"] == "ON" &&
                    m_settings["wifi_configured"] != "1")) {
            Configure();
        } else if (m_settings["WIFI"] == "OFF" && m_isInitialized) {
            WiFi.forceSleepBegin();
            m_isInitialized = false;
            m_isOTAInitialized = false;
        }

        if (m_isInitialized && WiFi.isConnected() && !m_isOTAInitialized &&
            m_settings["DEVL"] == "ON") {
            MDNS.begin(GetUniqueMDNSName().c_str());
            InitializeOTA();
        }

        if (m_isOTAInitialized && m_settings["DEVL"] == "ON") {
            ArduinoOTA.handle();
            MDNS.update();
            // server.handleClient();
        }

        m_wifiManager.loop();
    }

    static bool IsConfigured() { return !WiFi.SSID().isEmpty(); }

  private:
    void Configure() {
        // TODO: Make sure config portal isn't open when calling this
        Initialize();

        m_settings["WIFI"] = "OFF";
        m_settings["wifi_configured"].clear();
        m_settings.Save(true);

        m_wifiManager.resetSettings();
        WiFi.persistent(true);

        m_wifiManager.setConfigPortalTimeout(180);
        m_display.DrawTextScrolling("Connect to Foxie_WiFiSetup", GRAY);
        m_display.Clear();
        m_display.DrawText(1, "<(I)>", BLUE);
        m_display.Show();

        if (m_wifiManager.autoConnect("Foxie_WiFiSetup")) {
            m_display.DrawTextScrolling("SUCCESS", GREEN);
            m_settings["WIFI"] = "ON";
            m_settings["wifi_configured"] = "1";
        } else {
            m_display.DrawTextScrolling("FAILED", RED);
            m_settings["WIFI"] = "OFF";
            m_settings["wifi_configured"].clear();
            WiFi.disconnect();
            m_isInitialized = false;
        }
        m_settings.Save(true);
    }

    void Initialize() {
        if (!m_isInitialized && m_settings["WIFI"] != "OFF") {
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
            m_display.SetBrightness(30);
            m_display.Clear();
            m_display.DrawText(1, "RECV", DARK_GREEN);
            m_display.ClearRoundLEDs();
            m_display.Show();
        });
        ArduinoOTA.onEnd([&]() {
            m_display.DrawTextScrolling("Do not unplug", GRAY);
            m_display.DrawText(1, "FLSH", ORANGE);
            m_display.Show();
        });
        ArduinoOTA.onProgress([&](unsigned int progress, unsigned int total) {
            m_display.DrawPixel(FIRST_HOUR_LED + map(progress, 0, total, 0, 11),
                                PURPLE);
            m_display.Show();
        });
        ArduinoOTA.onError([&](ota_error_t error) {
            m_display.DrawTextScrolling("OTA ERR:" + String(error), RED);
            ESP.restart();
        });

        ArduinoOTA.setHostname(GetUniqueMDNSName().c_str());
        ArduinoOTA.begin();
        m_isOTAInitialized = true;
    }

    String GetUniqueMDNSName() {
        return "FoxieClock_" + String(WiFi.localIP()[3], DEC);
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