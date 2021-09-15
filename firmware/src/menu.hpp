#pragma once
#include <Arduino.h>
#include <memory>
#include <vector>

#include "button.hpp"
#include "display.hpp"
#include "settings.hpp"

class Menu {
  protected:
    Display& m_display;
    Settings& m_settings;

  public:
    Menu(Display& display, Settings& settings)
        : m_display(display), m_settings(settings) {}

    virtual void Update() = 0;
    virtual void Begin() {}
    virtual bool Up(const Button::Event_e evt) { return false; }
    virtual bool Down(const Button::Event_e evt) { return false; }
    virtual bool Left(const Button::Event_e evt) { return false; }
    virtual bool Right(const Button::Event_e evt) { return false; }
};

class MenuManager {
  private:
    Display& m_display;
    Settings& m_settings;

    std::vector<std::shared_ptr<Menu>> m_menus;
    int m_activeMenu{0};

    Button m_btnUp{PIN_BTN_UP, INPUT_PULLUP};
    Button m_btnDown{PIN_BTN_DOWN, INPUT};
    Button m_btnLeft{PIN_BTN_LEFT, INPUT_PULLUP};
    Button m_btnRight{PIN_BTN_RIGHT, INPUT_PULLUP};

  public:
    MenuManager(Display& display, Settings& settings)
        : m_display(display), m_settings(settings) {
        m_btnLeft.config.repeatRate = 250;
        m_btnRight.config.repeatRate = 250;
        m_btnLeft.config.canRepeat = false;
        m_btnRight.config.canRepeat = false;

        m_btnUp.config.handlerFunc = [&](const Button::Event_e evt) {
            m_menus[m_activeMenu]->Up(evt);
        };
        m_btnDown.config.handlerFunc = [&](const Button::Event_e evt) {
            m_menus[m_activeMenu]->Down(evt);
        };
        m_btnLeft.config.handlerFunc = [&](const Button::Event_e evt) {
            const bool handled = m_menus[m_activeMenu]->Left(evt);
            if (!handled && (evt == Button::PRESS || evt == Button::REPEAT)) {
                if (m_activeMenu == 0) {
                    return;
                }
                --m_activeMenu;
                m_menus[m_activeMenu]->Begin();
            }
        };
        m_btnRight.config.handlerFunc = [&](const Button::Event_e evt) {
            const bool handled = m_menus[m_activeMenu]->Right(evt);
            if (!handled && (evt == Button::PRESS || evt == Button::REPEAT)) {
                if (m_activeMenu == (int)m_menus.size() - 1) {
                    return;
                }
                m_activeMenu++;
                m_menus[m_activeMenu]->Begin();
            }
        };
    }

    void Add(std::shared_ptr<Menu> menu) {
        m_menus.push_back(menu);
        m_activeMenu = m_menus.size() - 1;
    }

    size_t GetActive() { return m_activeMenu; }

    void Update() {
        Button::Update();
        m_menus[m_activeMenu]->Update();
    }

    void ActivateMenu(size_t menuNum) {
        if (menuNum < m_menus.size()) {
            m_activeMenu = menuNum;
            m_menus[m_activeMenu]->Begin();
        }
    }
};
