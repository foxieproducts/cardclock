#pragma once
#include <Arduino.h>   // for pinMode(), digitalRead(), etc.
#include <functional>  // for std::function
#include <vector>      // for std::vector

#include "elapsed_time.hpp"

enum Pins_e {
    PIN_BTN_UP = 10,
    PIN_BTN_DOWN = 0,  // has external pull-up resistor
    PIN_BTN_LEFT = 12,
    PIN_BTN_RIGHT = 14,
};

class Button {
  public:
    enum Event_e {
        PRESS,
        REPEAT,
        RELEASE,
    };

  private:
    std::vector<int> m_pins;  // a Button can have multiple input pins
    bool m_currentPinState{false};

    bool m_enabled{true};
    bool m_isPressed{false};
    bool m_wasPressed{false};

    ElapsedTime m_timeInState;
    ElapsedTime m_timeSinceRepeat;

  public:
    using HandlerFunc_t = std::function<void(const Event_e evt)>;
    struct Config {
        bool canRepeat{true};

        // values below are in milliseconds
        size_t repeatRate{50};
        size_t delayBeforeRepeat{400};
        size_t delayBeforePressEvent{0};  // must be held prior to a PRESS event
        size_t debounceTime{5};
        HandlerFunc_t handlerFunc;
    } config;

    Button(int pin, int inputType) : Button({pin}, inputType) {}
    Button(std::initializer_list<int> pins, int inputType) {
        m_pins = pins;
        for (auto pin : m_pins) {
            pinMode(pin, inputType);
        }
    }

    void Update() {
        if (m_enabled) {
            CheckForPinStateChange();
            CheckForEventsToSend();
        }
    }

    void SetEnabled(const bool enabled) {
        m_enabled = enabled;
        Reset();
    }

    bool IsPressed() { return m_enabled && m_isPressed; }

    void Reset() {
        m_timeInState.Reset();
        m_isPressed = false;
        m_wasPressed = false;
    }

    static int AreAnyButtonsPressed() {
        if (digitalRead(PIN_BTN_UP) == LOW) {
            return PIN_BTN_UP;
        }
        if (digitalRead(PIN_BTN_DOWN) == LOW) {
            return PIN_BTN_DOWN;
        }
        if (digitalRead(PIN_BTN_LEFT) == LOW) {
            return PIN_BTN_LEFT;
        }
        if (digitalRead(PIN_BTN_RIGHT) == LOW) {
            return PIN_BTN_RIGHT;
        }
        return -1;
    }

    static int WaitForButtonPress(const size_t maxWaitMs = 10000) {
        ElapsedTime wait;
        while (true) {
            int button = AreAnyButtonsPressed();
            if (button != -1) {
                return button;
            }

            if (maxWaitMs && wait.Ms() > maxWaitMs) {
                return ERR_TIMEOUT;
            }
            yield();
        }
    }

    static void WaitForNoButtons(const size_t maxWaitMs = 10000) {
        ElapsedTime wait;
        while (AreAnyButtonsPressed() != -1) {
            if (maxWaitMs && wait.Ms() > maxWaitMs) {
                return;
            }
            yield();
        }
        return;
    }

  private:
    void CheckForPinStateChange() {
        if (m_timeInState.Ms() < config.debounceTime) {
            // do nothing while we're debouncing because this is
            // when the button can physically bounce.
            return;
        }

        bool lastPinState = m_currentPinState;
        m_currentPinState = GetCombinedPinState();
        if (lastPinState != m_currentPinState &&
            m_currentPinState != m_isPressed) {
            m_timeInState.Reset();
            m_isPressed = m_currentPinState;
        }
    }

    void CheckForEventsToSend() {
        if (!config.handlerFunc) {
            return;
        }

        if (m_isPressed != m_wasPressed) {
            if (m_isPressed &&
                m_timeInState.Ms() < config.delayBeforePressEvent) {
                return;
            }

            m_wasPressed = m_isPressed;
            m_timeSinceRepeat.Reset();

            config.handlerFunc(m_isPressed ? PRESS : RELEASE);
        } else if (m_isPressed &&
                   (m_timeInState.Ms() >= config.delayBeforeRepeat) &&
                   (m_timeSinceRepeat.Ms() >= config.repeatRate)) {
            m_timeSinceRepeat.Reset();
            if (config.canRepeat) {
                config.handlerFunc(REPEAT);
            }
        }
    }

    bool GetCombinedPinState() {
        bool state = true;
        for (auto pin : m_pins) {
            state = state && (digitalRead(pin) == LOW);
        }
        return state;
    }
};