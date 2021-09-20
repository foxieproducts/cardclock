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
            m_rtc.getDateTime();
            m_receivedInterrupt = false;
            m_millisAtInterrupt = millis();
        }
    }

    int Hour() {
        return m_settings[F("24HR")] == F("ON") ? m_rtc.getHour()
                                                : Conv24to12(m_rtc.getHour());
    }
    int Hour12() { return Conv24to12(m_rtc.getHour()); }
    int Minute() { return m_rtc.getMinute(); }
    int Second() { return m_rtc.getSecond(); }
    int Millis() { return millis() - m_millisAtInterrupt; }
    void SetTime(uint8_t hour, uint8_t minute, uint8_t second) {
        m_rtc.setTime(hour, minute, second);
        m_millisAtInterrupt = millis();
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
        }
    }

    void AttachInterrupt() {
        pinMode(PIN_RTC_INTERRUPT, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PIN_RTC_INTERRUPT), InterruptISR,
                        FALLING);
    }
    static inline void IRAM_ATTR InterruptISR() { m_receivedInterrupt = true; }
};
