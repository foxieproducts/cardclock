#pragma once
#include <Arduino.h>
#include <vector>
#include "display.hpp"
#include "settings.hpp"

class Option {
  protected:
    String m_name;
    bool m_isDone{false};
    std::function<void()> m_finishFunc;

  public:
    Option(String name, std::function<void()> finishFunc)
        : m_name(name), m_finishFunc(finishFunc) {}
    virtual String GetName() { return m_name; }
    virtual String GetCurrentValue() { return ""; };
    virtual void Begin(){};
    virtual void Update() {}
    virtual void Up() {}
    virtual void Down() {}
    virtual void Finish() {
        End();
        if (m_finishFunc) {
            m_finishFunc();
        }
    }
    virtual void End() {}

    virtual bool IsDone() { return m_isDone; }
};

class TextListOption : public Option {
  protected:
    Display& m_display;
    Settings& m_settings;
    std::vector<String> m_values;
    size_t m_index{0};
    bool m_useAnimation{true};

  public:
    TextListOption(Display& display,
                   Settings& settings,
                   String name,
                   std::vector<String> values,
                   std::function<void()> finishFunc)
        : Option(name, finishFunc),
          m_display(display),
          m_settings(settings),
          m_values(values) {}

    virtual void SetValues(std::vector<String> values) {
        m_values = values;
        Begin();
    }
    virtual String GetCurrentValue() override {
        return m_values.size() ? m_values[m_index] : "";
    }

    virtual void Begin() override {
        if (!m_values.size()) {
            return;
        }
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
        if (!m_values.size()) {
            return;
        }
        m_display.DrawText(0, m_values[m_index], GRAY);
        DrawArrows();
    }

    virtual void Up() override {
        if (!m_values.size()) {
            return;
        }
        if (m_index < m_values.size() - 1) {
            m_index++;
            if (m_useAnimation) {
                m_display.ScrollVertical(HEIGHT, 1);
            }
        }
    }
    virtual void Down() override {
        if (!m_values.size()) {
            return;
        }
        if (m_index > 0) {
            m_index--;
            if (m_useAnimation) {
                m_display.ScrollVertical(HEIGHT, -1);
            }
        }
    }

    virtual void End() override {
        if (m_values.size()) {
            m_settings[GetName()] = GetCurrentValue();
            m_settings.Save();
        }
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

class RangeOption : public TextListOption {
  public:
    RangeOption(Display& display,
                Settings& settings,
                String name,
                const int min,
                const int max,
                std::function<void()> finishFunc)
        : TextListOption(display, settings, name, {}, finishFunc) {
        m_useAnimation = false;
        for (int i = min; i <= max; ++i) {
            m_values.push_back(String(i));
        }
    }
};

class OneShotOption : public Option {
  private:
    std::function<void()> m_runFuncOnce;

  public:
    OneShotOption(String name,
                  std::function<void()> runFuncOnce,
                  std::function<void()> finishFunc)
        : Option(name, finishFunc), m_runFuncOnce(runFuncOnce) {}

    virtual void Begin() override {
        m_runFuncOnce();
        m_isDone = true;
    }
};