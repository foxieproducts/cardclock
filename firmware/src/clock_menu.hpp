#pragma once
#include "menu.hpp"
#include "rtc.hpp"

class ClockMenu : public Menu {
  private:
    Rtc& m_rtc;
    uint8_t m_colorWheel{0};

  public:
    ClockMenu(Display& display, Rtc& rtc) : Menu(display), m_rtc(rtc) {
        // m_title = "CLOCK";
    }

    virtual void Update() {
        DrawAnalog();
        DrawClockDigits();
    }

    virtual bool Up(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            m_colorWheel -= 4;
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            m_colorWheel += 4;
        }
        return true;
    }

  private:
    void DrawClockDigits() {
        m_display.Clear();

        char text[10];
        sprintf(text, "%2d", m_rtc.Hour12());
        m_display.DrawText(0, text, Display::ColorWheel(m_colorWheel));

        sprintf(text, "%02d", m_rtc.Minute());
        m_display.DrawText(10, text, Display::ColorWheel(m_colorWheel));

        if (m_rtc.Second() % 2) {
            m_display.DrawChar(8, ':', Display::ColorWheel(m_colorWheel));
        }

        m_display.ClearRoundLEDs(DARK_GRAY);
        m_display.DrawPixel(85 + m_rtc.Hour12() - 1,
                            Display::ColorWheel(m_colorWheel));

        m_display.DrawPixel(
            97 + m_display.GetMinuteLED(m_rtc.Second()),
            Display::ScaleBrightness(Display::ColorWheel(m_colorWheel), 0.5f));

        m_display.DrawPixel(97 + m_display.GetMinuteLED(m_rtc.Minute()),
                            Display::ColorWheel(m_colorWheel));
    }

    void DrawAnalog() {}
};
