#pragma once
#include "elapsed_time.hpp"
#include "menu.hpp"

class SettingsMenu : public Menu {
  private:
    int m_settingNum{0};

  public:
    SettingsMenu(Display& display, Settings& settings)
        : Menu(display, settings) {
        m_title = "SETTINGS";
    }

    virtual void Update() {
        m_display.Clear(BLACK, true);

        m_display.DrawText(0, "hi", PURPLE);
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
