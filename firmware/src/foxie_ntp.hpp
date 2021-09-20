#pragma once
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "elapsed_time.hpp"
#include "rtc.hpp"
#include "settings.hpp"

class FoxieNTP {
  private:
    enum {
        ONE_SECOND = 1000,
        ONE_MINUTE = 60000,
        WAIT_TO_INITIALIZE = 4000,
        UTC_ZERO = 0,
    };

    Settings& m_settings;
    Rtc& m_rtc;
    WiFiUDP m_udp;
    NTPClient m_ntpClient{m_udp, "time.nist.gov", UTC_ZERO, int(ONE_MINUTE)};

    ElapsedTime m_sinceLastUpdate;

    bool m_isInitialized{false};
    bool m_ntpSynced{false};
    size_t m_updateRTCInterval{ONE_SECOND};

  public:
    FoxieNTP(Settings& settings, Rtc& rtc) : m_settings(settings), m_rtc(rtc) {}

    void Update() {
        if (WiFi.isConnected()) {
            if (!m_isInitialized) {
                m_ntpClient.begin();
                m_isInitialized = true;
            } else {
                if (m_ntpSynced) {
                    m_ntpClient.update();
                } else if (m_sinceLastUpdate.Ms() > WAIT_TO_INITIALIZE) {
                    // check every few seconds until our first sync,
                    // then sync once per minute
                    m_sinceLastUpdate.Reset();
                    if (m_ntpClient.update()) {
                        m_ntpSynced = true;
                        m_updateRTCInterval = ONE_MINUTE;
                        UpdateRTCTime();
                    }
                }
            }

            if (m_sinceLastUpdate.Ms() >= m_updateRTCInterval &&
                m_ntpSynced == true) {
                m_sinceLastUpdate.Reset();
                UpdateRTCTime();
            }
        } else {
            m_sinceLastUpdate.Reset();
        }
    }

    void UpdateRTCTime() {
        const int offset = m_settings.containsKey("UTC")
                               ? m_settings["UTC"].as<int>()
                               : UTC_ZERO;

        m_ntpClient.setTimeOffset(offset * 60 * 60);

        if (m_ntpClient.getHours() != m_rtc.Hour() ||
            m_ntpClient.getMinutes() != m_rtc.Minute() ||
            m_ntpClient.getSeconds() != m_rtc.Second()) {
            m_rtc.SetTime(m_ntpClient.getHours(), m_ntpClient.getMinutes(),
                          m_ntpClient.getSeconds());
        }
    }
};