#pragma once
#include <functional>  // for std::function
#include <memory>      // for std::shared_ptr

#include "elapsed_time.hpp"
#include "foxie_wifi.hpp"
#include "menu.hpp"
#include "option.hpp"

class ConfigMenu : public Menu {
  protected:
    std::vector<std::shared_ptr<Option>> m_options;
    size_t m_menuOption{0};
    int m_selectedMenuOption{-1};

  public:
    ConfigMenu(Display& display, Settings& settings)
        : Menu(display, settings) {}

    void Add(std::shared_ptr<Option> opt) { m_options.push_back(opt); }
    void AddTextSetting(String name,
                        std::vector<String> values,
                        std::function<void()> finishFunc = nullptr) {
        m_options.push_back(std::make_shared<TextListOption>(
            m_display, m_settings, name, values, finishFunc));
    }
    void AddRunFuncSetting(String name,
                           std::function<void()> runFuncOnce,
                           std::function<void()> finishFunc = nullptr) {
        m_options.push_back(
            std::make_shared<OneShotOption>(name, runFuncOnce, finishFunc));
    }
    void AddRangeSetting(String name,
                         const int min,
                         const int max,
                         std::function<void()> finishFunc = nullptr) {
        m_options.push_back(std::make_shared<RangeOption>(
            m_display, m_settings, name, min, max, finishFunc));
    }

    virtual void Update() {
        m_display.Clear();

        ShowMenuOptionPositionOnHours();

        if (m_selectedMenuOption >= 0) {
            m_options[m_selectedMenuOption]->Update();

            if (m_options[m_selectedMenuOption]->IsDone()) {
                m_options[m_selectedMenuOption]->Finish();
                m_selectedMenuOption = -1;
            }
        } else {
            ShowCurrentOptionName();
        }
    }

    void ShowCurrentOptionName() {
        m_display.DrawText(
            0, m_options[m_menuOption]->GetName().substring(0, 4), GRAY);

        // m_display.DrawPixel(32, GREEN);
        // m_display.DrawPixel(49, GREEN);
        m_display.DrawPixel(50, GREEN);
        // m_display.DrawPixel(66, GREEN);
    }

    void ShowMenuOptionPositionOnHours() {
        m_display.ClearRoundLEDs();
        for (size_t i = 0; i < 12; ++i) {
            if (i < m_options.size()) {
                m_display.DrawHourLED(i + 1, DARK_GREEN);
            } else {
                m_display.DrawHourLED(i + 1, GRAY);
            }
        }
        m_display.DrawHourLED(m_menuOption + 1, GREEN);
    }

    virtual void Begin() { m_display.ScrollHorizontal(WIDTH, -1); }

    virtual bool Up(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_selectedMenuOption >= 0) {
                m_options[m_selectedMenuOption]->Up();
            } else {
                if (m_menuOption-- == 0) {
                    m_menuOption = m_options.size() - 1;
                }

                m_display.ScrollVertical(HEIGHT, 1);
            }
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_selectedMenuOption >= 0) {
                m_options[m_selectedMenuOption]->Down();
            } else {
                if (++m_menuOption == m_options.size()) {
                    m_menuOption = 0;
                }

                m_display.ScrollVertical(HEIGHT, -1);
            }
        }
        return true;
    }

    virtual bool Left(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            m_display.ScrollHorizontal(WIDTH, 1);
            if (m_selectedMenuOption >= 0) {
                m_options[m_selectedMenuOption]->Finish();
                m_selectedMenuOption = -1;
                return true;
            }
        }
        return false;  // exit the settings menu
    }

    virtual bool Right(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            if (m_selectedMenuOption == -1) {
                m_display.ScrollHorizontal(WIDTH, -1);
                m_selectedMenuOption = m_menuOption;
                m_options[m_selectedMenuOption]->Begin();
            }
        }
        return true;
    }

  private:
};
