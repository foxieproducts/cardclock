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
    size_t m_displayedOption{0};
    int m_selectedOption{-1};
    ElapsedTime m_delayBeforeRightArrow;

  public:
    ConfigMenu(Display& display, Settings& settings)
        : Menu(display, settings) {}

    void Add(const std::shared_ptr<Option> option) {
        m_options.push_back(option);
    }

    void AddTextSetting(const String name,
                        const std::vector<String>& values,
                        const std::function<void()> finishFunc = nullptr) {
        m_options.push_back(std::make_shared<TextListOption>(
            m_display, m_settings, name, values, finishFunc));
    }

    void AddRunFuncSetting(const String name,
                           const std::function<void()> runFuncOnce,
                           const std::function<void()> finishFunc = nullptr) {
        m_options.push_back(
            std::make_shared<OneShotOption>(name, runFuncOnce, finishFunc));
    }

    void AddRangeSetting(const String name,
                         const int min,
                         const int max,
                         std::function<void()> finishFunc = nullptr) {
        m_options.push_back(std::make_shared<RangeOption>(
            m_display, m_settings, name, min, max, finishFunc));
    }

    virtual void Update() override {
        m_display.Clear();

        ShowMenuOptionPositionOnHours();

        if (m_selectedOption >= 0) {
            m_options[m_selectedOption]->Update();

            if (m_options[m_selectedOption]->IsDone()) {
                m_options[m_selectedOption]->Finish();
                m_selectedOption = -1;
            }
        } else {
            ShowCurrentOptionName();
        }
    }

    void ShowCurrentOptionName() {
        m_display.DrawText(
            0, m_options[m_displayedOption]->GetName().substring(0, 4), GRAY);

        if (m_delayBeforeRightArrow.Ms() > 1000) {
            m_display.DrawChar(14, CHAR_RIGHT_ARROW, GREEN);
        } else if (m_delayBeforeRightArrow.Ms() > 500) {
            m_display.DrawChar(15, CHAR_RIGHT_ARROW, GREEN);
        }
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
        m_display.DrawHourLED(m_displayedOption + 1, GREEN);
    }

    virtual void Activate() override {
        m_display.ScrollHorizontal(WIDTH, -1);
        m_delayBeforeRightArrow.Reset();
    }

    virtual bool Up(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_selectedOption >= 0) {
                m_options[m_selectedOption]->Up();
            } else {
                if (m_displayedOption-- == 0) {
                    m_displayedOption = m_options.size() - 1;
                }

                m_display.ScrollVertical(HEIGHT, SCROLL_DOWN);
                m_delayBeforeRightArrow.Reset();
            }
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) override {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_selectedOption >= 0) {
                m_options[m_selectedOption]->Down();
            } else {
                if (++m_displayedOption == m_options.size()) {
                    m_displayedOption = 0;
                }

                m_display.ScrollVertical(HEIGHT, SCROLL_UP);
                m_delayBeforeRightArrow.Reset();
            }
        }
        return true;
    }

    virtual bool Left(const Button::Event_e evt) override {
        if (evt == Button::PRESS) {
            m_display.ScrollHorizontal(WIDTH, SCROLL_RIGHT);
            if (m_selectedOption >= 0) {
                m_options[m_selectedOption]->Finish();
                m_selectedOption = -1;
                return true;
            }
        }
        // exit the settings menu, MenuManager treats this as
        // moving to the previous Menu
        return false;
    }

    virtual bool Right(const Button::Event_e evt) override {
        if (evt == Button::PRESS) {
            if (m_selectedOption == -1) {
                m_display.ScrollHorizontal(WIDTH, SCROLL_LEFT);
                m_selectedOption = m_displayedOption;
                m_options[m_selectedOption]->Begin();
            }
        }
        return true;
    }
};
