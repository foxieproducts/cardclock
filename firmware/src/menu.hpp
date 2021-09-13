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
    String m_title;

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
    std::vector<std::shared_ptr<Menu>> m_menus;
    int m_pos{0};

    Button m_btnUp{PIN_BTN_UP, INPUT_PULLUP};
    Button m_btnDown{PIN_BTN_DOWN, INPUT};
    Button m_btnLeft{PIN_BTN_LEFT, INPUT_PULLUP};
    Button m_btnRight{PIN_BTN_RIGHT, INPUT_PULLUP};

  public:
    MenuManager() {
        m_btnLeft.config.repeatRate = 250;
        m_btnRight.config.repeatRate = 250;

        m_btnUp.config.handlerFunc = [&](const Button::Event_e evt) {
            m_menus[m_pos]->Up(evt);
        };
        m_btnDown.config.handlerFunc = [&](const Button::Event_e evt) {
            m_menus[m_pos]->Down(evt);
        };
        m_btnLeft.config.handlerFunc = [&](const Button::Event_e evt) {
            const bool handled = m_menus[m_pos]->Left(evt);
            if (!handled && (evt == Button::PRESS || evt == Button::REPEAT)) {
                if (m_pos == 0) {
                    return;
                }
                --m_pos;
                m_menus[m_pos]->Begin();
            }
        };
        m_btnRight.config.handlerFunc = [&](const Button::Event_e evt) {
            const bool handled = m_menus[m_pos]->Right(evt);
            if (!handled && (evt == Button::PRESS || evt == Button::REPEAT)) {
                if (m_pos == (int)m_menus.size() - 1) {
                    return;
                }
                m_pos++;
                m_menus[m_pos]->Begin();
            }
        };
    }

    void Add(std::shared_ptr<Menu> menu) {
        m_menus.push_back(menu);
        m_pos = m_menus.size() - 1;
    }

    void Update() {
        Button::Update();
        m_menus[m_pos]->Update();
    }

    void ActivateMenu(const unsigned int menuNum) {
        if (menuNum < m_menus.size()) {
            m_pos = menuNum;
            m_menus[m_pos]->Begin();
        }
    }
};
