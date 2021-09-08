#pragma once
#include <Arduino.h>
#include <memory>
#include <vector>

#include "button.hpp"
#include "display.hpp"

class Menu {
  protected:
    Display& m_display;
    String m_title;

  public:
    Menu(Display& display) : m_display(display) {}

    virtual void ShowTitle(const int color) {
        m_display.DrawTextScrolling(m_title, color);
    }

    virtual void Update() = 0;
    virtual bool Up(const Button::Event_e evt) { return false; }
    virtual bool Down(const Button::Event_e evt) { return false; }
    virtual bool Left(const Button::Event_e evt) { return false; }
    virtual bool Right(const Button::Event_e evt) { return false; }
};

class MenuManager {
  private:
    Display& m_display;
    std::vector<std::shared_ptr<Menu>> m_menus;
    int m_pos{0};

    enum Pins_e {
        PIN_BTN_UP = 10,
        PIN_BTN_DOWN = 0,  // has external pull-up resistor
        PIN_BTN_LEFT = 12,
        PIN_BTN_RIGHT = 14,
    };

    Button m_btnUp{PIN_BTN_UP, INPUT_PULLUP};
    Button m_btnDown{PIN_BTN_DOWN, INPUT};
    Button m_btnLeft{PIN_BTN_LEFT, INPUT_PULLUP};
    Button m_btnRight{PIN_BTN_RIGHT, INPUT_PULLUP};

  public:
    MenuManager(Display& display) : m_display(display) {
        m_btnUp.config.canRepeat = true;
        m_btnDown.config.canRepeat = true;
        m_btnLeft.config.canRepeat = true;
        m_btnRight.config.canRepeat = true;

        m_btnUp.config.handlerFunc = [&](const Button::Event_e evt) {
            m_menus[m_pos]->Up(evt);
        };
        m_btnDown.config.handlerFunc = [&](const Button::Event_e evt) {
            m_menus[m_pos]->Down(evt);
        };
        m_btnLeft.config.handlerFunc = [&](const Button::Event_e evt) {
            if (!m_menus[m_pos]->Left(evt) && evt == Button::PRESS) {
                if (m_pos-- == 0) {
                    m_pos = m_menus.size() - 1;
                }
                m_menus[m_pos]->ShowTitle(PURPLE);
            }
        };
        m_btnRight.config.handlerFunc = [&](const Button::Event_e evt) {
            if (!m_menus[m_pos]->Right(evt) && evt == Button::PRESS) {
                if (++m_pos == (int)m_menus.size()) {
                    m_pos = 0;
                }
                m_menus[m_pos]->ShowTitle(BLUE);
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
        m_display.SetBrightness(m_display.GetBrightness());
        m_display.Show();
    }
};
