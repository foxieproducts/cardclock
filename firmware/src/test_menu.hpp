#pragma once
#include "elapsed_time.hpp"
#include "menu.hpp"
#include "rtc.hpp"

class TestMenu : public Menu {
  private:
  public:
    TestMenu(Display& display) : Menu(display) {}

    virtual void Update() {
        m_display.Clear(BLACK, true);
        const int brightness = m_display.GetBrightness();

        const int scaled = map(brightness, LightSensor::MIN_BRIGHTNESS,
                               LightSensor::MAX_BRIGHTNESS, 0, 12);
        m_display.DrawPixel(85 + 11, PURPLE);
        if (scaled > 0) {
            for (int i = 0; i < scaled; ++i) {
                m_display.DrawPixel(85 + i, PURPLE);
            }
        }

        m_display.DrawText(0,
                           String(map(brightness, LightSensor::MIN_BRIGHTNESS,
                                      LightSensor::MAX_BRIGHTNESS, 0, 100)) +
                               "%",
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
