#pragma once
#include "menu.hpp"
#include "rtc.hpp"

class ClockMenu : public Menu {
  private:
    Rtc m_rtc;
    uint8_t m_colorWheel{0};

  public:
    ClockMenu(Display& display) : Menu(display) {}

    virtual void Update() {
        m_rtc.Update();

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
        char text[10];
        sprintf(text, "%2d:%02d", m_rtc.Hour(), m_rtc.Minute());
        m_display.Clear();
        m_display.DrawText(0, text, Display::ColorWheel(m_colorWheel));
    }
};
