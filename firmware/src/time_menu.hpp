#pragma once
#include "menu.hpp"
#include "rtc.hpp"

class TimeMenu : public Menu {
  private:
    Rtc& m_rtc;
    uint8_t m_colorWheel{0};

    String m_mode{"RTC"};

  public:
    TimeMenu(Display& display, Rtc& rtc) : Menu(display), m_rtc(rtc) {
        m_title = "SET TIME";
    }

    virtual void Update() { m_display.DrawText(0, m_mode, GREEN); }

    virtual bool Up(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            m_display.DrawTextScrolling("Up pressed", GREEN);
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            m_display.DrawTextScrolling("Down pressed", GREEN);
        }
        return true;
    }

  private:
};
