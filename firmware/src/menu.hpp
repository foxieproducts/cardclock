#pragma once
#include <Arduino.h>
#include <memory>  // for std::shared_ptr
#include <vector>  // for std::vector

#include "button.hpp"
#include "display.hpp"
#include "elapsed_time.hpp"
#include "settings.hpp"

class Menu {
  protected:
    Display& m_display;
    Settings& m_settings;
    ElapsedTime m_timeSinceButtonPress;

  public:
    Menu(Display& display, Settings& settings)
        : m_display(display), m_settings(settings) {}

    void ResetTimeSinceButtonPress() { m_timeSinceButtonPress.Reset(); }
    size_t GetTimeSinceButtonPress() { return m_timeSinceButtonPress.Ms(); }

    virtual void Update() = 0;
    virtual void Activate() {}  // called when menu becomes active
    virtual void Hide() {}      // called when menu becomes inactive
    virtual bool Up(const Button::Event_e evt) { return false; }
    virtual bool Down(const Button::Event_e evt) { return false; }
    virtual bool Left(const Button::Event_e evt) { return false; }
    virtual bool Right(const Button::Event_e evt) { return false; }
    virtual bool ShouldTimeout() { return true; }
    virtual void Timeout() {}
};

class MenuManager {
  private:
    enum {
        MENU_TIMEOUT_MS = 5000,
    };

    Display& m_display;
    Settings& m_settings;

    std::vector<std::shared_ptr<Menu>> m_menus;
    size_t m_activeMenu{0}, m_defaultMenu{0};

    Button m_btnUp{PIN_BTN_UP, INPUT_PULLUP};
    Button m_btnDown{PIN_BTN_DOWN, INPUT};
    Button m_btnLeft{PIN_BTN_LEFT, INPUT_PULLUP};
    Button m_btnRight{PIN_BTN_RIGHT, INPUT_PULLUP};

  public:
    MenuManager(Display& display, Settings& settings)
        : m_display(display), m_settings(settings) {
        ConfigureButtons();
    }

    void Add(std::shared_ptr<Menu> menu) {
        m_menus.push_back(menu);
        m_activeMenu = m_menus.size() - 1;
    }

    size_t GetActive() { return m_activeMenu; }

    void Update() {
        m_btnUp.Update();
        m_btnDown.Update();
        m_btnLeft.Update();
        m_btnRight.Update();
        m_menus[m_activeMenu]->Update();
        if (m_menus[m_activeMenu]->ShouldTimeout() &&
            m_menus[m_activeMenu]->GetTimeSinceButtonPress() >
                MENU_TIMEOUT_MS) {
            m_menus[m_activeMenu]->Timeout();
            ActivateMenu(m_defaultMenu);
        }
    }

    void SetDefaultAndActivateMenu(const size_t menuNum) {
        m_defaultMenu = menuNum;
        ActivateMenu(menuNum);
    }

    void ActivateMenu(const size_t menuNum) {
        if (menuNum < m_menus.size()) {
            m_menus[m_activeMenu]->Hide();
            m_activeMenu = menuNum;
            m_menus[m_activeMenu]->Activate();
            m_menus[m_activeMenu]->ResetTimeSinceButtonPress();
        }
    }

  private:
    void ConfigureButtons() {
        m_btnLeft.config.repeatRate = 250;
        m_btnRight.config.repeatRate = 250;
        m_btnLeft.config.canRepeat = false;
        m_btnRight.config.canRepeat = false;

        m_btnUp.config.handlerFunc = [&](const Button::Event_e evt) {
            m_menus[m_activeMenu]->Up(evt);
            m_menus[m_activeMenu]->ResetTimeSinceButtonPress();
        };

        m_btnDown.config.handlerFunc = [&](const Button::Event_e evt) {
            m_menus[m_activeMenu]->Down(evt);
            m_menus[m_activeMenu]->ResetTimeSinceButtonPress();
        };

        m_btnLeft.config.handlerFunc = [&](const Button::Event_e evt) {
            const bool handled = m_menus[m_activeMenu]->Left(evt);
            if (!handled && (evt == Button::PRESS || evt == Button::REPEAT)) {
                if (m_activeMenu == 0) {
                    return;
                }
                ActivateMenu(m_activeMenu - 1);
            }
        };

        m_btnRight.config.handlerFunc = [&](const Button::Event_e evt) {
            const bool handled = m_menus[m_activeMenu]->Right(evt);
            if (!handled && (evt == Button::PRESS || evt == Button::REPEAT)) {
                if (m_activeMenu == m_menus.size() - 1) {
                    return;
                }
                ActivateMenu(m_activeMenu + 1);
            }
        };
    }
};
