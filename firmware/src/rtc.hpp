#pragma once
#include <Rtc_Pcf8563.h>

class Rtc {
  private:
    enum {
        PIN_RTC_INTERRUPT = 13,
    };

    Rtc_Pcf8563 m_rtc;
    bool m_isInitialized{false};
    inline static bool m_receivedInterrupt{false};
    inline static int m_millisAtInterrupt{0};

  public:
    Rtc() { AttachInterrupt(); }

    // Rtc_Pcf8563& Get() { return m_rtc; }

    bool IsInitialized() { return m_isInitialized; }

    void Update(bool force = false) {
        if (!m_isInitialized) {
            Initialize();
        }

        if (m_receivedInterrupt || force) {
            m_rtc.getDateTime();
            m_receivedInterrupt = false;
        }
    }

    int Hour() { return m_rtc.getHour(); }
    int Minute() { return m_rtc.getMinute(); }
    int Second() { return m_rtc.getSecond(); }
    int Millis() { return millis() - m_millisAtInterrupt; }

  private:
    void Initialize() {
        if (!m_rtc.timerEnabled()) {
            // interrupt every second
            m_rtc.setTimer(1, TMR_1Hz, true);
        } else {
            m_isInitialized = true;
        }
    }

    void AttachInterrupt() {
        pinMode(PIN_RTC_INTERRUPT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PIN_RTC_INTERRUPT), InterruptISR,
                        FALLING);
    }
    static inline void IRAM_ATTR InterruptISR() {
        m_millisAtInterrupt = millis();
        m_receivedInterrupt = true;
    }
};