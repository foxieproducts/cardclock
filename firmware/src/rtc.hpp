#pragma once
#include <Rtc_Pcf8563.h>

#include "settings.hpp"

class Rtc {
  private:
    enum {
        PIN_RTC_INTERRUPT = 13,
        TIMER_NUM_SECONDS = 1,  // interrupt every N seconds for time keeping
    };

    Rtc_Pcf8563 m_rtc;
    Settings& m_settings;
    bool m_isInitialized{false};
    size_t m_millisAtInterrupt{0};
    uint8_t m_hour{0}, m_minute{0}, m_second{0};

    inline static bool m_receivedInterrupt{false};  // used by InterruptISR()

  public:
    Rtc(Settings& settings) : m_settings(settings) { m_rtc.getDateTime(); }

    bool IsInitialized() { return m_isInitialized; }

    void Update() {
        if (!m_isInitialized) {
            Initialize();
        }

        // ideally, we get interrupted every 1 second. However, the PCF8563
        // occasionally seems to get confused and send interrupts once a minute,
        // so this forces us to check in with the RTC once a second if it is
        // stuck in that state
        if (m_receivedInterrupt || Millis() >= 1000) {
            m_millisAtInterrupt = millis();
            GetTimeFromRTC();
            m_receivedInterrupt = false;
        }
    }

    int Hour() {
        return m_settings[F("24HR")] == F("ON") ? m_hour : Conv24to12(m_hour);
    }
    int Hour12() { return Conv24to12(m_hour); }
    int Minute() { return m_minute; }
    int Second() { return m_second; }
    int Millis() { return (millis() - m_millisAtInterrupt) % 1000; }
    void SetTime(uint8_t hour, uint8_t minute, uint8_t second) {
        m_rtc.setTime(hour, minute, second);
        m_millisAtInterrupt = millis();
        GetTimeFromRTC();
    }
    int Conv24to12(int hour) {
        if (hour > 12) {
            hour -= 12;
        } else if (hour == 0) {
            hour = 12;
        }
        return hour;
    }

    void SetClockToZero() { m_rtc.zeroClock(); }

  private:
    void Initialize() {
        // try to set the alarm -- if it succeeds, the RTC is booted and we can
        // setup the 1 second timer
        m_rtc.getDateTime();
        m_rtc.setAlarm(1, 2, 3, 4);
        m_rtc.enableAlarm();
        m_rtc.getAlarm();
        if (m_rtc.getAlarmMinute() == 1 && m_rtc.getAlarmHour() == 2 &&
            m_rtc.getAlarmDay() == 3 && m_rtc.getAlarmWeekday() == 4) {
            m_rtc.clearAlarm();

            m_rtc.getDateTime();
            if (m_rtc.getTimerValue() != TIMER_NUM_SECONDS) {
                // fresh boot -- no time backup, timer was not enabled
                m_rtc.zeroClock();
            }

            m_rtc.clearStatus();
            m_rtc.clearTimer();

            // interrupt every second, hopefully
            m_rtc.setTimer(TIMER_NUM_SECONDS, TMR_1Hz, true);

            m_isInitialized = true;
            AttachInterrupt();

            GetTimeFromRTC();
        }
    }

    void GetTimeFromRTC() {
        m_rtc.getDateTime();
        m_hour = m_rtc.getHour();
        m_minute = m_rtc.getMinute();
        m_second = m_rtc.getSecond();
    }

    void AttachInterrupt() {
        pinMode(PIN_RTC_INTERRUPT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PIN_RTC_INTERRUPT), InterruptISR,
                        FALLING);
    }
    static inline void IRAM_ATTR InterruptISR() { m_receivedInterrupt = true; }
};
