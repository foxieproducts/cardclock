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
            // special case for minimum brightness
            color = Display::ScaleBrightness(color, 0.8f);
        }

        m_display.DrawText(0, text, color);

        sprintf(text, "%02d", m_rtc.Minute());
        m_display.DrawText(10, text, color);

        DrawSeparator(color);

        DrawAnalog(color);
    }

    void DrawSeparator(const int color) {
        const float ms = m_rtc.Millis() / 1000.0f;
        const int fadedColor = Display::ScaleBrightness(
            color, 0.3f + (m_rtc.Second() % 2 ? ms : 1.0f - ms) * 0.7f);
        m_display.DrawChar(8, ':', fadedColor);
    }

    void DrawAnalog(const int color) {
        int halfColor = Display::ScaleBrightness(color, 0.5f);
        m_display.ClearRoundLEDs(m_display.GetBrightness() ? DARK_GRAY : 0);

        m_display.DrawSecondLEDs(m_rtc.Second(), halfColor);

        m_display.DrawPixel(85 + m_rtc.Hour12() - 1, color);
        m_display.DrawPixel(97 + m_display.GetMinuteLED(m_rtc.Minute()), color);
    }
};
