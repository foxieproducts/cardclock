#pragma once
#include "elapsed_time.hpp"
#include "menu.hpp"

class TestMenu : public Menu {
  private:
  public:
    TestMenu(Display& display, Settings& settings) : Menu(display, settings) {
        m_title = "TEST";
    }

    virtual void Update() {
        m_display.Clear(BLACK, true);
        const int brightness = m_display.GetBrightness();

        const int scaled = map(brightness, 0, LightSensor::RANGE, 0, 12);
        m_display.DrawPixel(85 + 11, PURPLE);
        if (scaled > 0) {
            for (int i = 0; i < scaled; ++i) {
                m_display.DrawPixel(85 + i, PURPLE);
            }
        }

        m_display.DrawTextCentered(
            String(map(brightness, 0, LightSensor::RANGE, 0, 100)) + "%",
            WHITE);
    }

    virtual void Begin() {}

    virtual bool Up(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
        }
        return true;
    }

#if 0
    virtual bool Left(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
        }
        return true;
    }

    virtual bool Right(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
        }
        return true;
    }
#endif
  private:
};
