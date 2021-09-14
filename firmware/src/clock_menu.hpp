#pragma once
#include "menu.hpp"
#include "rtc.hpp"

class ClockMenu : public Menu {
  private:
    Rtc& m_rtc;
    uint8_t m_colorWheel{0};

  public:
    ClockMenu(Display& display, Rtc& rtc, Settings& settings)
        : Menu(display, settings), m_rtc(rtc) {}

    virtual void Update() {
        int color = Display::ColorWheel(m_colorWheel);
        DrawClockDigits(color);
        DrawSeparator(color);
        DrawAnalog(color);
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
    void DrawClockDigits(int color) {
        if (m_display.GetBrightness() == 0) {
            // special case for minimum brightness
            color = Display::ScaleBrightness(color, 0.8f);
        }

        m_display.Clear();

        char text[10];
        sprintf(text, "%2d", m_settings["24HR"] == "12" ? m_rtc.Hour12() : m_rtc.Hour());
        m_display.DrawText(0, text, color);
        sprintf(text, "%02d", m_rtc.Minute());
        m_display.DrawText(10, text, color);
    }

    void DrawSeparator(const int color) {
        const float ms = m_rtc.Millis() / 1000.0f;
        const int transitionColor = Display::ScaleBrightness(
            color, 0.2f + (m_rtc.Second() % 2 ? ms : 1.0f - ms) * 0.8f);

        m_display.DrawChar(8, ':', transitionColor);
    }

    void DrawAnalog(const int color) {
        int darkerColor = Display::ScaleBrightness(color, 0.3f);
        m_display.ClearRoundLEDs(m_display.GetBrightness()
                                     ? Display::ScaleBrightness(WHITE, 0.15f)
                                     : 0);

        m_display.DrawSecondLEDs(m_rtc.Second(), darkerColor);

        m_display.DrawPixel(85 + m_rtc.Hour12() - 1, color);
        m_display.DrawPixel(97 + m_display.GetMinuteLED(m_rtc.Minute()), color);
    }
};
