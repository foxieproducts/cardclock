#pragma once
#include <Rtc_Pcf8563.h>

class Rtc {
  private:
    enum {
        PIN_RTC_INTERRUPT = 13,
    };

    Rtc_Pcf8563 m_rtc;
    inline static bool m_isInitialized{false};
    inline static bool m_receivedInterrupt{false};
    inline static int m_millisAtInterrupt{0};

  public:
    Rtc() { m_rtc.clearTimer(); }

    bool IsInitialized() { return m_isInitialized; }

    Rtc_Pcf8563& Get() { return m_rtc; }

    void Update() {
        if (!m_isInitialized) {
            Initialize();
        }

        if (m_receivedInterrupt) {
            m_rtc.getDateTime();
            m_receivedInterrupt = false;
        }
    }

    int Hour() { return m_rtc.getHour(); }
    int Hour12() { return Get12(m_rtc.getHour()); }
    int Minute() { return m_rtc.getMinute(); }
    int Second() { return m_rtc.getSecond(); }
    int Millis() { return millis() - m_millisAtInterrupt; }
    void SetTime(byte hour, byte minute, byte second) {
        m_rtc.setTime(hour, minute, second);
    }

    int Get12(int hour) {
        if (hour > 12) {
            hour -= 12;
        } else if (hour == 0) {
            hour = 12;
        }
        return hour;
    }

  private:
    void Initialize() {
        // try to set the alarm, when it succeeds, we're booted
        // and can setup our 1 second timer

        m_rtc.getDateTime();
        m_rtc.setAlarm(1, 2, 3, 4);
        m_rtc.enableAlarm();
        m_rtc.getAlarm();
        if (m_rtc.getAlarmMinute() == 1 && m_rtc.getAlarmHour() == 2 &&
            m_rtc.getAlarmDay() == 3 && m_rtc.getAlarmWeekday() == 4) {
            m_rtc.clearAlarm();

            m_rtc.getDateTime();
            if (m_rtc.getTimerValue() != 1) {
                // fresh boot, no time backup, timer was not enabled
                m_rtc.zeroClock();
            }
            // interrupt every second
            m_rtc.setTimer(1, TMR_1Hz, true);

            m_isInitialized = true;
            AttachInterrupt();
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
