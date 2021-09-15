#pragma once
#include "foxie_wifi.hpp"
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

        if (FoxieWiFi::IsConnected()) {
            m_display.DrawPixel(42, DARK_GRAY);
        }
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
        if (IsMinimumBrightness()) {
            color = Display::ScaleBrightness(color, 0.6f);
        }

        m_display.Clear();

        char text[10];
        sprintf(text, "%2d",
                m_settings["HOUR_FMT"] == "24" ? m_rtc.Hour() : m_rtc.Hour12());
        m_display.DrawText(0, text, color);
        sprintf(text, "%02d", m_rtc.Minute());
        m_display.DrawText(10, text, color);
    }

    void DrawSeparator(int color) {
        if (IsMinimumBrightness()) {
            color = Display::ScaleBrightness(color, 0.6f);
        }

        const float ms = m_rtc.Millis() / 1000.0f;
        const int transitionColor = Display::ScaleBrightness(
            color, 0.2f + (m_rtc.Second() % 2 ? ms : 1.0f - ms) * 0.8f);

        m_display.DrawChar(8, ':', transitionColor);
    }

    void DrawAnalog(int color) {
        if (IsMinimumBrightness()) {
            color = Display::ScaleBrightness(color, 0.7f);
        }

        int darkerColor = Display::ScaleBrightness(color, 0.3f);
        m_display.ClearRoundLEDs(DARK_GRAY);

        m_display.DrawSecondLEDs(m_rtc.Second(), darkerColor);
        m_display.DrawHourLED(m_rtc.Hour12(), color);
        m_display.DrawMinuteLED(m_rtc.Minute(), color);
    }

    bool IsMinimumBrightness() { return m_display.GetBrightness() == 0; }
};
