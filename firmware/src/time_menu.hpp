#pragma once
#include "elapsed_time.hpp"
#include "menu.hpp"
#include "rtc.hpp"

class TimeMenu : public Menu {
  private:
    enum Mode_e {
        SET_SECOND,
        SET_MINUTE,
        SET_HOUR,
    };

    Rtc& m_rtc;
    Mode_e m_mode{SET_MINUTE};
    int m_hour, m_minute, m_second;
    bool m_timeChanged{false};
    bool m_secondsChanged{false};

  public:
    TimeMenu(Display& display, Rtc& rtc, Settings& settings)
        : Menu(display, settings), m_rtc(rtc) {}

    virtual void Update() {
        m_display.Clear();

        if (!m_timeChanged) {
            m_hour = m_rtc.Hour();
            m_minute = m_rtc.Minute();
        }

        if (!m_secondsChanged) {
            m_second = m_rtc.Second();
        }

        DrawClockDigits();
    }

    virtual void Begin() {
        m_mode = SET_HOUR;
        m_timeChanged = false;
        m_secondsChanged = false;
    }

    virtual bool Up(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            m_timeChanged = true;
            switch (m_mode) {
                case SET_HOUR:
                    if (++m_hour > 23) {
                        m_hour = 0;
                    }
                    break;
                case SET_MINUTE:
                    if (++m_minute > 59) {
                        m_minute = 0;
                    }
                    break;
                case SET_SECOND:
                    m_secondsChanged = true;
                    if (++m_second > 59) {
                        m_second = 0;
                    }
                    break;
            }
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            m_timeChanged = true;
            switch (m_mode) {
                case SET_HOUR:
                    if (m_hour-- == 0) {
                        m_hour = 23;
                    }
                    break;
                case SET_MINUTE:
                    if (m_minute-- == 0) {
                        m_minute = 59;
                    }
                    break;
                case SET_SECOND:
                    m_secondsChanged = true;
                    if (m_second-- == 0) {
                        m_second = 59;
                    }
                    break;
            }
        }
        return true;
    }

    virtual bool Left(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_mode == SET_HOUR) {
                SetRTCIfTimeChanged();
                // exit by allowing MenuManager to handle button press
                return false;
            } else if (m_mode == SET_MINUTE) {
                m_mode = SET_HOUR;
            } else if (m_mode == SET_SECOND) {
                m_display.ScrollHorizontal(9, 1);
                m_mode = SET_MINUTE;
            }
        }
        return true;
    }

    virtual bool Right(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_mode == SET_HOUR) {
                m_mode = SET_MINUTE;
            } else if (m_mode == SET_MINUTE) {
                m_display.ScrollHorizontal(9, -1);
                m_mode = SET_SECOND;
            } else if (m_mode == SET_SECOND) {
                m_display.ScrollHorizontal(9, 1);
                DrawClockDigits();
                SetRTCIfTimeChanged();
                // exit by allowing MenuManager to handle button press
                return false;
            }
        }
        return true;
    }

  private:
    void SetRTCIfTimeChanged() {
        if (m_timeChanged) {
            m_rtc.SetTime(m_hour, m_minute,
                          m_secondsChanged ? m_second : m_rtc.Second());
        }
    }

    void DrawClockDigits() {
        m_display.Clear(BLACK);

        int color = m_rtc.Millis() < 500 ? GREEN : 0x00AF00;

        char text[10];
        if (m_mode == SET_SECOND) {
            sprintf(text, "%02d", m_minute);
            m_display.DrawText(0, text, m_mode == SET_MINUTE ? color : GRAY);

            sprintf(text, "%02d", m_second);
            m_display.DrawText(10, text, m_mode == SET_SECOND ? color : GRAY);
        } else {
            sprintf(text, "%2d",
                    m_settings[F("24HR")] == F("ON")
                        ? m_hour
                        : m_rtc.Conv24to12(m_hour));
            m_display.DrawText(0, text, m_mode == SET_HOUR ? color : GRAY);

            sprintf(text, "%02d", m_minute);
            m_display.DrawText(10, text, m_mode == SET_MINUTE ? color : GRAY);
        }

        m_display.DrawChar(8, ':', GRAY);

        m_display.ClearRoundLEDs(DARK_GRAY);

        m_display.DrawSecondLEDs(m_second,
                                 m_mode == SET_SECOND ? color : WHITE);
        m_display.DrawHourLED(m_rtc.Conv24to12(m_hour),
                              m_mode == SET_HOUR ? color : WHITE);
        m_display.DrawMinuteLED(m_minute, m_mode == SET_MINUTE ? color : WHITE);
    }
};
