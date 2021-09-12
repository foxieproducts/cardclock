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

        int color = Display::ColorWheel(m_colorWheel);
        if (m_display.GetBrightness() == 0) {
            color = Display::ScaleBrightness(Display::ColorWheel(m_colorWheel),
                                             0.8f);
        }

        m_display.DrawText(0, text, color);

        sprintf(text, "%02d", m_rtc.Minute());
        m_display.DrawText(10, text, color);

        if (m_rtc.Second() % 2) {
            m_display.DrawChar(8, ':', color);
        }

        m_display.ClearRoundLEDs(m_display.GetBrightness() ? DARK_GRAY : 0);
        m_display.DrawPixel(85 + m_rtc.Hour12() - 1, color);

        m_display.DrawPixel(97 + m_display.GetMinuteLED(m_rtc.Second()), color);

        m_display.DrawPixel(97 + m_display.GetMinuteLED(m_rtc.Minute()), color);
    }

    void DrawAnalog() {}
};
