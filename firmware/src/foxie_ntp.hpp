#pragma once
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "elapsed_time.hpp"
#include "rtc.hpp"
#include "settings.hpp"

class FoxieNTP {
  private:
    enum {
        ONE_MINUTE = 60000,
    };
    Settings& m_settings;
    Rtc& m_rtc;
    WiFiUDP m_udp;
    NTPClient m_ntpClient{m_udp, "time.nist.gov", 0, int(ONE_MINUTE)};
    ElapsedTime m_sinceLastUpdate;
    bool m_isInitialized{false};

  public:
    FoxieNTP(Settings& settings, Rtc& rtc) : m_settings(settings), m_rtc(rtc) {}

    void Update() {
        if (WiFi.isConnected()) {
            if (m_isInitialized && WiFi.isConnected()) {
                m_ntpClient.update();
                if (m_sinceLastUpdate.Ms() >= ONE_MINUTE) {
                    UpdateRTCTime();
                }
            } else if (!m_isInitialized && WiFi.isConnected()) {
                m_ntpClient.begin();
                m_ntpClient.setTimeOffset(m_settings["UTC"].as<int>() * 60 *
                                          60);
                m_isInitialized = true;
                UpdateRTCTime();
            }
        }
    }

    void UpdateRTCTime() {
        const int offset = m_settings["UTC"] ? m_settings["UTC"].as<int>() : 0;
        m_ntpClient.setTimeOffset(offset * 60 * 60);
        m_ntpClient.update();
        if (m_ntpClient.getHours() != m_rtc.Hour() ||
            m_ntpClient.getMinutes() != m_rtc.Minute() ||
            m_ntpClient.getSeconds() != m_rtc.Second()) {
            m_rtc.SetTime(m_ntpClient.getHours(), m_ntpClient.getMinutes(),
                          m_ntpClient.getSeconds());
            m_sinceLastUpdate.Reset();
            // m_ntpClient.
        }
    }
};