#pragma once
#include <Arduino.h>

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
        }

        if (IsConnected()) {
            m_display.DrawPixel(0, BLUE);
        }

        m_wifiManager.loop();
    }

    bool IsConfigured() { return !WiFi.SSID().isEmpty(); }

    bool IsConnected() { return IsConfigured() && WiFi.isConnected(); }

  private:
    void Configure() {
        // TODO: Make sure config portal isn't open when calling this
        Initialize();
        m_wifiManager.resetSettings();
        WiFi.persistent(true);
        m_wifiManager.setConfigPortalTimeout(180);
        m_display.DrawTextScrolling("Connect to Foxie_WiFiSetup", GRAY);
        m_display.Clear();
        m_display.DrawText(1, "<(I)>", BLUE);
        m_display.Update();

        m_settings["WIFI"] = "OFF";
        m_settings["wifi_configured"].clear();
        m_settings.Save(true);

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
};