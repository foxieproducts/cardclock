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
        ONE_MINUTE_IN_MS = 60000,
        FIVE_MINUTES_IN_MS = 300000,
    };
    Settings& m_settings;
    Rtc& m_rtc;
    WiFiUDP m_udp;
    NTPClient m_ntpClient{m_udp, "time.nist.gov", 0,  //-18000,
                          int(ONE_MINUTE_IN_MS)};
    ElapsedTime m_sinceLastUpdate;
    bool m_isInitialized{false};

  public:
    FoxieNTP(Settings& settings, Rtc& rtc) : m_settings(settings), m_rtc(rtc) {
        m_ntpClient.begin();
    }

    void Update() {
        if (WiFi.isConnected()) {
            if (m_isInitialized && WiFi.isConnected()) {
                m_ntpClient.update();
                if (m_sinceLastUpdate.Ms() >= ONE_MINUTE_IN_MS) {
                    SetTimeFromNTP();
                }
            } else if (!m_isInitialized && WiFi.isConnected()) {
                m_ntpClient.begin();
                m_isInitialized = true;
                SetTimeFromNTP();
            }
        }
    }

    void SetTimeFromNTP() {
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