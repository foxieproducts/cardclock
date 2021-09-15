#pragma once
#include "elapsed_time.hpp"
#include "menu.hpp"

class ConfigMenu : public Menu {
  private:
    class Option {
      private:
        Display& m_display;
        Settings& m_settings;
        String m_name;
        std::vector<String> m_values;
        size_t m_index{0};

      public:
        Option(Display& display,
               Settings& settings,
               String name,
               std::initializer_list<String> options)
            : m_display(display),
              m_settings(settings),
              m_name(name),
              m_values(options) {
            Begin();
        }

        String& GetName() { return m_name; }
        String& GetCurrentValue() { return m_values[m_index]; }

        void Begin() {
            if (m_values[m_index] != m_settings[m_name]) {
                Reset();
            }
        }

        void Update() {
            m_display.DrawText(0, m_values[m_index], GRAY);
            DrawArrows();
        }

        void Up() {
            if (m_index < m_values.size() - 1) {
                m_index++;
                m_display.ScrollVertical(HEIGHT, 1);
            }
        }
        void Down() {
            if (m_index > 0) {
                m_index--;
                m_display.ScrollVertical(HEIGHT, -1);
            }
        }

      protected:
        void Reset() {
            for (size_t i = 0; i < m_values.size(); ++i) {
                if (m_values[i] == m_settings[m_name]) {
                    m_index = i;
                    break;
                }
            }
        }

        void DrawArrows() {
            int upColor = m_index > 0 ? GREEN : DARK_GREEN;
            int downColor = m_index < m_values.size() - 1 ? GREEN : DARK_GREEN;

            m_display.DrawPixel(15, downColor);
            m_display.DrawPixel(31, downColor);
            m_display.DrawPixel(32, downColor);
            m_display.DrawPixel(33, downColor);

            m_display.DrawPixel(65, upColor);
            m_display.DrawPixel(66, upColor);
            m_display.DrawPixel(67, upColor);
            m_display.DrawPixel(83, upColor);
        }
    };

    std::vector<Option> m_options;
    Option* m_curOption{nullptr};
    size_t m_optionNum{0};

  public:
    ConfigMenu(Display& display, Settings& settings) : Menu(display, settings) {
        // TODO: move these out to main.cpp where the menu is initialized
    }

    void Add(Option opt) { m_options.push_back(opt); }

    virtual void Update() {
        m_display.Clear(BLACK, true);

        if (m_curOption) {
            m_curOption->Update();
        } else {
            m_display.DrawText(
                0, m_options[m_optionNum].GetName().substring(0, 4), GRAY);

            m_display.DrawPixel(32, GREEN);
            m_display.DrawPixel(49, GREEN);
            m_display.DrawPixel(50, GREEN);
            m_display.DrawPixel(66, GREEN);
        }
    }

    virtual void Begin() { m_display.ScrollHorizontal(WIDTH, -1); }

    virtual bool Up(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_curOption) {
                m_curOption->Up();
            } else {
                if (++m_optionNum == m_options.size()) {
                    m_optionNum = 0;
                }
                m_display.ScrollVertical(HEIGHT, 1);
            }
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            if (m_curOption) {
                m_curOption->Down();
            } else {
                if (m_optionNum-- == 0) {
                    m_optionNum = m_options.size() - 1;
                }
                m_display.ScrollVertical(HEIGHT, -1);
            }
        }
        return true;
    }

    virtual bool Left(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            m_display.ScrollHorizontal(WIDTH, 1);
            if (m_curOption) {
                m_settings[m_curOption->GetName()] =
                    m_curOption->GetCurrentValue();

                m_curOption = nullptr;
                m_settings.Save();
                return true;
            }
        }
        return false;  // exit the settings menu
    }

    virtual bool Right(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            if (!m_curOption) {
                m_display.ScrollHorizontal(WIDTH, -1);
                m_curOption = &m_options[m_optionNum];
                m_curOption->Begin();
            }
        }
        return true;
    }

  private:
};
