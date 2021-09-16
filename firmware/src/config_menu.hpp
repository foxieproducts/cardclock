#pragma once
#include "elapsed_time.hpp"
#include "foxie_wifi.hpp"
#include "menu.hpp"

class ConfigMenu : public Menu {
  public:
    class Option {
      protected:
        Display& m_display;
        Settings& m_settings;
        String m_name;
        std::vector<String> m_values;
        size_t m_index{0};
        std::function<void()> m_runFuncOnce;
        bool m_isDone{false};

      public:
        Option(Display& display,
               Settings& settings,
               String name,
               std::initializer_list<String> options = {})
            : m_display(display),
              m_settings(settings),
              m_name(name),
              m_values(options) {}

        Option(Display& display,
               Settings& settings,
               String name,
               std::function<void()> runFuncOnce)
            : m_display(display),
              m_settings(settings),
              m_name(name),
              m_runFuncOnce(runFuncOnce) {}

        String& GetName() { return m_name; }
        String& GetCurrentValue() { return m_values[m_index]; }

        virtual void SetValues(std::vector<String> values) {
            m_values = values;
            Begin();
        }

        virtual void Begin() {
            if (m_values.size() && m_values[m_index] != m_settings[m_name]) {
                Reset();
            }

            if (m_runFuncOnce) {
                m_runFuncOnce();
                m_isDone = true;
            }
        }

        virtual void Update() {
            if (m_values.size()) {
                m_display.DrawText(0, m_values[m_index], GRAY);
                DrawArrows();
            }
        }

        virtual void Up() {
            if (m_index < m_values.size() - 1) {
                m_index++;
                m_display.ScrollVertical(HEIGHT, 1);
            }
        }
        virtual void Down() {
            if (m_index > 0) {
                m_index--;
                m_display.ScrollVertical(HEIGHT, -1);
            }
        }

        virtual void Finish() {
            if (m_values.size()) {
                m_settings[GetName()] = GetCurrentValue();
                m_settings.Save();
            }
        }

        virtual bool IsDone() { return m_isDone; }

      protected:
        virtual void Reset() {
            m_index = 0;
            for (size_t i = 0; i < m_values.size(); ++i) {
                if (m_values[i] == m_settings[m_name]) {
                    m_index = i;
                    break;
                }
            }
        }

        virtual void DrawArrows() {
            const int downColor = m_index > 0 ? GREEN : DARK_GREEN;
            const int upColor =
                m_index < m_values.size() - 1 ? GREEN : DARK_GREEN;

            m_display.DrawPixel(15, upColor);
            m_display.DrawPixel(31, upColor);
            m_display.DrawPixel(32, upColor);
            m_display.DrawPixel(33, upColor);

            m_display.DrawPixel(65, downColor);
            m_display.DrawPixel(66, downColor);
            m_display.DrawPixel(67, downColor);
            m_display.DrawPixel(83, downColor);
        }
    };

    std::vector<Option> m_options;
    size_t m_menuOption{0};
    int m_selectedMenuOption{-1};

  public:
    ConfigMenu(Display& display, Settings& settings)
        : Menu(display, settings) {}

    void Add(Option opt) { m_options.push_back(opt); }

    virtual void Update() {
        m_display.Clear();

        if (m_selectedMenuOption >= 0) {
            m_options[m_selectedMenuOption].Update();

            if (m_options[m_selectedMenuOption].IsDone()) {
                m_options[m_selectedMenuOption].Finish();
                m_selectedMenuOption = -1;
            }
        } else {
            ShowCurrentOptionName();
            ShowMenuOptionPositionOnHours();
        }
    }

    void ShowCurrentOptionName() {
        m_display.DrawText(0, m_options[m_menuOption].GetName().substring(0, 4),
                           GRAY);

        m_display.DrawPixel(32, GREEN);
        m_display.DrawPixel(49, GREEN);
        m_display.DrawPixel(50, GREEN);
        m_display.DrawPixel(66, GREEN);
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
                m_options[m_selectedMenuOption].Up();
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
                m_options[m_selectedMenuOption].Down();
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
                m_options[m_selectedMenuOption].Finish();
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
                m_options[m_selectedMenuOption].Begin();
            }
        }
        return true;
    }

  private:
};
