#pragma once
#include <Arduino.h>
#include <vector>
#include "display.hpp"
#include "settings.hpp"

class Option {
  protected:
    String m_name;
    bool m_isDone{false};

  public:
    Option(String name) : m_name(name) {}
    virtual String GetName() { return m_name; }
    virtual String GetCurrentValue() { return ""; };
    virtual void Begin(){};
    virtual void Update() {}
    virtual void Up() {}
    virtual void Down() {}
    virtual void Finish() {}

    virtual bool IsDone() { return m_isDone; }
};

class TextListOption : public Option {
  private:
    Display& m_display;
    Settings& m_settings;
    std::vector<String> m_values;
    size_t m_index{0};

  public:
    TextListOption(Display& display,
                   Settings& settings,
                   String name,
                   std::vector<String> values)
        : Option(name),
          m_display(display),
          m_settings(settings),
          m_values(values) {}

    virtual String GetCurrentValue() override { return m_values[m_index]; }

    virtual void Begin() override {
        if (m_values[m_index] != m_settings[m_name]) {
            // when we come back into the menu, make sure that the setting
            // selected is what is actually set in m_settings
            m_index = 0;
            for (size_t i = 0; i < m_values.size(); ++i) {
                if (m_values[i] == m_settings[m_name]) {
                    m_index = i;
                    break;
                }
            }
        }
    }

    virtual void Update() override {
        m_display.DrawText(0, m_values[m_index], GRAY);
        DrawArrows();
    }

    virtual void Up() override {
        if (m_index < m_values.size() - 1) {
            m_index++;
            m_display.ScrollVertical(HEIGHT, 1);
        }
    }
    virtual void Down() override {
        if (m_index > 0) {
            m_index--;
            m_display.ScrollVertical(HEIGHT, -1);
        }
    }

    virtual void Finish() override {
        m_settings[GetName()] = GetCurrentValue();
        m_settings.Save();
    }

  private:
    virtual void DrawArrows() {
        const int downColor = m_index > 0 ? GREEN : DARK_GREEN;
        const int upColor = m_index < m_values.size() - 1 ? GREEN : DARK_GREEN;

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

class OneShotOption : public Option {
  private:
    std::function<void()> m_runFuncOnce;

  public:
    OneShotOption(String name, std::function<void()> runFuncOnce)
        : Option(name), m_runFuncOnce(runFuncOnce) {}

    virtual void Begin() override {
        m_runFuncOnce();
        m_isDone = true;
    }
};