#pragma once
#include <stdint.h>  // for uint16_t and others

#include "elapsed_time.hpp"
#include "menu.hpp"
#include "rtc.hpp"

class Clock : public Menu {
  private:
    enum Mode_e {
        MODE_ANIM_OFF,
        MODE_SHIMMER,
        MODE_RAINBOW,
        MODE_MARQUEE,
        MODE_MARQUEE_SHIMMER,
        MODE_MARQUEE_RAINBOW,
        MODE_BINARY,
        MODE_BINARY_SHIMMER,
        MODE_BINARY_RAINBOW,
        TOTAL_MODES,
    };

    Rtc& m_rtc;

    int m_mode{(int)MODE_ANIM_OFF};
    bool m_shouldSaveSettings{false};
    ElapsedTime m_waitingToSaveSettings;
    ElapsedTime m_waitToAnimate;
    ElapsedTime m_showingColorWheel;
    ElapsedTime m_marqueeMovement;

    uint8_t m_colorWheelPos{0};
    uint32_t m_currentColor{0};
    bool m_displayingColorWheel{false};

    int m_marqueePos{0};

  public:
    Clock(Display& display, Rtc& rtc, Settings& settings)
        : Menu(display, settings), m_rtc(rtc) {
        if (!m_settings.containsKey(F("WLED"))) {
            m_settings[F("WLED")] = F("ON");
        }

        if (!m_settings.containsKey(F("COLR"))) {
            m_settings[F("COLR")] = m_colorWheelPos;
        } else {
            m_colorWheelPos = m_settings[F("COLR")].as<uint32_t>();
        }

        if (!m_settings.containsKey(F("MODE"))) {
            m_settings[F("MODE")] = m_mode;
        } else {
            m_mode = m_settings[F("MODE")].as<int>();
        }

        if (!m_settings.containsKey(F("CLKB"))) {
            m_settings[F("CLKB")] = F("ON");
        }
    }

    virtual void Update() {
        uint32_t color = Display::ColorWheel(m_colorWheelPos);
        m_currentColor = color;

        if (m_displayingColorWheel && m_showingColorWheel.Ms() > 750) {
            m_displayingColorWheel = false;
        }

        switch (m_mode) {
            case MODE_ANIM_OFF:
            case MODE_SHIMMER:
            case MODE_RAINBOW:
                DrawClockDigits(color);
                DrawSeparator(color);

                if (WiFi.isConnected() && m_settings[F("WLED")] == F("ON")) {
                    // it's fitting that 42 is exactly the right place for
                    // the WiFi status LED.
                    m_display.DrawPixel(42, color);
                }
                break;

            case MODE_MARQUEE:
            case MODE_MARQUEE_SHIMMER:
            case MODE_MARQUEE_RAINBOW: {
                m_display.Clear();
                char text[20];
                sprintf(text, "%2d:%02d:%02d",
                        m_settings[F("24HR")] == F("ON") ? m_rtc.Hour()
                                                         : m_rtc.Hour12(),
                        m_rtc.Minute(), m_rtc.Second());
                int length =
                    m_display.DrawText(m_marqueePos, text, m_currentColor) + 1;
                if (m_marqueeMovement.Ms() > 100) {
                    m_marqueeMovement.Reset();
                    if (--m_marqueePos == -length) {
                        m_marqueePos = WIDTH;
                    }
                }
                break;
            }

            case MODE_BINARY:
            case MODE_BINARY_SHIMMER:
            case MODE_BINARY_RAINBOW:
                break;

            default:
                break;
        }

        if (m_shouldSaveSettings && m_waitingToSaveSettings.Ms() >= 2000) {
            m_settings.Save();
            m_shouldSaveSettings = false;
        }

        DrawAnalog(color);
    }

    virtual bool Up(const Button::Event_e evt) {
        if (evt == Button::PRESS || evt == Button::REPEAT) {
            // cycle through all colors
            m_colorWheelPos -= 4;
            m_settings[F("COLR")] = m_colorWheelPos;

            m_displayingColorWheel = true;
            m_showingColorWheel.Reset();
            // wait to save the setting until the user has stopped changing
            // the color for a few seconds
            m_waitingToSaveSettings.Reset();
            m_shouldSaveSettings = true;
        }
        return true;
    }

    virtual bool Down(const Button::Event_e evt) {
        if (evt == Button::PRESS) {
            // cycle through modes
            if (++m_mode == TOTAL_MODES) {
                m_mode = MODE_ANIM_OFF;
            }
            m_settings[F("MODE")] = m_mode;
            m_waitingToSaveSettings.Reset();
            m_shouldSaveSettings = true;

            SetMode();
        }
        return true;
    }

    virtual void Activate() override { SetMode(false); }
    virtual void Hide() override { m_display.GetLEDs().clearColorOverride(); }

  private:
    void SetMode(bool showMessage = true) {
        m_display.GetLEDs().clearColorOverride();
        m_marqueePos = WIDTH;

        String message;
        switch (m_mode) {
            case MODE_ANIM_OFF:
                message = F("ANIM OFF");
                break;

            case MODE_SHIMMER:
                AddShimmerEffect();
                message = F("SHIMMER");
                break;

            case MODE_RAINBOW:
                AddRainbowEffect();
                message = F("RAINBOW");
                break;

            case MODE_MARQUEE:
                message = F("MARQUEE");
                break;

            case MODE_MARQUEE_SHIMMER:
                AddShimmerEffect();
                message = F("MARQUEE SHIMMER");
                break;

            case MODE_MARQUEE_RAINBOW:
                AddRainbowEffect();
                message = F("MARQUEE RAINBOW");
                break;

            case MODE_BINARY:
                message = F("BINARY");
                break;

            case MODE_BINARY_SHIMMER:
                AddShimmerEffect();
                message = F("BINARY SHIMMER");
                break;

            case MODE_BINARY_RAINBOW:
                AddRainbowEffect();
                message = F("BINARY RAINBOW");
                break;

            default:
                message = F("MODE ") + String(m_mode);
                break;
        }

        if (showMessage) {
            m_display.DrawTextScrolling(
                message, Display::ColorWheel(m_colorWheelPos), 50);
        }
    }

    void DrawClockDigits(uint32_t color) {
        if (m_display.IsAtMinimumBrightness()) {
            color = Display::ScaleBrightness(color, 0.6f);
        }

        m_display.Clear();

        char text[10];
        sprintf(
            text, "%2d",
            m_settings[F("24HR")] == F("ON") ? m_rtc.Hour() : m_rtc.Hour12());
        m_display.DrawText(0, text, color);
        sprintf(text, "%02d", m_rtc.Minute());
        m_display.DrawText(10, text, color);
    }

    void DrawSeparator(uint32_t color) {
        const float ms = m_rtc.Millis() / 1000.0f;
        const int transitionColor = Display::ScaleBrightness(
            m_currentColor,
            0.2f + (m_rtc.Second() % 2 ? ms : 1.0f - ms) * 0.8f);

        // force the pixels to be drawn in the edited color
        m_display.DrawPixel(WIDTH * 1 + 8, transitionColor, true);
        m_display.DrawPixel(WIDTH * 3 + 8, transitionColor, true);
    }

    void DrawAnalog(uint32_t color) {
        if (m_displayingColorWheel) {
            uint8_t colorWheel = m_colorWheelPos - 128;
            for (size_t i = 0; i < 12; ++i) {
                uint32_t color = Display::ColorWheel(colorWheel);
                colorWheel += 255 / 12;
                m_display.DrawPixel(FIRST_MINUTE_LED + i, color);
                m_display.DrawPixel(FIRST_HOUR_LED + i, color);
            }
        } else {
            m_display.ClearRoundLEDs(
                m_settings[F("CLKB")] == F("ON") ? DARK_GRAY : BLACK);

            m_display.DrawSecondLEDs(
                m_rtc.Second(), Display::ScaleBrightness(m_currentColor, 0.4f),
                true);
            m_display.DrawHourLED(m_rtc.Hour12(), m_currentColor);
            m_display.DrawMinuteLED(m_rtc.Minute(), m_currentColor);
        }
    }

    void AddShimmerEffect() {
        m_display.GetLEDs().setColorOverride(
            [&](uint16_t num, uint32_t& color) {
                static uint8_t jump = 0;
                static int pixelsChanged = 0;
                static float multiplier = 1.0f;
                const float ANIM_TIME_MS = 600.0f;
                if (m_waitToAnimate.Ms() > ANIM_TIME_MS) {
                    m_waitToAnimate.Reset();
                    jump--;
                }

                if (num == 0) {
                    pixelsChanged = 0;
                }

                if (color != BLACK && num < WIDTH * HEIGHT) {
                    if ((++pixelsChanged + jump) % 4 == 0) {
                        multiplier = m_waitToAnimate.Ms() / ANIM_TIME_MS;
                        if (m_waitToAnimate.Ms() < ANIM_TIME_MS / 2) {
                            multiplier = 1.0f - multiplier;
                        }

                        color = Display::ScaleBrightness(
                            color, 0.1f + (multiplier * 0.7f));
                    }
                }
            });
    }

    void AddRainbowEffect() {
        m_display.GetLEDs().setColorOverride(
            [&](uint16_t num, uint32_t& color) {
                static uint8_t baseColor = 128;
                static uint8_t curColor = 0;

                if (m_waitToAnimate.Ms() > 50) {
                    m_waitToAnimate.Reset();
                    baseColor++;
                }
                if (num == 0) {
                    curColor = 0;
                }

                // DARK_GRAY is the color of the analog clock ring
                if (color != BLACK && color != DARK_GRAY) {
                    curColor += 4;
                    m_currentColor = color =
                        Display::ColorWheel(baseColor + curColor);
                }
            });
    }
};
